#!/usr/bin/env python3

import array
import os
from pathlib import Path
import selectors
import socket
import stat
import struct
import subprocess
import sys
import tempfile
import time

MAGIC = 0x47435448
VERSION = 1
SPAWN_REQUEST = 1
SPAWN_RESPONSE = 2
EXIT = 3
HEADER = struct.Struct("=IHHiII")
MARKER = b"__GOREE_HOST_PTY_OK__"


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


def recv_spawn_response(sock: socket.socket) -> tuple[tuple[int, int, int, int, int, int], int]:
    sock.settimeout(5.0)
    payload = bytearray()
    received_fd = None

    while len(payload) < HEADER.size:
        chunk, ancdata, _flags, _address = sock.recvmsg(
            HEADER.size - len(payload), socket.CMSG_SPACE(array.array("i").itemsize)
        )
        if not chunk:
            raise EOFError("socket closed before spawn response")
        payload.extend(chunk)

        for level, cmsg_type, cmsg_data in ancdata:
            if level == socket.SOL_SOCKET and cmsg_type == socket.SCM_RIGHTS:
                fds = array.array("i")
                usable = len(cmsg_data) - (len(cmsg_data) % fds.itemsize)
                fds.frombytes(cmsg_data[:usable])
                if len(fds) != 1 or received_fd is not None:
                    for fd in fds:
                        os.close(fd)
                    raise AssertionError("spawn response must carry exactly one PTY fd")
                received_fd = fds[0]

    header = HEADER.unpack(bytes(payload))
    if received_fd is None:
        raise AssertionError("spawn response did not carry a PTY master fd")
    return header, received_fd


def read_pty_until_marker(fd: int, timeout: float = 5.0) -> bytes:
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
                # Linux PTY masters commonly return EIO after the slave closes.
                if exc.errno == 5:
                    break
                raise
            if not chunk:
                break
            output.extend(chunk)
            if MARKER in output:
                return bytes(output)
    finally:
        selector.close()

    raise AssertionError(f"host shell marker not observed; output={bytes(output)!r}")


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
        process = subprocess.Popen(
            [str(agent)],
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        runtime_dir = Path(runtime) / "goreecloud-terminal"
        socket_path = runtime_dir / "host-agent.sock"

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

            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
                client.connect(str(socket_path))
                client.sendall(
                    HEADER.pack(MAGIC, VERSION, SPAWN_REQUEST, 0, 24, 80)
                )

                response, pty_fd = recv_spawn_response(client)
                try:
                    magic, version, msg_type, child_pid, rows, columns = response
                    assert magic == MAGIC
                    assert version == VERSION
                    assert msg_type == SPAWN_RESPONSE
                    assert child_pid > 0
                    assert rows == 0 and columns == 0
                    assert os.isatty(pty_fd), "received fd is not a PTY"

                    os.write(pty_fd, b"printf '__GOREE_HOST_PTY_OK__\\n'; exit\\n")
                    output = read_pty_until_marker(pty_fd)
                    assert MARKER in output

                    exit_message = HEADER.unpack(recv_exact(client, HEADER.size))
                    assert exit_message[0] == MAGIC
                    assert exit_message[1] == VERSION
                    assert exit_message[2] == EXIT
                    assert exit_message[4] == 0 and exit_message[5] == 0
                finally:
                    os.close(pty_fd)
        finally:
            process.terminate()
            try:
                stdout, stderr = process.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                stdout, stderr = process.communicate(timeout=5)

            # The host agent exits 143 from its explicit SIGTERM cleanup handler.
            if process.returncode not in (0, -15, 143):
                raise RuntimeError(
                    f"host agent terminated unexpectedly: rc={process.returncode}\n"
                    f"stdout={stdout}\nstderr={stderr}"
                )

    print("native host-session PTY contract: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
