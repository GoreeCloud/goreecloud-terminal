#!/usr/bin/env python3
"""Collect privacy-safe GoreeCloud Terminal workstation acceptance evidence.

This tool is intentionally read-only. It does not install, remove, update, roll back,
connect to SSH destinations, modify settings, or change release lifecycle state.
"""

from __future__ import annotations

import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Iterable

PRODUCT = "GoreeCloud Terminal"
RELEASE_TAG = "50.2-rc.1"
RELEASE_SOURCE_SHA = "ae5d10511a2aed4a504b189165e268a1e9c1040f"
APP_ID = "com.goreecloud.Terminal"
EXPECTED_RUNTIME_APP = "org.gnome.Platform"
EXPECTED_RUNTIME_BRANCH = "50"
EXPECTED_BUNDLE = (
    "goreecloud-terminal-50.2-rc.1-"
    "ae5d10511a2aed4a504b189165e268a1e9c1040f.flatpak"
)
EXPECTED_BUNDLE_SHA256 = "f6819b045319babfbde7e7cfb4c097ed441bc160727c513fd2432283d962cd64"
EXPECTED_OSTREE_COMMIT = "b381917f3ba700c3b1b903423f8241fa80b640bdebdfebb438f47212559f8af7"

MANUAL_CHECKS = (
    "desktop_launch_identity",
    "dbus_single_instance",
    "new_window_tab_preferences",
    "local_shell_pty",
    "working_directory_behavior",
    "rendering_unicode_fonts",
    "tabs_session_lifecycle",
    "keyboard_shortcuts",
    "selection_clipboard",
    "context_menu_actions",
    "sudo_workflows",
    "glaze_light_dark",
    "glaze_palette_transparency",
    "glaze_focus_accessibility",
    "wardveil_detector_transitions",
    "wardveil_mixed_tabs",
    "openssh_controlled_host",
    "openssh_failure_disconnect",
    "settings_persistence",
    "settings_migration_rollback",
    "flatpak_permission_minimization",
    "upstream_ptyxis_coexistence",
    "package_install_remove_reinstall",
    "crash_recovery",
)
VALID_MANUAL_STATUSES = {"pass", "fail", "pending"}


class AcceptanceError(RuntimeError):
    pass


def run(args: list[str], *, allow_failure: bool = False) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(args, text=True, capture_output=True, check=False)
    if result.returncode and not allow_failure:
        raise AcceptanceError(f"required command failed: {args[0]} {args[1] if len(args) > 1 else ''}".strip())
    return result


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_os_release() -> dict[str, str]:
    values: dict[str, str] = {}
    try:
        for line in Path("/etc/os-release").read_text(encoding="utf-8").splitlines():
            if "=" not in line:
                continue
            key, value = line.split("=", 1)
            if key in {"ID", "VERSION_ID"}:
                values[key] = value.strip().strip('"')
    except OSError:
        pass
    return {
        "id": values.get("ID", "unknown"),
        "version_id": values.get("VERSION_ID", "unknown"),
    }


def session_type() -> str:
    value = os.environ.get("XDG_SESSION_TYPE", "").strip().lower()
    return value if value in {"wayland", "x11", "tty"} else "other"


def detect_flatpak_scope(flatpak: str) -> str:
    installed: list[str] = []
    for scope in ("user", "system"):
        result = run([flatpak, f"--{scope}", "info", APP_ID], allow_failure=True)
        if result.returncode == 0:
            installed.append(scope)
    if not installed:
        raise AcceptanceError(f"{APP_ID} is not installed in user or system Flatpak scope")
    if len(installed) != 1:
        raise AcceptanceError(f"{APP_ID} is installed in both user and system Flatpak scopes")
    return installed[0]


def flatpak_info(flatpak: str, scope: str, option: str) -> str:
    result = run([flatpak, f"--{scope}", "info", option, APP_ID])
    return result.stdout.strip()


def split_metadata_list(value: str) -> set[str]:
    return {item for item in (part.strip() for part in value.split(";")) if item}


def permission_observations(raw: str) -> dict[str, object]:
    parser = configparser.ConfigParser(interpolation=None, strict=False)
    parser.optionxform = str
    try:
        parser.read_string(raw)
    except configparser.Error as exc:
        raise AcceptanceError("could not parse Flatpak permission metadata") from exc

    context = parser["Context"] if parser.has_section("Context") else {}
    shared = split_metadata_list(context.get("shared", ""))
    sockets = split_metadata_list(context.get("sockets", ""))
    devices = split_metadata_list(context.get("devices", ""))
    features = split_metadata_list(context.get("features", ""))
    filesystems = split_metadata_list(context.get("filesystems", ""))
    session_bus = parser["Session Bus Policy"] if parser.has_section("Session Bus Policy") else {}

    return {
        "metadata_sha256": hashlib.sha256(raw.encode("utf-8")).hexdigest(),
        "network_shared": "network" in shared,
        "ipc_shared": "ipc" in shared,
        "x11_socket": "x11" in sockets or "fallback-x11" in sockets,
        "wayland_socket": "wayland" in sockets,
        "dri_device": "dri" in devices,
        "devel_feature": "devel" in features,
        "host_filesystem": "host" in filesystems,
        "flatpak_session_bus_talk": session_bus.get("org.freedesktop.Flatpak", "") == "talk",
    }


def load_manual_status(path: Path | None) -> dict[str, str]:
    statuses = {check: "pending" for check in MANUAL_CHECKS}
    if path is None:
        return statuses

    try:
        obj = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise AcceptanceError("manual-status file is unreadable or invalid JSON") from exc

    if not isinstance(obj, dict) or set(obj) != {"schema_version", "checks"}:
        raise AcceptanceError("manual-status file must contain only schema_version and checks")
    if obj["schema_version"] != 1 or not isinstance(obj["checks"], dict):
        raise AcceptanceError("manual-status schema is invalid")

    unknown = set(obj["checks"]) - set(MANUAL_CHECKS)
    if unknown:
        raise AcceptanceError("manual-status file contains unknown check identifiers")

    for key, value in obj["checks"].items():
        if value not in VALID_MANUAL_STATUSES:
            raise AcceptanceError("manual-status values must be pass, fail, or pending")
        statuses[key] = value
    return statuses


def verify_bundle(path: Path | None) -> dict[str, object]:
    if path is None:
        return {"provided": False, "verified": False}
    if not path.is_file():
        raise AcceptanceError("release bundle path is not a regular file")
    if path.name != EXPECTED_BUNDLE:
        raise AcceptanceError("release bundle filename does not match the published RC artifact")
    actual = sha256_file(path)
    if actual != EXPECTED_BUNDLE_SHA256:
        raise AcceptanceError("release bundle SHA-256 does not match the published RC artifact")
    return {
        "provided": True,
        "verified": True,
        "filename": EXPECTED_BUNDLE,
        "sha256": actual,
    }


def validate_api(raw: str) -> dict[str, object]:
    try:
        obj = json.loads(raw)
    except json.JSONDecodeError as exc:
        raise AcceptanceError("GoreeCloud Terminal local API did not return valid JSON") from exc
    try:
        assert obj["release_lifecycle"] == "Release Candidate"
        assert obj["release_candidate"] == RELEASE_TAG
        assert obj["identity"]["production_application_id"] == APP_ID
        assert obj["api"]["read_only"] is True
        assert obj["privacy"]["terminal_content_included"] is False
        assert obj["privacy"]["credentials_included"] is False
    except (AssertionError, KeyError, TypeError) as exc:
        raise AcceptanceError("GoreeCloud Terminal local API contract does not match the published RC") from exc
    return {
        "contract_valid": True,
        "read_only": True,
        "terminal_content_included": False,
        "credentials_included": False,
    }


def write_evidence(path: Path | None, evidence: dict[str, object]) -> None:
    payload = json.dumps(evidence, indent=2, sort_keys=True) + "\n"
    if path is None:
        sys.stdout.write(payload)
        return
    path.write_text(payload, encoding="utf-8")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bundle", type=Path, help="exact published 50.2-rc.1 Flatpak bundle to verify")
    parser.add_argument("--manual-status", type=Path, help="fixed status-only manual acceptance JSON")
    parser.add_argument("--output", type=Path, help="write sanitized JSON evidence to this path")
    parser.add_argument(
        "--require-complete",
        action="store_true",
        help="exit nonzero unless exact bundle and every required workstation manual check pass",
    )
    return parser


def main(argv: Iterable[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    flatpak = shutil.which("flatpak")
    if not flatpak:
        raise AcceptanceError("flatpak is required")

    scope = detect_flatpak_scope(flatpak)
    runtime = flatpak_info(flatpak, scope, "--show-runtime")
    runtime_parts = runtime.split("/")
    if len(runtime_parts) != 3 or runtime_parts[0] != EXPECTED_RUNTIME_APP or runtime_parts[2] != EXPECTED_RUNTIME_BRANCH:
        raise AcceptanceError("installed runtime does not match org.gnome.Platform branch 50")

    installed_commit = flatpak_info(flatpak, scope, "--show-commit")
    if installed_commit != EXPECTED_OSTREE_COMMIT:
        raise AcceptanceError("installed OSTree commit does not match the published 50.2-rc.1 artifact")

    installed_ref = flatpak_info(flatpak, scope, "--show-ref")
    if not installed_ref.startswith(f"app/{APP_ID}/"):
        raise AcceptanceError("installed Flatpak ref does not match the production application ID")

    version = run([flatpak, "run", f"--{scope}", "--command=goreecloud-terminal", APP_ID, "--version"]).stdout
    if PRODUCT not in version:
        raise AcceptanceError("installed launcher version output does not identify GoreeCloud Terminal")

    api_raw = run([flatpak, "run", f"--{scope}", "--command=goreecloud-terminal", APP_ID, "api", "status"]).stdout
    api = validate_api(api_raw)

    permissions_raw = flatpak_info(flatpak, scope, "--show-permissions")
    permissions = permission_observations(permissions_raw)
    bundle = verify_bundle(args.bundle)
    manual = load_manual_status(args.manual_status)
    manual_complete = all(value == "pass" for value in manual.values())
    workstation_complete = bool(bundle["verified"] and manual_complete)

    evidence = {
        "schema_version": 1,
        "product": PRODUCT,
        "release_lifecycle": "Release Candidate",
        "release_tag": RELEASE_TAG,
        "release_source_sha": RELEASE_SOURCE_SHA,
        "application_id": APP_ID,
        "expected_published_bundle": EXPECTED_BUNDLE,
        "expected_published_bundle_sha256": EXPECTED_BUNDLE_SHA256,
        "expected_ostree_commit": EXPECTED_OSTREE_COMMIT,
        "workstation": {
            "os": read_os_release(),
            "session_type": session_type(),
        },
        "machine_checks": {
            "flatpak_scope": scope,
            "installed_ref": installed_ref,
            "runtime": runtime,
            "installed_ostree_commit": installed_commit,
            "version_reports_product": True,
            "local_api": api,
            "permissions": permissions,
        },
        "published_bundle": bundle,
        "manual_checks": manual,
        "manual_checks_complete": manual_complete,
        "workstation_acceptance_complete": workstation_complete,
        "production_approved": False,
        "stable_approved": False,
        "remaining_stable_blockers_even_if_workstation_complete": [
            "second distinct package and repository-backed update/rollback acceptance",
            "data compatibility after rollback",
        ],
        "privacy": {
            "terminal_content_collected": False,
            "shell_history_collected": False,
            "username_collected": False,
            "hostname_collected": False,
            "ssh_configuration_collected": False,
            "credentials_collected": False,
            "free_form_notes_collected": False,
        },
    }
    write_evidence(args.output, evidence)

    if args.require_complete and not workstation_complete:
        return 2
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AcceptanceError as exc:
        print(f"workstation acceptance error: {exc}", file=sys.stderr)
        raise SystemExit(1)
