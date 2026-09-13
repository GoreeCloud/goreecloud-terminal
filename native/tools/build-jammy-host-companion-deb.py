#!/usr/bin/env python3
"""Build the inert Development .deb for the Zorin 17.3/Jammy host companion."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import stat
import subprocess
import sys
import tempfile
from pathlib import Path

PACKAGE_NAME = "goreecloud-terminal-host-companion"
MAINTAINER = "GoreeCloud <295260680+GoreeCloud@users.noreply.github.com>"
DEPENDENCIES = (
    "libc6 (>= 2.35), "
    "libglib2.0-0 (>= 2.72), "
    "libglib2.0-bin (>= 2.72), "
    "systemd (>= 249)"
)
EXPECTED_PAYLOAD = {
    "/usr/bin/goreecloud-terminal-migrate": 0o755,
    "/usr/libexec/goreecloud-terminal-host-agent": 0o755,
    "/usr/lib/systemd/user/goreecloud-terminal-host-agent.service": 0o644,
    "/usr/share/glib-2.0/schemas/com.goreecloud.Terminal.Migration.gschema.xml": 0o644,
}
SEMVER = re.compile(r"^(?P<core>[0-9]+\.[0-9]+\.[0-9]+)(?:-(?P<pre>[0-9A-Za-z.-]+))?$")
REVISION = re.compile(r"^[0-9A-Za-z.+~]+$")
SHA40 = re.compile(r"^[0-9a-f]{40}$")


def die(message: str) -> "NoReturn":
    print(f"host-companion-deb: {message}", file=sys.stderr)
    raise SystemExit(1)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def debian_version(source_version: str, revision: str) -> str:
    match = SEMVER.fullmatch(source_version)
    if match is None:
        die(f"unsupported source version: {source_version!r}")
    if REVISION.fullmatch(revision) is None:
        die(f"invalid Debian revision: {revision!r}")

    core = match.group("core")
    prerelease = match.group("pre")
    upstream = core if prerelease is None else f"{core}~{prerelease}"
    return f"{upstream}-{revision}"


def stage_files(stage: Path) -> dict[str, Path]:
    if not stage.is_dir():
        die(f"staging root does not exist: {stage}")

    actual: dict[str, Path] = {}
    for path in stage.rglob("*"):
        if path.is_symlink():
            die(f"symlink is not allowed in host companion payload: {path}")
        if path.is_file():
            absolute = "/" + path.relative_to(stage).as_posix()
            actual[absolute] = path

    missing = sorted(set(EXPECTED_PAYLOAD) - set(actual))
    unexpected = sorted(set(actual) - set(EXPECTED_PAYLOAD))
    if missing or unexpected:
        if missing:
            print("host-companion-deb: missing staged files:", file=sys.stderr)
            for item in missing:
                print(f"  {item}", file=sys.stderr)
        if unexpected:
            print("host-companion-deb: unexpected staged files:", file=sys.stderr)
            for item in unexpected:
                print(f"  {item}", file=sys.stderr)
        raise SystemExit(1)

    for absolute, expected_mode in EXPECTED_PAYLOAD.items():
        path = actual[absolute]
        mode = stat.S_IMODE(path.stat().st_mode)
        if mode != expected_mode:
            die(f"mode drift for {absolute}: expected {expected_mode:04o}, got {mode:04o}")

    return actual


def write_control(root: Path, version: str, architecture: str, installed_size: int) -> None:
    control_dir = root / "DEBIAN"
    control_dir.mkdir(mode=0o755)
    control = control_dir / "control"
    control.write_text(
        "\n".join(
            [
                f"Package: {PACKAGE_NAME}",
                f"Version: {version}",
                "Section: utils",
                "Priority: optional",
                f"Architecture: {architecture}",
                f"Maintainer: {MAINTAINER}",
                f"Installed-Size: {installed_size}",
                f"Depends: {DEPENDENCIES}",
                "Description: Development host companion for GoreeCloud Terminal",
                " Provides the same-user host PTY agent and explicit transitional",
                " migration/rollback maintenance command for the controlled Zorin OS",
                " 17.3 / Ubuntu 22.04 workstation path. It does not contain the GTK/VTE",
                " Terminal GUI, does not start or enable the user service automatically,",
                " and does not run migration automatically.",
                "",
            ]
        ),
        encoding="utf-8",
    )
    control.chmod(0o644)


def normalize_tree(root: Path, epoch: int) -> None:
    for path in sorted(root.rglob("*"), key=lambda item: len(item.parts), reverse=True):
        if path.is_symlink():
            die(f"unexpected symlink in package root: {path}")
        os.utime(path, (epoch, epoch), follow_symlinks=False)
    os.utime(root, (epoch, epoch), follow_symlinks=False)


def build_package(
    stage: Path,
    output_dir: Path,
    source_version: str,
    source_sha: str,
    revision: str,
    architecture: str,
    source_date_epoch: int,
) -> tuple[Path, Path]:
    if SHA40.fullmatch(source_sha) is None:
        die("source SHA must be a lowercase 40-character Git SHA")
    if architecture != "amd64":
        die(f"issue #99 currently qualifies only the controlled amd64 workstation, not {architecture!r}")
    if source_date_epoch <= 0:
        die("SOURCE_DATE_EPOCH must be a positive Unix timestamp")

    files = stage_files(stage)
    package_version = debian_version(source_version, revision)
    output_dir.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="goree-terminal-deb-") as temporary:
        package_root = Path(temporary) / "root"
        package_root.mkdir(mode=0o755)

        payload_evidence = []
        installed_bytes = 0
        for absolute, source in sorted(files.items()):
            target = package_root / absolute.lstrip("/")
            target.parent.mkdir(parents=True, exist_ok=True, mode=0o755)
            shutil.copyfile(source, target)
            target.chmod(EXPECTED_PAYLOAD[absolute])
            size = target.stat().st_size
            installed_bytes += size
            payload_evidence.append(
                {
                    "path": absolute,
                    "mode": f"{EXPECTED_PAYLOAD[absolute]:04o}",
                    "size": size,
                    "sha256": sha256(target),
                }
            )

        installed_size = max(1, (installed_bytes + 1023) // 1024)
        write_control(package_root, package_version, architecture, installed_size)
        normalize_tree(package_root, source_date_epoch)

        filename = f"{PACKAGE_NAME}_{package_version}_{architecture}.deb"
        package_path = output_dir / filename
        environment = os.environ.copy()
        environment["SOURCE_DATE_EPOCH"] = str(source_date_epoch)
        subprocess.run(
            [
                "dpkg-deb",
                "--root-owner-group",
                "-Zxz",
                "-z9",
                "--build",
                str(package_root),
                str(package_path),
            ],
            check=True,
            env=environment,
        )

    provenance = {
        "schema_version": 1,
        "status": "development",
        "package_name": PACKAGE_NAME,
        "source_sha": source_sha,
        "source_version": source_version,
        "debian_version": package_version,
        "debian_revision": revision,
        "architecture": architecture,
        "target": "Zorin OS 17.3 / Ubuntu 22.04 Jammy host companion",
        "gui_included": False,
        "automatic_host_agent_enablement": False,
        "automatic_host_agent_start": False,
        "automatic_migration": False,
        "automatic_user_state_deletion": False,
        "signed": False,
        "production_approved": False,
        "stable_approved": False,
        "package_file": package_path.name,
        "package_size": package_path.stat().st_size,
        "package_sha256": sha256(package_path),
        "payload": payload_evidence,
    }
    provenance_path = output_dir / f"{package_path.name}.provenance.json"
    provenance_path.write_text(
        json.dumps(provenance, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    os.utime(provenance_path, (source_date_epoch, source_date_epoch))
    return package_path, provenance_path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--stage", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--source-version", required=True)
    parser.add_argument("--source-sha", required=True)
    parser.add_argument("--debian-revision", default="1")
    parser.add_argument("--architecture", default="amd64")
    parser.add_argument("--source-date-epoch", required=True, type=int)
    args = parser.parse_args()

    package_path, provenance_path = build_package(
        args.stage.resolve(),
        args.output_dir.resolve(),
        args.source_version,
        args.source_sha,
        args.debian_revision,
        args.architecture,
        args.source_date_epoch,
    )
    print(package_path)
    print(provenance_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
