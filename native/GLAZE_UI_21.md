# Native Glaze UI 2.1 Integration

## Purpose

This slice maps GoreeCloud Terminal's original native GTK/VTE shell to **Glaze UI 2.1.0 Stable** while preserving VTE as the terminal-rendering authority.

## Stable source boundary

- Glaze UI version: `2.1.0`
- Canonical `v2.1.0` source revision: `c49113eb8b93c267613fdf1bbca1f814495acad7`
- Historical 1.x/2.0 mappings are migration and rollback context only.
- Glaze Motion experimentation is not required by this Stable native mapping.

## Native mapping

**Content is solid. Interaction is glazed.**

The native shell owns Glaze-aligned header, action, tab, selected-session, exited-session, focus, and System/Light/Dark appearance presentation. Persistent application chrome uses a restrained Soft Glaze treatment; the terminal content plane remains the native VTE surface.

The general interactive target floor is **48px**. The native contract also defines the **56px Touch Assistance floor**, with a `glaze-touch-assistance` adapter class for host/product activation. High-contrast system themes strengthen chrome boundaries. A reduced-transparency adapter state removes translucent chrome without changing task meaning.

VTE remains outside the Glaze application-chrome stylesheet. Terminal palette, ANSI colors, cursor, selection, shell output, terminal text scaling, and terminal rendering behavior are not recolored, inspected, or reinterpreted by this layer.

## Appearance and accessibility

System → Light → Dark appearance cycling remains explicit and keyboard-accessible. The appearance action has a changing accessible label that describes both current state and next activation result. Focus indicators remain visible, and current-session/exited-session states are not color-only.

The application defines no custom animation, so platform reduced-animation behavior is not bypassed. Current native acceptance must still cover large text, keyboard/focus traversal, screen-reader semantics, high contrast, reduced transparency, supported window sizes, and representative VTE content.

## Privacy and platform boundaries

This implementation adds no telemetry, analytics, remote fonts, remote icons, command inspection, terminal-content logging, credential handling, private-host collection, or process-argument capture. Wardveil Security, Privacy Shield, Everkeep, SSH/profile, and recovery integrations remain separate evidence-backed extension points.

Presentation classes and appearance state do not create or strengthen security/privacy/recovery claims.

## Acceptance boundary

Source/build and deterministic contract tests are required before merge. Real rendered Linux acceptance remains separate and must cover System/Light/Dark, high contrast, keyboard/focus behavior, reduced-motion settings, reduced transparency, large text, responsive sizing, screen-reader semantics, 48px general targets, 56px Touch Assistance behavior where enabled, and representative VTE rendering.

Passing source CI does not by itself establish current Glaze application conformance, release acceptance, production readiness, or Terminal Stable lifecycle status.