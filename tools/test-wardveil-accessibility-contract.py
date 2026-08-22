#!/usr/bin/env python3
"""Validate the source-level Wardveil accessibility contract.

This is intentionally a source contract, not a substitute for supported-workstation
AT-SPI runtime acceptance.
"""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WINDOW_DRESSING = ROOT / "src" / "ptyxis-window-dressing.c"
SESSION_CONTEXT = ROOT / "src" / "goreecloud-session-context.c"
DOC = ROOT / "docs" / "wardveil-atspi-accessibility-fix.md"


def require(text: str, needle: str, description: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {description}: {needle!r}")


def require_count(text: str, needle: str, expected: int, description: str) -> None:
    actual = text.count(needle)
    if actual != expected:
        raise SystemExit(
            f"unexpected {description} count: expected {expected}, found {actual}: {needle!r}"
        )


def main() -> None:
    window = WINDOW_DRESSING.read_text(encoding="utf-8")
    context = SESSION_CONTEXT.read_text(encoding="utf-8")
    doc = DOC.read_text(encoding="utf-8")

    require_count(
        window,
        '"accessible-role", GTK_ACCESSIBLE_ROLE_GROUP',
        1,
        "semantic group role on Wardveil session-context chip",
    )
    require_count(
        window,
        '"accessible-role", GTK_ACCESSIBLE_ROLE_NONE',
        2,
        "non-semantic decorative child roles",
    )
    require(
        window,
        "GTK_ACCESSIBLE_PROPERTY_LABEL,\n                                  context->accessible_description",
        "accessible label mapping",
    )
    require(
        window,
        "gtk_widget_set_tooltip_text (self->session_context_chip,\n                               context->accessible_description)",
        "tooltip mapping",
    )
    require(
        window,
        "gtk_widget_set_visible (self->session_context_chip, context->show_indicator)",
        "detector-driven visibility mapping",
    )

    for exact in (
        "Remote terminal session",
        "Containerized terminal session",
        "Elevated superuser terminal session",
    ):
        require(context, exact, f"exact accessible description {exact}")
        require(doc, exact, f"documented runtime acceptance target {exact}")

    require(
        doc,
        "The published 50.2-rc.1 tag and bundle remain immutable.",
        "immutable Release Candidate boundary",
    )

    print("Wardveil accessibility source contract: PASS")


if __name__ == "__main__":
    main()
