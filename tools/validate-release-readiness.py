#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
status = json.loads((ROOT / "release/status.json").read_text(encoding="utf-8"))
prod = json.loads((ROOT / "com.goreecloud.Terminal.json").read_text(encoding="utf-8"))
dev = json.loads((ROOT / "com.goreecloud.Terminal.Devel.json").read_text(encoding="utf-8"))
privacy = json.loads((ROOT / "privacy-shield/adapter.json").read_text(encoding="utf-8"))

assert status["schema_version"] == 1
assert status["product"] == "GoreeCloud Terminal"
assert status["release_lifecycle"] == "Release Candidate"
assert status["release_candidate"] == "50.2-rc.2"
assert status["production_approved"] is False
assert status["stable_approved"] is False
assert len(status["stable_blockers"]) >= 8
assert status["identity"] == {
    "production_application_id": "com.goreecloud.Terminal",
    "development_application_id": "com.goreecloud.Terminal.Devel",
    "canonical_launcher": "goreecloud-terminal",
}

required_source_acceptance = {
    "exact_head_required",
    "fork_foundation_required",
    "flatpak_lifecycle_contract_required",
    "development_flatpak_required",
    "release_readiness_required",
    "production_identity_flatpak_required",
    "wardveil_accessibility_contract_required",
    "supported_workstation_contract_required",
    "flatpak_permission_contract_required",
    "production_no_host_filesystem_transition_required",
}
assert required_source_acceptance <= status["source_acceptance"].keys()
assert all(status["source_acceptance"][key] is True for key in required_source_acceptance)

assert prod["app-id"] == "com.goreecloud.Terminal"
assert prod["runtime"] == "org.gnome.Platform"
assert prod["runtime-version"] == "50"
assert prod["sdk"] == "org.gnome.Sdk"
assert prod["command"] == "goreecloud-terminal"
assert set(prod["finish-args"]) == set(dev["finish-args"])

prod_modules = {m["name"]: m for m in prod["modules"]}
dev_modules = {m["name"]: m for m in dev["modules"]}
required_modules = {"libportal", "fast_float", "simdutf", "vte", "goreecloud-terminal"}
assert required_modules <= prod_modules.keys()
assert required_modules <= dev_modules.keys()

for name in ("libportal", "fast_float", "simdutf", "vte"):
    assert prod_modules[name]["sources"] == dev_modules[name]["sources"], f"production/development source pin drift for {name}"

app_opts = set(prod_modules["goreecloud-terminal"]["config-opts"])
assert "--wrap-mode=nodownload" in app_opts
assert "-Ddevelopment=false" in app_opts
assert "-Ddevelopment=true" not in app_opts
assert "-Dlibc-compat=true" in app_opts
assert prod_modules["goreecloud-terminal"]["sources"] == [{"type": "dir", "path": "."}]

assert privacy["capabilities"] == ["telemetry-minimization", "data-minimization"]
assert privacy["acceptance"]["runtime_acceptance_required"] is True
assert privacy["acceptance"]["production_approved"] is False

launcher = (ROOT / "data/goreecloud-terminal").read_text(encoding="utf-8")
for marker in (
    "goreecloud-terminal api status",
    '"release_lifecycle":"Release Candidate"',
    '"release_candidate":"50.2-rc.2"',
    '"terminal_content_included":false',
    '"credentials_included":false',
):
    assert marker in launcher, f"missing local API marker: {marker}"

glaze = (ROOT / "docs/glaze-ui.md").read_text(encoding="utf-8")
assert "Glaze UI 1.4.0 Stable" in glaze
assert "883d40ff51d02885650024723c01d229de456285" in glaze

css = (ROOT / "src/style.css").read_text(encoding="utf-8")
for marker in (
    "@media (prefers-color-scheme: light)",
    "@media (prefers-reduced-motion: reduce)",
    "@media (prefers-contrast: more)",
    "min-height: 44px",
    "min-width: 44px",
    ":focus-visible",
):
    assert marker in css, f"missing Glaze source invariant: {marker}"

local_api = (ROOT / "docs/local-api.md").read_text(encoding="utf-8")
for marker in ("read-only local CLI contract", "does not open a network listener", "schema_version"):
    assert marker in local_api

readiness = (ROOT / "docs/release-readiness.md").read_text(encoding="utf-8")
assert "No automation may convert RC to Stable merely because CI is green." in readiness
assert "Production Identity RC Flatpak acceptance" in readiness
assert "50.2-rc.2" in readiness

readme = (ROOT / "README.md").read_text(encoding="utf-8")
assert "Release Candidate 50.2-rc.2" in readme
assert "Stable and production approval remain separate" in readme

notes = (ROOT / "release/50.2-rc.2.md").read_text(encoding="utf-8")
assert "# GoreeCloud Terminal 50.2-rc.2" in notes
assert "50.2-rc.1 remains immutable" in notes
assert "production_approved=false" in notes
assert "stable_approved=false" in notes

man = (ROOT / "man/goreecloud-terminal.1.in").read_text(encoding="utf-8")
assert "goreecloud-terminal api" in man
assert ".SH LOCAL API" in man

print("GoreeCloud Terminal Release Candidate source readiness validated")
