# Native Glaze UI 1.5 Integration

## Purpose

This slice maps GoreeCloud Terminal's original native GTK/VTE shell to the current Stable Glaze UI 1.5.0 contract while preserving VTE as the terminal-rendering authority.

## Stable source boundary

- Glaze UI version: `1.5.0`
- Stable promotion revision: `2e1618397f6ebcdd254a76bfdd7e98846f2c5aa3`
- Glaze Motion: not consumed; its current lifecycle is Experimental.

## Native mapping

The native shell now owns Glaze-aligned header, action, tab, selected-session, exited-session, focus, material/depth, and System/Light/Dark appearance presentation. High-contrast system themes receive stronger chrome boundaries. The application defines no custom animation, so system/reduced-animation settings are not bypassed.

VTE remains outside the Glaze application-chrome stylesheet. Terminal palette, ANSI colors, cursor, selection, and shell output are not recolored or inspected by this layer.

## Privacy and platform boundaries

This implementation does not add telemetry, analytics, remote fonts, remote icons, command inspection, terminal-content logging, credential handling, private-host collection, or process-argument capture. Wardveil Security, Privacy Shield, Everkeep, SSH/profile, and recovery integrations remain separate evidence-backed extension points.

## Acceptance boundary

Source/build and deterministic appearance-contract tests are required before merge. Real rendered Linux acceptance must still cover System/Light/Dark, high contrast, keyboard/focus behavior, reduced-motion settings, responsive sizing, screen-reader semantics, and representative VTE rendering before any current Glaze conformance or production claim.
