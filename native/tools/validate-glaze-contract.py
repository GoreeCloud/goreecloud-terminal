#!/usr/bin/env python3
"""Validate GoreeCloud Terminal's native Glaze UI adoption metadata.

This validator checks repository-local synchronization only. It does not claim
rendered, assistive-technology, physical-device, or production acceptance.
"""

from __future__ import annotations

import json
from pathlib import Path

NATIVE = Path(__file__).resolve().parents[1]
MANIFEST = NATIVE / "data" / "glaze-ui-manifest.json"
HEADER = NATIVE / "src" / "glaze-contract.h"
DOC = NATIVE / "GLAZE_UI_13.md"
CSS = NATIVE / "data" / "glaze-ui.css"
MAIN = NATIVE / "src" / "main.c"
THEME_ENGINE = NATIVE / "src" / "theme-engine.c"

EXPECTED_VERSION = "1.3.0"
EXPECTED_TAG = "v1.3.0"
EXPECTED_REVISION = "ff34f232f295c9dcb07e4c681f66d4104d0b9323"
EXPECTED_PRODUCT = "GLAZE UI V1.3 — Adaptive Resonance"
EXPECTED_APPEARANCE_MODES = ["follow-system", "light", "dark", "deep-dark"]
EXPECTED_CONTEXT_ACTIONS = ["copy", "paste", "select-all", "clear", "new-session", "close-session"]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"Glaze contract validation failed: {message}")


def main() -> None:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    glaze = manifest["glazeUi"]
    mapping = manifest["nativeMapping"]
    evidence = manifest["evidence"]

    require(manifest["integrationLifecycle"] == "development", "native mapping must remain Development")
    require(glaze["productLabel"] == EXPECTED_PRODUCT, "product label drift")
    require(glaze["version"] == EXPECTED_VERSION, "Glaze version drift")
    require(glaze["requiredConsumerVersion"] == EXPECTED_VERSION, "required consumer version drift")
    require(glaze["tag"] == EXPECTED_TAG, "Glaze tag drift")
    require(glaze["sourceRevision"] == EXPECTED_REVISION, "Glaze source revision drift")
    require(glaze["personalizationContract"] == "contracts/v1.3/personalization.candidate.json", "personalization contract drift")
    require(mapping["terminalEmulationAuthority"] == "VTE", "VTE emulation authority must be explicit")
    require(mapping["terminalContentStyledByGlaze"] is False, "Glaze must not style terminal content")
    require(mapping["formFactor"] == "desktop-pointer-keyboard", "desktop form-factor mapping drift")
    require(mapping["desktopInteractiveDensityPolicy"] == "compact-form-factor-aware", "desktop density policy drift")
    require(mapping["touchOrientedReferenceTargetPx"] == 48, "48px touch-oriented reference required")
    require(mapping["touchAssistanceMinimumInteractiveTargetPx"] == 56, "56px Touch Assistance floor required")
    require(mapping["appearanceModes"] == EXPECTED_APPEARANCE_MODES, "Terminal theme surface drift")
    require(mapping["themeEngine"]["owner"] == "GoreeCloud Terminal", "Theme Engine ownership drift")
    require(mapping["themeEngine"]["semanticAnsiPaletteOverride"] is False, "Theme Engine must not silently remap ANSI semantics")
    require(mapping["contextMenu"]["actions"] == EXPECTED_CONTEXT_ACTIONS, "context-menu action drift")
    require(mapping["contextMenu"]["sudoAptUpdateExposed"] is False, "sudo apt update must not be exposed")
    require(mapping["contextMenu"]["clearExecutesShellCommand"] is False, "Clear must not execute a shell command")
    require(mapping["customMotion"] is False, "native mapping must not claim custom motion")
    require(evidence["productionEligible"] is False, "Development mapping cannot claim production eligibility")

    header = HEADER.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    css = CSS.read_text(encoding="utf-8")
    main_source = MAIN.read_text(encoding="utf-8")
    theme_source = THEME_ENGINE.read_text(encoding="utf-8")

    for value in (EXPECTED_VERSION, EXPECTED_TAG, EXPECTED_REVISION, EXPECTED_PRODUCT):
        require(value in header or value in doc, f"missing synchronized identity value: {value}")

    require("GOREE_TERMINAL_GLAZE_VERSION \"1.3.0\"" in header, "header version macro drift")
    require("GOREE_TERMINAL_GLAZE_TOUCH_REFERENCE_TARGET_PX 48" in header, "header touch reference drift")
    require("GOREE_TERMINAL_GLAZE_TOUCH_ASSISTANCE_TARGET_PX 56" in header, "header Touch Assistance floor drift")
    require("min-width: 56px" in css and "min-height: 56px" in css, "CSS 56px Touch Assistance floor missing")
    require("min-width: 48px" not in css and "min-height: 48px" not in css,
            "desktop CSS must not force the touch-oriented 48px reference floor")
    require("desktop" in css.lower() and "pointer/keyboard" in css.lower(),
            "desktop density boundary must be documented in CSS")
    require("vte-terminal" not in css.lower(), "Glaze CSS must not select the VTE terminal surface")
    require("deep-dark" in theme_source, "deep-dark Theme Engine mode missing")
    require('g_menu_append(edit, "Clear", "terminal.clear")' in main_source, "Clear context action missing")
    require("sudo apt update" not in main_source.lower(), "forbidden package-management context action present")
    require('vte_terminal_feed(VTE_TERMINAL(user_data), "\\033[2J\\033[H", -1)' in main_source,
            "Clear must use terminal display control rather than a shell command")

    stale_files = [HEADER, DOC, CSS, NATIVE / "README.md", THEME_ENGINE]
    for path in stale_files:
        text = path.read_text(encoding="utf-8")
        require("Glaze UI 2.1" not in text and "2.1.0" not in text, f"stale 2.1 target in {path.name}")
        require("Glaze UI 1.5" not in text and "1.5.0" not in text, f"stale 1.5 target in {path.name}")

    print("Glaze UI 1.3.0 repository-local contract validation passed.")


if __name__ == "__main__":
    main()
