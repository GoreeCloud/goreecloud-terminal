# GoreeCloud Terminal Native Foundation

This directory contains the original GoreeCloud-owned GoreeCloud Terminal implementation required by the platform-wide native-application mandate.

## Boundary

The native implementation is intentionally isolated from the inherited Ptyxis product source. Ptyxis remains a temporary migration, compatibility, behavioral-reference, testing, and historical source while native capability replaces inherited product code.

The native foundation may use narrowly scoped mature platform libraries where independently replacing them would increase terminal, rendering, accessibility, operating-system, standards, or interoperability risk. The current foundation uses GTK 4 and VTE as supporting libraries; it does not import Ptyxis application architecture, UI, workflows, branding, or general application logic.

VTE remains the terminal rendering authority. GoreeCloud owns the surrounding product chrome, session state, application actions, accessibility semantics, and Glaze UI mapping.

## Current Development slice

The verified source on this branch provides:

- an original GTK application entry point and GoreeCloud-owned development identity;
- an embedded VTE terminal surface with local default-shell spawning;
- GoreeCloud-owned local session, tab, close, and window presentation;
- explicit local-running and local-exited session labels backed by runtime state;
- Glaze UI V1.3 / `1.3.0` application-chrome mapping pinned to the canonical Stable tag commit;
- semantic GTK theme roles rather than a duplicated hard-coded light/dark palette;
- System, Light, and Dark application appearance controls;
- high-contrast chrome strengthening without recoloring terminal content;
- 48px general interactive targets and a reserved 56px Touch Assistance adapter state;
- visible focus treatment and accessible names for native controls;
- `Ctrl+Shift+T` for a new local session and `Ctrl+Shift+W` for closing the current session;
- no custom application motion that bypasses reduced-motion behavior;
- a machine-readable Glaze adoption manifest, contract tests, and drift validator; and
- an isolated Meson build with strict compiler warnings and native unit tests.

The exact Glaze boundary and remaining acceptance work are documented in `GLAZE_UI_13.md`.

## Explicitly unclaimed integrations

The native source does not yet claim implemented SSH/remote state, disconnected-state presentation, elevated-context warnings, Wardveil Security state, Privacy Shield authorization state, Everkeep recovery state, or verified reduced-transparency platform adaptation. Those capabilities require their own runtime contracts and evidence before UI can present them as real.

## Lifecycle

This is **Development source only**. It is not a replacement package, release candidate, production deployment, current Stable implementation, or workstation replacement.

Passing repository-local build and contract tests is necessary but does not establish rendered accessibility acceptance, assistive-technology acceptance, physical-device/native-platform acceptance, Glaze consumer conformance, or GoreeCloud release approval.

## Required next slices

The native implementation still needs profiles/workspaces, SSH and remote-session workflows, evidence-backed remote/disconnected/elevated/recovery state, Wardveil Security integration, Privacy Shield controls, Everkeep continuity/recovery, settings persistence, richer shell/context integration, split-pane workflows, migration from the transitional application, packaging, rendered accessibility validation, and supported-workstation acceptance before it can replace the transitional Ptyxis-derived line.
