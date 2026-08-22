#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
adapter = json.loads((ROOT / "privacy-shield/adapter.json").read_text(encoding="utf-8"))

expected_top = {"schema_version", "adapter", "capabilities", "privacy", "acceptance"}
assert set(adapter) == expected_top, "unexpected Privacy Shield adapter keys"
assert adapter["schema_version"] == 1
assert adapter["adapter"] == {
    "id": "terminal",
    "product": "GoreeCloud Terminal",
    "runtime_authority": "GoreeCloud/goreecloud-terminal",
    "contract_version": 1,
}
assert adapter["capabilities"] == ["telemetry-minimization", "data-minimization"]
assert adapter["privacy"] == {
    "local_first": True,
    "raw_private_activity_exported_for_status": False,
    "remote_tracker_learning": False,
    "remote_tracker_telemetry": False,
}
assert adapter["acceptance"] == {
    "runtime_acceptance_required": True,
    "production_approved": False,
}

doc = (ROOT / "docs/privacy-shield-integration.md").read_text(encoding="utf-8")
for marker in (
    "telemetry-minimization",
    "data-minimization",
    "terminal scrollback and terminal contents",
    "OpenSSH remains authoritative",
    "production_approved` is false",
):
    assert marker in doc, f"missing Privacy Shield documentation marker: {marker}"

print("GoreeCloud Terminal Privacy Shield adapter validated")
