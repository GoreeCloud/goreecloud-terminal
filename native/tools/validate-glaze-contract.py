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

EXPECTED_VERSION = "1.3.0"
EXPECTED_TAG = "v1.3.0"
EXPECTED_REVISION = "ff34f232f295c9dcb07e4c681f66d4104d0b9323"
EXPECTED_PRODUCT = "GLAZE UI V1.3 — Adaptive Resonance"


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
    require(mapping["terminalSurfaceAuthority"] == "VTE", "VTE terminal-surface authority must be explicit")
    require(mapping["terminalContentStyledByGlaze"] is False, "Glaze must not style terminal content")
    require(mapping["minimumInteractiveTargetPx"] == 48, "48px target floor required")
    require(mapping["touchAssistanceMinimumInteractiveTargetPx"] == 56, "56px Touch Assistance floor required")
    require(mapping["customMotion"] is False, "native mapping must not claim custom motion")
    require(evidence["productionEligible"] is False, "Development mapping cannot claim production eligibility")

    header = HEADER.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")
    css = CSS.read_text(encoding="utf-8")

    for value in (EXPECTED_VERSION, EXPECTED_TAG, EXPECTED_REVISION, EXPECTED_PRODUCT):
        require(value in header or value in doc, f"missing synchronized identity value: {value}")

    require("GOREE_TERMINAL_GLAZE_VERSION \"1.3.0\"" in header, "header version macro drift")
    require("GOREE_TERMINAL_GLAZE_GENERAL_TARGET_PX 48" in header, "header target floor drift")
    require("GOREE_TERMINAL_GLAZE_TOUCH_ASSISTANCE_TARGET_PX 56" in header, "header Touch Assistance floor drift")
    require("min-width: 48px" in css and "min-height: 48px" in css, "CSS 48px target floor missing")
    require("min-width: 56px" in css and "min-height: 56px" in css, "CSS 56px Touch Assistance floor missing")
    require("vte-terminal" not in css.lower(), "Glaze CSS must not select the VTE terminal surface")

    stale_files = [HEADER, DOC, CSS, NATIVE / "README.md"]
    for path in stale_files:
        text = path.read_text(encoding="utf-8")
        require("Glaze UI 2.1" not in text and "2.1.0" not in text, f"stale 2.1 target in {path.name}")
        require("Glaze UI 1.5" not in text and "1.5.0" not in text, f"stale 1.5 target in {path.name}")

    print("Glaze UI 1.3.0 repository-local contract validation passed.")


if __name__ == "__main__":
    main()
