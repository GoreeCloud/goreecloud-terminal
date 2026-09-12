#!/usr/bin/env python3
"""Render a deterministic non-secret manifest for a staged Terminal package payload."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import stat
from pathlib import Path


IDENTITIES = {
    "development": "com.goreecloud.Terminal.Devel",
    "production": "com.goreecloud.Terminal",
}

VERSION_RE = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?$")
SOURCE_RE = re.compile(r"^[0-9a-f]{40}$")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def collect_files(stage: Path) -> list[dict]:
    files: list[dict] = []
    for path in sorted(stage.rglob("*"), key=lambda value: value.as_posix()):
        if path.is_dir():
            continue
        metadata = path.lstat()
        if not stat.S_ISREG(metadata.st_mode):
            raise SystemExit(f"payload-manifest: non-regular staged path is not allowed: {path}")
        relative = "/" + path.relative_to(stage).as_posix()
        files.append(
            {
                "path": relative,
                "sha256": sha256_file(path),
                "size": metadata.st_size,
                "mode": format(stat.S_IMODE(metadata.st_mode), "04o"),
            }
        )
    if not files:
        raise SystemExit("payload-manifest: staged payload is empty")
    return files


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("stage", type=Path)
    parser.add_argument("identity", choices=tuple(IDENTITIES))
    parser.add_argument("version")
    parser.add_argument("source_revision")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    stage = args.stage.resolve()
    if not stage.is_dir():
        raise SystemExit(f"payload-manifest: staging root does not exist: {stage}")
    if VERSION_RE.fullmatch(args.version) is None:
        raise SystemExit(f"payload-manifest: invalid version: {args.version}")
    if SOURCE_RE.fullmatch(args.source_revision) is None:
        raise SystemExit("payload-manifest: source revision must be a lowercase 40-character Git SHA")

    manifest = {
        "schema_version": 1,
        "kind": "goreecloud-terminal-staged-package-payload",
        "lifecycle": "development",
        "package_format": "unselected",
        "source_revision": args.source_revision,
        "version": args.version,
        "identity": args.identity,
        "application_id": IDENTITIES[args.identity],
        "install_prefix": "/usr",
        "files": collect_files(stage),
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    os.chmod(args.output, 0o644)
    print(
        f"payload-manifest: PASS identity={args.identity} "
        f"version={args.version} files={len(manifest['files'])}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
