#!/usr/bin/env python3
"""Validate that the host-agent user service preserves spawned-shell semantics."""

from __future__ import annotations

import argparse
from pathlib import Path


ALLOWED_SERVICE_DIRECTIVES = {
    "Type",
    "ExecStart",
    "Restart",
    "RestartSec",
}

REQUIRED_SERVICE_VALUES = {
    "Type": "simple",
    "Restart": "on-failure",
    "RestartSec": "2s",
}


def fail(message: str) -> None:
    raise SystemExit(f"host-agent service contract failed: {message}")


def parse_service(path: Path) -> dict[str, str]:
    section = ""
    directives: dict[str, str] = {}

    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw_line.strip()
        if not line or line.startswith(("#", ";")):
            continue
        if line.startswith("[") and line.endswith("]"):
            section = line[1:-1]
            continue
        if section != "Service":
            continue
        if "=" not in line:
            fail(f"invalid [Service] line {line_number}: {raw_line!r}")

        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip()
        if key not in ALLOWED_SERVICE_DIRECTIVES:
            fail(
                f"{key}= is not permitted in [Service]; execution restrictions and "
                "environment mutations are inherited by spawned host shells"
            )
        if key in directives:
            fail(f"duplicate [Service] directive: {key}=")
        directives[key] = value

    return directives


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("service", type=Path)
    parser.add_argument("--expect-exec", required=True)
    args = parser.parse_args()

    directives = parse_service(args.service)

    for key, expected in REQUIRED_SERVICE_VALUES.items():
        actual = directives.get(key)
        if actual != expected:
            fail(f"expected {key}={expected}, found {key}={actual!r}")

    actual_exec = directives.get("ExecStart")
    if actual_exec != args.expect_exec:
        fail(f"expected ExecStart={args.expect_exec}, found ExecStart={actual_exec!r}")

    missing = ALLOWED_SERVICE_DIRECTIVES - directives.keys()
    if missing:
        fail(f"missing required [Service] directive(s): {', '.join(sorted(missing))}")

    print(
        "host-agent service contract passed: lifecycle-only unit preserves descendant "
        "host-shell semantics"
    )


if __name__ == "__main__":
    main()
