#!/usr/bin/env python3
"""Validate the GoreeCloud Terminal Flatpak permission-review contract.

This is a source-review guardrail. It does not alter Flatpak permissions and cannot
establish supported-workstation, production, or Stable acceptance.
"""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PROD_MANIFEST = ROOT / "com.goreecloud.Terminal.json"
DEV_MANIFEST = ROOT / "com.goreecloud.Terminal.Devel.json"
REVIEW_PATH = ROOT / "release/flatpak-permission-review.json"
DOC_PATH = ROOT / "docs/flatpak-permission-minimization.md"

EXPECTED_PERMISSIONS = {
    "--allow=devel",
    "--device=dri",
    "--filesystem=host",
    "--share=ipc",
    "--share=network",
    "--socket=fallback-x11",
    "--socket=wayland",
    "--talk-name=org.freedesktop.Flatpak",
}

ALLOWED_DISPOSITIONS = {
    "retain-pending-supported-workstation",
    "retain-required-capability",
    "retain-required-inherited-host-agent",
    "highest-priority-minimization-candidate",
}


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> int:
    prod = load_json(PROD_MANIFEST)
    dev = load_json(DEV_MANIFEST)
    review = load_json(REVIEW_PATH)

    assert prod["app-id"] == "com.goreecloud.Terminal"
    assert dev["app-id"] == "com.goreecloud.Terminal.Devel"
    assert prod["runtime"] == dev["runtime"] == "org.gnome.Platform"
    assert prod["runtime-version"] == dev["runtime-version"] == "50"

    prod_permissions = prod["finish-args"]
    dev_permissions = dev["finish-args"]
    assert len(prod_permissions) == len(set(prod_permissions)), "duplicate production finish-arg"
    assert len(dev_permissions) == len(set(dev_permissions)), "duplicate development finish-arg"
    assert set(prod_permissions) == set(dev_permissions), "production/development permission drift"
    assert set(prod_permissions) == EXPECTED_PERMISSIONS, "manifest permission set changed without review"

    assert review["schema_version"] == 1
    assert review["product"] == "GoreeCloud Terminal"
    assert review["release_candidate"] == "50.2-rc.1"
    assert review["application_ids"] == {
        "production": "com.goreecloud.Terminal",
        "development": "com.goreecloud.Terminal.Devel",
    }
    assert review["permission_set_minimized"] is False
    assert review["production_approved"] is False
    assert review["stable_approved"] is False

    contract_permissions = review["current_permissions"]
    assert len(contract_permissions) == len(set(contract_permissions)), "duplicate contract permission"
    assert set(contract_permissions) == EXPECTED_PERMISSIONS, "contract permission set drift"

    records = review["permissions"]
    assert isinstance(records, list) and records
    names = [record["permission"] for record in records]
    assert len(names) == len(set(names)), "duplicate per-permission review record"
    assert set(names) == EXPECTED_PERMISSIONS, "permission records must cover the complete manifest set"

    by_name = {record["permission"]: record for record in records}
    for permission, record in by_name.items():
        assert record["disposition"] in ALLOWED_DISPOSITIONS, f"unknown disposition for {permission}"
        assert isinstance(record.get("reason"), str) and record["reason"].strip(), f"missing reason for {permission}"
        checks = record.get("required_acceptance")
        assert isinstance(checks, list) and checks, f"missing workstation acceptance requirements for {permission}"
        assert all(isinstance(check, str) and check.strip() for check in checks)

    assert by_name["--filesystem=host"]["disposition"] == "highest-priority-minimization-candidate"
    assert by_name["--talk-name=org.freedesktop.Flatpak"]["disposition"] == "retain-required-inherited-host-agent"
    assert by_name["--share=network"]["disposition"] == "retain-required-capability"
    assert by_name["--socket=wayland"]["disposition"] == "retain-required-capability"

    completion_rule = review["completion_rule"]
    assert "supported workstation" in completion_rule.lower()
    assert "source review and ci alone cannot" in completion_rule.lower()

    privacy_rule = review["privacy_rule"].lower()
    for marker in ("terminal contents", "private ssh configuration", "credentials", "private keys", "tokens"):
        assert marker in privacy_rule

    docs = DOC_PATH.read_text(encoding="utf-8")
    for marker in (
        "Permission minimization is not complete",
        "--filesystem=host",
        "--talk-name=org.freedesktop.Flatpak",
        "one permission change at a time",
        "production_approved` and `stable_approved` remain `false`",
    ):
        assert marker in docs, f"missing documentation marker: {marker}"

    print("GoreeCloud Terminal Flatpak permission review contract validated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
