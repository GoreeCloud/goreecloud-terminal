#!/usr/bin/env python3
"""Generate a production-identity no-host-filesystem Flatpak candidate.

The canonical production manifest remains untouched. This tool derives a temporary
candidate from com.goreecloud.Terminal.json and permits exactly one manifest delta:
removal of --filesystem=host from finish-args.

The generated manifest is acceptance input only. It does not establish production or
Stable approval and must not replace the canonical production manifest without the
separate supported-workstation acceptance required by GoreeCloud release governance.
"""

from __future__ import annotations

import argparse
import copy
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASE_MANIFEST = ROOT / "com.goreecloud.Terminal.json"
DEFAULT_OUTPUT = ROOT / "com.goreecloud.Terminal.NoHostFS.generated.json"
REMOVED_PERMISSION = "--filesystem=host"
EXPECTED_APP_ID = "com.goreecloud.Terminal"


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def derive_candidate(base: dict) -> dict:
    if base.get("app-id") != EXPECTED_APP_ID:
        raise SystemExit("unexpected canonical production app-id")

    finish_args = base.get("finish-args")
    if not isinstance(finish_args, list):
        raise SystemExit("canonical production manifest is missing finish-args")
    if finish_args.count(REMOVED_PERMISSION) != 1:
        raise SystemExit(
            f"canonical production manifest must contain {REMOVED_PERMISSION} exactly once"
        )

    candidate = copy.deepcopy(base)
    candidate["finish-args"] = [
        permission for permission in finish_args if permission != REMOVED_PERMISSION
    ]

    if len(candidate["finish-args"]) != len(finish_args) - 1:
        raise SystemExit("candidate permission delta is not exactly one removal")
    if REMOVED_PERMISSION in candidate["finish-args"]:
        raise SystemExit("candidate still contains host filesystem access")

    restored = copy.deepcopy(candidate)
    restored["finish-args"] = copy.deepcopy(finish_args)
    if restored != base:
        raise SystemExit("candidate changed fields outside finish-args")

    return candidate


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help="Path for the generated production candidate manifest",
    )
    args = parser.parse_args()

    base = load_json(BASE_MANIFEST)
    candidate = derive_candidate(base)

    output = args.output
    if not output.is_absolute():
        output = ROOT / output
    output.write_text(json.dumps(candidate, indent=2) + "\n", encoding="utf-8")

    print(f"Generated production-identity candidate: {output}")
    print(f"Application ID: {EXPECTED_APP_ID}")
    print(f"Removed permission: {REMOVED_PERMISSION}")
    print("Canonical production manifest was not modified")
    print("production_approved=false stable_approved=false")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
