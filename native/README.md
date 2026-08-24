# GoreeCloud Terminal Native Foundation

This directory begins the original GoreeCloud-owned GoreeCloud Terminal implementation required by the platform-wide native-application mandate.

## Boundary

The native implementation is intentionally isolated from the inherited Ptyxis product source. Ptyxis remains a temporary migration, compatibility, behavioral-reference, testing, and historical source while native capability replaces inherited product code.

The native foundation may use narrowly scoped mature platform libraries where independently replacing them would increase terminal, rendering, accessibility, operating-system, standards, or interoperability risk. The initial foundation uses GTK 4 and VTE as supporting libraries; it does not import Ptyxis application architecture, UI, workflows, branding, or general application logic.

## Current slice

The first slice provides:

- an original GTK application entry point;
- a GoreeCloud-owned application identity reserved for development isolation;
- an embedded VTE terminal surface;
- default-shell spawning through VTE;
- a basic accessible terminal-session label; and
- an isolated Meson build definition with strict compiler warnings.

This is Development source only. It is not a replacement package, release candidate, production deployment, or Stable implementation.

## Required next slices

The native implementation must add GoreeCloud-owned session/tab/window architecture, profiles, SSH workflows, context detection, Glaze UI presentation, Wardveil Security state, Privacy Shield controls, Everkeep continuity/recovery surfaces, settings persistence, migration from the transitional application, packaging, accessibility validation, and supported-workstation acceptance before it can replace the transitional Ptyxis-derived line.
