#!/usr/bin/env python3

import array
import errno
import os
from pathlib import Path
import pwd
import selectors
import socket
import stat
import struct
import subprocess
import sys
import tempfile
import time

MAGIC = 0x47435448
VERSION = 2
SPAWN_REQUEST = 1
SPAWN_RESPONSE = 2
EXIT = 3
ERROR = 4
ENV_INHERIT_SAFE = 0
ENV_CLEAN = 1
HEADER = struct.Struct("=IHHiII")
SPAWN = struct.Struct("=IHHiIIII256s1024s" + ("64s" * 16))
MARKER = b"__GOREE_HOST_PTY_OK__"
PROFILE_DONE = b"__PROFILE_DONE__"


def wait_for_socket(path: Path, process: subprocess.Popen[str], timeout: float = 5.0) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if process.poll() is not None:
            stdout, stderr = process.communicate(timeout=1)
            raise RuntimeError(
                f"host agent exited before socket creation: rc={process.returncode}\n"
                f"stdout={stdout}\nstderr={stderr}"
            )
        if path.exists():
            return
        time.sleep(0.05)
    raise TimeoutError(f"host-agent socket was not created: {path}")


def recv_exact(sock: socket.socket, length: int, timeout: float = 5.0) -> bytes:
    sock.settimeout(timeout)
    data = bytearray()
    while len(data) < length:
        chunk = sock.recv(length - len(data))
        if not chunk:
            raise EOFError(f"socket closed with {length - len(data)} bytes still expected")
        data.extend(chunk)
    return bytes(data)


def recv_response(
    sock: socket.socket,
) -> tuple[tuple[int, int, int, int, int, int], int | None]:
    sock.settimeout(5.0)
    payload = bytearray()
    received_fd = None

    while len(payload) < HEADER.size:
        chunk, ancdata, _flags, _address = sock.recvmsg(
            HEADER.size - len(payload), socket.CMSG_SPACE(array.array("i").itemsize)
        )
        if not chunk:
            raise EOFError("socket closed before host-agent response")
        payload.extend(chunk)

        for level, cmsg_type, cmsg_data in ancdata:
            if level == socket.SOL_SOCKET and cmsg_type == socket.SCM_RIGHTS:
                fds = array.array("i")
                usable = len(cmsg_data) - (len(cmsg_data) % fds.itemsize)
                fds.frombytes(cmsg_data[:usable])
                if len(fds) != 1 or received_fd is not None:
                    for fd in fds:
                        os.close(fd)
                    raise AssertionError("response may carry at most one PTY fd")
                received_fd = fds[0]

    return HEADER.unpack(bytes(payload)), received_fd


def recv_spawn_response(sock: socket.socket) -> tuple[tuple[int, int, int, int, int, int], int]:
    header, received_fd = recv_response(sock)
    if received_fd is None:
        raise AssertionError(f"spawn response did not carry a PTY master fd: {header!r}")
    return header, received_fd


def read_pty_until(fd: int, marker: bytes, timeout: float = 5.0) -> bytes:
    selector = selectors.DefaultSelector()
    selector.register(fd, selectors.EVENT_READ)
    deadline = time.monotonic() + timeout
    output = bytearray()

    try:
        while time.monotonic() < deadline:
            events = selector.select(max(0.0, deadline - time.monotonic()))
            if not events:
                continue
            try:
                chunk = os.read(fd, 4096)
            except OSError as exc:
                if exc.errno == errno.EIO:
                    break
                raise
            if not chunk:
                break
            output.extend(chunk)
            if marker in output:
                return bytes(output)
    finally:
        selector.close()

    raise AssertionError(f"PTY marker not observed: {marker!r}; output={bytes(output)!r}")


def fixed(value: str, size: int) -> bytes:
    encoded = value.encode("utf-8")
    if len(encoded) >= size:
        raise ValueError(f"field is too long for {size}-byte slot")
    return encoded + (b"\0" * (size - len(encoded)))


def pack_spawn(
    *,
    shell: str = "",
    cwd: str = "",
    environment_policy: int = ENV_INHERIT_SAFE,
    environment_names: tuple[str, ...] = (),
    rows: int = 24,
    columns: int = 80,
) -> bytes:
    if len(environment_names) > 16:
        raise ValueError("too many environment names")

    names = [fixed(name, 64) for name in environment_names]
    names.extend([b"\0" * 64] * (16 - len(names)))
    return SPAWN.pack(
        MAGIC,
        VERSION,
        SPAWN_REQUEST,
        0,
        rows,
        columns,
        environment_policy,
        len(environment_names),
        fixed(shell, 256),
        fixed(cwd, 1024),
        *names,
    )


def assert_error(socket_path: Path, request: bytes, expected_errno: int) -> None:
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.connect(str(socket_path))
        client.sendall(request)
        header, received_fd = recv_response(client)
        if received_fd is not None:
            os.close(received_fd)
            raise AssertionError("rejected request unexpectedly returned a PTY")
        magic, version, msg_type, error_number, rows, columns = header
        assert magic == MAGIC
        assert version == VERSION
        assert msg_type == ERROR
        assert error_number == expected_errno, (error_number, expected_errno)
        assert rows == 0 and columns == 0


def run_default_shell_case(socket_path: Path) -> None:
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.connect(str(socket_path))
        client.sendall(pack_spawn())

        response, pty_fd = recv_spawn_response(client)
        try:
            magic, version, msg_type, child_pid, rows, columns = response
            assert magic == MAGIC
            assert version == VERSION
            assert msg_type == SPAWN_RESPONSE
            assert child_pid > 0
            assert rows == 0 and columns == 0
            assert os.isatty(pty_fd), "received fd is not a PTY"

            os.write(pty_fd, b"printf '__GOREE_HOST_PTY_OK__\\n'; exit\n")
            output = read_pty_until(pty_fd, MARKER)
            assert MARKER in output

            exit_message = HEADER.unpack(recv_exact(client, HEADER.size))
            assert exit_message[0] == MAGIC
            assert exit_message[1] == VERSION
            assert exit_message[2] == EXIT
            assert exit_message[4] == 0 and exit_message[5] == 0
        finally:
            os.close(pty_fd)


def run_profile_launch_case(socket_path: Path, profile_dir: Path) -> None:
    account = pwd.getpwuid(os.getuid())
    shell = account.pw_shell or "/bin/sh"
    assert os.path.isabs(shell)
    assert os.access(shell, os.X_OK)

    request = pack_spawn(
        shell=shell,
        cwd=str(profile_dir),
        environment_policy=ENV_CLEAN,
        environment_names=("GOREE_TERMINAL_TEST_ALLOWED",),
    )
    assert b"profile-value" not in request
    assert b"must-not-leak" not in request

    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.connect(str(socket_path))
        client.sendall(request)
        response, pty_fd = recv_spawn_response(client)
        try:
            assert response[0] == MAGIC
            assert response[1] == VERSION
            assert response[2] == SPAWN_RESPONSE
            command = (
                "printf '__PROFILE_PWD__%s\\n' \"$PWD\"; "
                "printf '__PROFILE_ALLOWED__%s\\n' \"$GOREE_TERMINAL_TEST_ALLOWED\"; "
                "printf '__PROFILE_UNLISTED__%s\\n' \"${GOREE_TERMINAL_TEST_UNLISTED-unset}\"; "
                "printf '__PROFILE_%s__\\n' 'DONE'; exit\n"
            ).encode("utf-8")
            os.write(pty_fd, command)
            output = read_pty_until(pty_fd, PROFILE_DONE)
            normalized = output.replace(b"\r", b"")
            assert f"__PROFILE_PWD__{profile_dir}\n".encode() in normalized
            assert b"__PROFILE_ALLOWED__profile-value\n" in normalized
            assert b"__PROFILE_UNLISTED__unset\n" in normalized

            exit_message = HEADER.unpack(recv_exact(client, HEADER.size))
            assert exit_message[2] == EXIT
        finally:
            os.close(pty_fd)


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} /path/to/goreecloud-terminal-host-agent", file=sys.stderr)
        return 2

    agent = Path(sys.argv[1]).resolve()
    if not agent.is_file():
        raise FileNotFoundError(agent)

    with tempfile.TemporaryDirectory(prefix="goree-host-agent-test-") as runtime:
        env = os.environ.copy()
        env["XDG_RUNTIME_DIR"] = runtime
        env["GOREE_TERMINAL_TEST_ALLOWED"] = "profile-value"
        env["GOREE_TERMINAL_TEST_UNLISTED"] = "must-not-leak"
        process = subprocess.Popen(
            [str(agent)],
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        runtime_dir = Path(runtime) / "goreecloud-terminal"
        socket_path = runtime_dir / "host-agent.sock"
        profile_dir = Path(runtime) / "profile-working-directory"
        profile_dir.mkdir(mode=0o700)
        unapproved_shell = Path(runtime) / "not-a-login-shell"
        unapproved_shell.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
        unapproved_shell.chmod(0o700)

        try:
            wait_for_socket(socket_path, process)

            directory_mode = stat.S_IMODE(runtime_dir.stat().st_mode)
            socket_stat = socket_path.stat()
            socket_mode = stat.S_IMODE(socket_stat.st_mode)
            assert directory_mode == 0o700, oct(directory_mode)
            assert socket_mode == 0o600, oct(socket_mode)
            assert stat.S_ISSOCK(socket_stat.st_mode)

            printed_socket = subprocess.check_output(
                [str(agent), "--print-socket"], env=env, text=True
            ).strip()
            assert printed_socket == str(socket_path)

            run_default_shell_case(socket_path)
            run_profile_launch_case(socket_path, profile_dir)

            assert_error(
                socket_path,
                pack_spawn(shell=str(unapproved_shell)),
                errno.EPERM,
            )
            assert_error(
                socket_path,
                pack_spawn(cwd="relative-directory"),
                errno.EPROTO,
            )
            assert_error(
                socket_path,
                pack_spawn(environment_names=("INVALID=VALUE",)),
                errno.EPROTO,
            )
        finally:
            process.terminate()
            try:
                stdout, stderr = process.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                stdout, stderr = process.communicate(timeout=5)

            if process.returncode not in (0, -15, 143):
                raise RuntimeError(
                    f"host agent terminated unexpectedly: rc={process.returncode}\n"
                    f"stdout={stdout}\nstderr={stderr}"
                )

    print("native host-session PTY and launch-context contract: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
