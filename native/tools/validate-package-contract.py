#!/usr/bin/env python3
"""Validate the format-neutral GoreeCloud Terminal host-native package contract."""

from __future__ import annotations

import argparse
import json
import stat
import sys
from pathlib import Path, PurePosixPath


EXPECTED_POLICY = {
    "automatic_migration": False,
    "automatic_partial_migration": False,
    "automatic_transitional_state_deletion": False,
    "automatic_user_state_deletion": False,
    "automatic_host_agent_enablement": False,
    "automatic_host_agent_start": False,
}

EXPECTED_EVIDENCE = {
    "staged_payload_validation_is_production_acceptance": False,
    "staged_payload_validation_is_stable_acceptance": False,
    "package_manager_acceptance_required": True,
    "physical_workstation_acceptance_required": True,
    "exact_artifact_verification_required": True,
    "everkeep_recovery_acceptance_required": True,
}

EXPECTED_SANDBOX_EXCLUSIONS = {
    "/app/bin/goreecloud-terminal-migrate",
    "/app/share/glib-2.0/schemas/com.goreecloud.Terminal.Migration.gschema.xml",
    "/app/libexec/goreecloud-terminal-host-agent",
    "/app/lib/systemd/user/goreecloud-terminal-host-agent.service",
}

EXPECTED_PRESERVED_STATE = {
    "$XDG_CONFIG_HOME/goreecloud/terminal",
    "$XDG_STATE_HOME/goreecloud/terminal",
    "transitional GSettings source state",
}


def die(message: str) -> None:
    print(f"package-contract: {message}", file=sys.stderr)
    raise SystemExit(1)


def require(condition: bool, message: str) -> None:
    if not condition:
        die(message)


def load_contract(path: Path) -> dict:
    try:
        with path.open("r", encoding="utf-8") as handle:
            value = json.load(handle)
    except (OSError, json.JSONDecodeError) as exc:
        die(f"cannot read {path}: {exc}")
    require(isinstance(value, dict), "top-level contract must be a JSON object")
    return value


def validate_contract(contract: dict, identity: str) -> tuple[str, list[dict]]:
    require(contract.get("schema_version") == 1, "schema_version must be 1")
    require(contract.get("status") == "development", "contract status must remain development")
    require(contract.get("tracking_issue") == 97, "tracking_issue must remain GitHub issue #97")

    package_format = contract.get("package_format")
    require(isinstance(package_format, dict), "package_format must be an object")
    require(package_format.get("status") == "unselected", "distro package format must remain unselected until governed")
    require(package_format.get("selected_formats") == [], "selected_formats must remain empty until governed")
    rule = package_format.get("rule")
    require(isinstance(rule, str) and "Do not infer" in rule, "package-format non-inference rule is required")

    artifact = contract.get("artifact_identity")
    require(isinstance(artifact, dict), "artifact_identity must be an object")
    require(artifact.get("package_name") == "goreecloud-terminal", "canonical package name drifted")
    require(artifact.get("install_prefix") == "/usr", "host-native install prefix must be /usr")
    require(artifact.get("version_authority") == "native/meson.build:project.version", "version authority drifted")
    require(
        set(artifact) == {"package_name", "install_prefix", "version_authority"},
        "artifact_identity must reference version authority without duplicating the current version value",
    )

    identities = contract.get("application_identities")
    require(
        identities == {
            "development": "com.goreecloud.Terminal.Devel",
            "production": "com.goreecloud.Terminal",
        },
        "application identity contract drifted",
    )
    require(identity in identities, f"unsupported identity: {identity}")

    payload = contract.get("host_native_payload")
    require(isinstance(payload, list) and payload, "host_native_payload must be a non-empty list")

    seen_templates: set[str] = set()
    for entry in payload:
        require(isinstance(entry, dict), "each host-native payload entry must be an object")
        template = entry.get("path")
        component = entry.get("component")
        executable = entry.get("executable")
        require(isinstance(template, str), "payload path must be a string")
        require(isinstance(component, str) and component, f"payload component missing for {template!r}")
        require(isinstance(executable, bool), f"payload executable flag must be boolean for {template!r}")
        require(template not in seen_templates, f"duplicate payload template: {template}")
        seen_templates.add(template)
        require(template.startswith("/usr/"), f"host-native payload escapes /usr: {template}")
        parts = PurePosixPath(template.replace("{application_id}", "app-id")).parts
        require(".." not in parts, f"parent traversal is not allowed in payload path: {template}")
        if "{" in template or "}" in template:
            require(template.count("{application_id}") == 1, f"unsupported payload placeholder: {template}")

    require(contract.get("package_action_policy") == EXPECTED_POLICY, "package action policy must remain fail-closed")
    require(contract.get("evidence_boundary") == EXPECTED_EVIDENCE, "package evidence boundary drifted")
    require(set(contract.get("sandbox_exclusions", [])) == EXPECTED_SANDBOX_EXCLUSIONS, "sandbox exclusion contract drifted")
    require(set(contract.get("preserved_user_state", [])) == EXPECTED_PRESERVED_STATE, "preserved user-state contract drifted")

    return identities[identity], payload


def collect_stage_files(stage: Path) -> set[str]:
    require(stage.is_dir(), f"staging root does not exist: {stage}")
    files: set[str] = set()
    for path in stage.rglob("*"):
        if path.is_file() or path.is_symlink():
            relative = path.relative_to(stage).as_posix()
            files.add("/" + relative)
    return files


def validate_stage(stage: Path, application_id: str, payload: list[dict]) -> None:
    expected: dict[str, bool] = {}
    for entry in payload:
        path = entry["path"].replace("{application_id}", application_id)
        require(path not in expected, f"resolved payload path is duplicated: {path}")
        expected[path] = entry["executable"]

    actual = collect_stage_files(stage)
    missing = sorted(set(expected) - actual)
    unexpected = sorted(actual - set(expected))
    if missing or unexpected:
        if missing:
            print("package-contract: missing staged files:", file=sys.stderr)
            for item in missing:
                print(f"  {item}", file=sys.stderr)
        if unexpected:
            print("package-contract: unexpected staged files:", file=sys.stderr)
            for item in unexpected:
                print(f"  {item}", file=sys.stderr)
        raise SystemExit(1)

    for absolute, executable in expected.items():
        path = stage / absolute.lstrip("/")
        mode = path.stat().st_mode
        has_exec_bit = bool(mode & (stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH))
        if executable:
            require(has_exec_bit, f"required executable has no execute bit: {absolute}")
        else:
            require(stat.S_ISREG(mode), f"required non-executable payload is not a regular file: {absolute}")

    require(not any("Terminal.Native" in path for path in actual), "obsolete .Native identity returned in staged payload")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("contract", type=Path)
    parser.add_argument("stage", type=Path)
    parser.add_argument("identity", choices=("development", "production"))
    args = parser.parse_args()

    contract = load_contract(args.contract)
    application_id, payload = validate_contract(contract, args.identity)
    validate_stage(args.stage.resolve(), application_id, payload)

    print(
        f"package-contract: PASS identity={args.identity} "
        f"application_id={application_id} files={len(payload)} format=unselected"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
