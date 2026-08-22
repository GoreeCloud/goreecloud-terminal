#!/usr/bin/env python3
"""Generate a development-only GoreeCloud Terminal Flatpak permission candidate.

The canonical production and development manifests remain untouched. This tool derives
an experimental development manifest from the canonical development manifest and
allows exactly one permission delta: removal of --filesystem=host.

The generated manifest is evidence input for CI and supported-workstation testing. It
is not a production manifest and cannot establish production or Stable acceptance.
"""

from __future__ import annotations

import argparse
import copy
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BASE_MANIFEST = ROOT / "com.goreecloud.Terminal.Devel.json"
DEFAULT_OUTPUT = ROOT / "com.goreecloud.Terminal.Devel.NoHostFS.generated.json"
REMOVED_PERMISSION = "--filesystem=host"


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def derive_candidate(base: dict) -> dict:
    finish_args = base.get("finish-args")
    if not isinstance(finish_args, list):
        raise SystemExit("canonical development manifest is missing finish-args")
    if finish_args.count(REMOVED_PERMISSION) != 1:
        raise SystemExit(
            f"canonical development manifest must contain {REMOVED_PERMISSION} exactly once"
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
        help="Path for the generated candidate manifest",
    )
    args = parser.parse_args()

    base = load_json(BASE_MANIFEST)
    candidate = derive_candidate(base)

    if base.get("app-id") != "com.goreecloud.Terminal.Devel":
        raise SystemExit("unexpected canonical development app-id")

    output = args.output
    if not output.is_absolute():
        output = ROOT / output
    output.write_text(json.dumps(candidate, indent=2) + "\n", encoding="utf-8")

    print(f"Generated development-only candidate: {output}")
    print(f"Removed permission: {REMOVED_PERMISSION}")
    print("Canonical manifests were not modified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
