# Native Glaze UI 1.3 Integration

## Status

**Lifecycle:** Development  
**Shared design-system target:** GLAZE UI V1.3 — Adaptive Resonance / `1.3.0`  
**Canonical tag:** `v1.3.0`  
**Canonical tag commit:** `ff34f232f295c9dcb07e4c681f66d4104d0b9323`  
**Terminal issue:** #73

This document defines the repository-local native mapping for GoreeCloud Terminal. It does not promote the native Terminal implementation, replace the transitional release line, or establish production acceptance.

## Architecture boundary

GoreeCloud owns the application chrome, session presentation, accessibility semantics, appearance controls, and product identity around the terminal surface. VTE remains the terminal-emulation and terminal-rendering authority.

Glaze UI must not recolor, inspect, reinterpret, log, or otherwise take ownership of shell output, ANSI colors, terminal palettes, cursor rendering, selection, command history, credentials, private hosts, or raw process arguments.

The design-system layer is therefore intentionally split into:

- **application chrome:** GoreeCloud/Glaze-owned header, actions, tabs, focus treatment, application appearance, and supported session-state presentation;
- **terminal content:** VTE-owned rendering and interaction surface.

## V1.3 contract mapping

The native mapping follows the current Stable shared Glaze target and records its exact authority in `data/glaze-ui-manifest.json` and `src/glaze-contract.h`.

Implemented repository-local contract points include:

- semantic GTK theme roles instead of local hard-coded light/dark color palettes;
- System, Light, and Dark application appearance choices;
- a 48px general interactive-target floor;
- a 56px Touch Assistance target floor for the reserved platform adapter state;
- visible keyboard-focus treatment;
- stronger non-color boundaries and solid application chrome for detected high-contrast themes;
- no custom motion or animation that could bypass reduced-motion behavior;
- explicit accessible names for native controls;
- explicit local-session and exited-session presentation backed by runtime state; and
- no Glaze selector targeting the VTE terminal content widget.

## Appearance behavior

The current GTK-native adapter snapshots the platform preference for dark application themes when a Terminal window is created. System mode restores that baseline, while Light and Dark request the corresponding GTK application theme preference. Styling continues to use semantic GTK theme roles so Terminal does not invent an independent hard-coded palette.

High-contrast detection is based on the active GTK theme identity and strengthens application-chrome boundaries while removing translucent header treatment. A portable reduced-transparency platform preference adapter is not yet verified; the corresponding CSS class is reserved but is not automatically asserted.

## Keyboard and focus contract

The native layer provides deterministic application-level shortcuts for common session operations while preserving normal VTE keyboard input for terminal content:

- `Ctrl+Shift+T` — open a new local terminal session.
- `Ctrl+Shift+W` — close the current terminal session.

Appearance remains reachable through normal keyboard focus traversal and an explicitly named button. No additional global shortcut is taken for appearance switching.

## Runtime state truth

The native implementation may only present session states supported by runtime evidence.

Currently supported presentation truth:

- local session;
- running local session; and
- exited local session with preserved output.

Not yet represented as implemented:

- SSH/remote session identity;
- disconnected remote state;
- elevated-context warning;
- Everkeep-restored/recovery state;
- Wardveil Security state; and
- Privacy Shield authorization state.

Those remain future evidence-backed extension points and must not be shown as decorative completed integrations.

## Validation boundary

Repository-local validation includes the native build, session lifecycle tests, Glaze contract tests, and `tools/validate-glaze-contract.py` drift checks.

Still required before any claim of Glaze consumer acceptance or production readiness:

- rendered Linux acceptance across System/Light/Dark behavior;
- keyboard and focus traversal review;
- high-contrast review;
- large-text and reflow review;
- representative VTE rendering review;
- assistive-technology validation for the supported matrix;
- verified reduced-transparency platform integration if claimed;
- physical-device/native-platform acceptance where required; and
- normal GoreeCloud release and lifecycle approval.

Passing source CI is necessary but does not by itself establish Stable status, consumer conformance, workstation replacement, or production eligibility.
