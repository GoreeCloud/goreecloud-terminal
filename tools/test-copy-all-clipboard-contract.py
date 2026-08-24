#!/usr/bin/env python3
"""Validate GoreeCloud Terminal's full-buffer Copy All clipboard contract."""

from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TERMINAL_SOURCE = ROOT / "src" / "ptyxis-terminal.c"


def trim_trailing_blank_lines(text: str) -> str:
    """Reference model for the C Copy All normalization semantics."""
    lines = text.split("\n")

    while lines and all(ch.isspace() for ch in lines[-1]):
        lines.pop()

    return "\n".join(lines)


def function_body(source: str, start_marker: str, end_marker: str) -> str:
    try:
        start = source.index(start_marker)
        end = source.index(end_marker, start)
    except ValueError as exc:
        raise AssertionError(f"Unable to locate source markers: {start_marker!r}") from exc

    return source[start:end]


def test_reference_behavior() -> None:
    cases = {
        "trailing empty rows": ("alpha\n\n", "alpha"),
        "trailing whitespace-only rows": ("alpha\n   \n\t\n", "alpha"),
        "internal blank rows": ("alpha\n\nbeta\n\n", "alpha\n\nbeta"),
        "internal whitespace-only row": ("alpha\n   \nbeta\n", "alpha\n   \nbeta"),
        "meaningful trailing spaces": ("alpha  \n\n", "alpha  "),
        "unicode output": ("café ✓\n\n", "café ✓"),
        "wrapped-looking output": ("first visual row\ncontinuation row\n\n", "first visual row\ncontinuation row"),
        "only whitespace": ("\n   \n\t\n", ""),
        "already clean": ("alpha\nbeta", "alpha\nbeta"),
    }

    for name, (source, expected) in cases.items():
        actual = trim_trailing_blank_lines(source)
        assert actual == expected, f"{name}: expected {expected!r}, got {actual!r}"


def test_source_contract() -> None:
    source = TERMINAL_SOURCE.read_text(encoding="utf-8")

    selection_copy = function_body(
        source,
        "static void\ncopy_clipboard_action",
        "static void\ncopy_all_clipboard_action",
    )
    copy_all = function_body(
        source,
        "static void\ncopy_all_clipboard_action",
        "static void\nrun_apt_update_action",
    )

    required_copy_all_fragments = (
        "vte_terminal_write_contents_sync",
        'g_strsplit (text, "\\n", -1)',
        "g_ascii_isspace",
        'g_strjoinv ("\\n", lines)',
        "if (text[0] == '\\0')",
        "gdk_clipboard_set_text (clipboard, text)",
    )
    for fragment in required_copy_all_fragments:
        assert fragment in copy_all, f"Copy All contract missing source fragment: {fragment}"

    assert "vte_terminal_get_text_selected" in selection_copy, (
        "Selection copy must remain selection-scoped"
    )
    assert "g_strsplit" not in selection_copy, (
        "Trailing-row normalization must not affect ordinary selection copy"
    )
    assert "g_ascii_isspace" not in selection_copy, (
        "Whitespace trimming must remain isolated to Copy All"
    )


def main() -> None:
    test_reference_behavior()
    test_source_contract()
    print("Copy All clipboard contract: PASS")


if __name__ == "__main__":
    main()
