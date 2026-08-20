# GoreeCloud Terminal — Glaze UI Foundation

This document records the first Glaze UI integration layer for GoreeCloud Terminal.

## Scope

The initial Glaze UI layer is intentionally limited to application chrome and supporting interface surfaces. It does not override the VTE terminal canvas palette, shell text colors, ANSI colors, or user-selected terminal palettes.

The foundation covers:

- application header-bar treatment;
- buttons and active/hover states;
- popovers and menus;
- search/find interface;
- preference cards and palette selection surfaces;
- focus visibility;
- drag-and-drop terminal boundary feedback;
- consistent rounded geometry and subtle glass-like edge treatment.

## Design principles

GoreeCloud Terminal must remain a terminal first. Glaze UI presentation must not reduce terminal readability, interfere with shell output, weaken accessibility, or make privileged and remote-session state harder to recognize.

The interface therefore applies Glaze UI primarily to surrounding application chrome while leaving terminal content under the control of VTE and the active terminal palette.

## Foundation tokens

The first implementation uses the following source-level visual tokens in `src/style.css`:

- `glaze_accent` — primary GoreeCloud interactive accent;
- `glaze_accent_soft` — low-emphasis interactive surface;
- `glaze_edge` — subtle glass/surface boundary;
- `glaze_surface` — translucent application surface;
- `glaze_surface_strong` — stronger elevated surface;
- `glaze_text_soft` — secondary text treatment.

These are implementation tokens rather than a replacement for platform accessibility behavior. Native GTK/libadwaita state handling remains authoritative where appropriate.

## Accessibility boundary

Focus states are intentionally stronger than decorative borders. User terminal palettes remain untouched. Destructive and suggested actions retain libadwaita semantic styling rather than being recolored as ordinary GoreeCloud controls.

## Next integration work

The next Glaze UI phases should cover tab presentation, preferences information architecture, dialogs, empty states, session-status presentation, and Wardveil Security states for SSH, elevated privileges, and other security-sensitive contexts.
