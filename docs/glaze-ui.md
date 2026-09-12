# GoreeCloud Terminal — Glaze UI Integration

## Current shared authority

The current Stable shared GoreeCloud design-system authority is:

- **Product:** GLAZE UI V1.3 — Adaptive Resonance
- **Version:** `1.3.0`
- **Tag:** `v1.3.0`
- **Tag commit:** `ff34f232f295c9dcb07e4c681f66d4104d0b9323`

This live shared authority controls current Terminal design-system integration. Older Terminal documents or transitional source that refer to later historical/candidate Glaze labels do not override the current Stable shared contract.

## Two implementation tracks

GoreeCloud Terminal currently contains two separate UI integration contexts.

### Original native Development implementation

The GoreeCloud-owned native implementation under `native/` is the active architecture-migration path. Its current repository-local Glaze mapping is defined by:

- `native/GLAZE_UI_13.md`
- `native/data/glaze-ui-manifest.json`
- `native/data/glaze-ui.css`
- `native/src/glaze-contract.[ch]`
- `native/tools/validate-glaze-contract.py`

The native mapping pins `1.3.0` and the exact canonical tag commit, styles GoreeCloud-owned application chrome, and leaves VTE authoritative for terminal rendering/content.

### Transitional Release Candidate implementation

The Ptyxis-derived Release Candidate line contains an older GTK/libadwaita Glaze mapping created before the current shared V1.3 authority was established. That implementation remains part of the transitional RC source and its historical validation record, but its former version label must not be represented as current shared-design-system conformance.

The transitional line remains subject to its own rendered appearance and Stable acceptance gates in `release/status.json`.

## Native semantic mapping

The current original-native mapping uses these boundaries:

- **Application canvas/chrome:** GoreeCloud-owned GTK widgets using semantic theme roles.
- **Interaction surfaces:** restrained Glaze presentation for the header, actions, tabs, focus state, and truthful session-state chrome.
- **Solid accessibility fallback:** high-contrast application chrome removes translucent treatment and strengthens non-color boundaries.
- **Focus:** visible keyboard focus independent of decorative material effects.
- **Targets:** 48px minimum general interactive target and 56px Touch Assistance floor as specified by the current Glaze accessibility contract.
- **Motion:** no custom application motion is currently introduced by the native mapping.
- **Runtime state:** only states backed by native runtime evidence may be presented.

## Terminal-content boundary

VTE owns terminal glyph rendering, ANSI colors, active palettes, cursor rendering, selection, terminal input/output semantics, and the terminal canvas.

Glaze UI must not override terminal palette choices, recolor shell output, inspect terminal contents, log terminal contents, or reinterpret terminal semantics for decorative presentation.

This boundary protects terminal correctness, ANSI meaning, user-selected themes, privacy, and VTE accessibility behavior.

## Appearance

The native Development implementation provides System, Light, and Dark application appearance choices using GTK semantic theme behavior rather than maintaining a duplicated local color palette.

High-contrast GTK themes trigger stronger application-chrome boundaries and solid header treatment. A portable reduced-transparency preference adapter has not yet been verified for the supported Linux matrix; the native CSS reserves the state but does not automatically claim it.

## Accessibility contract

The native Glaze layer must preserve:

- keyboard navigation and logical focus order;
- visible focus treatment;
- accessible names, roles, state, and relationships where applicable;
- 48px general interactive-target minimums;
- 56px Touch Assistance target minimums when that adapter is active;
- non-color state/boundary signals;
- high-contrast legibility;
- terminal-input preservation;
- task and focus continuity when presentation adapts; and
- separation between generated/product presentation and actual runtime truth.

Repository-local automated tests establish source-contract evidence only. They do **not** establish screen-reader acceptance, physical-device acceptance, human optical review, native-platform parity, production performance acceptance, or Stable status.

## Runtime truth and extension points

The original native implementation currently has evidence-backed local/running and local/exited session presentation.

The following remain unimplemented or pending in the native path and must not be shown as completed Glaze state merely for visual completeness:

- SSH/remote identity;
- disconnected remote state;
- elevated-context state;
- Wardveil Security state;
- Privacy Shield authorization state;
- Everkeep restored/recovery state; and
- verified reduced-transparency platform adaptation.

Each future state requires its own runtime contract and validation evidence.

## Validation

Current repository-local validation for the original native mapping includes:

```bash
python3 native/tools/validate-glaze-contract.py
meson setup native/_build native --buildtype=debugoptimized
meson compile -C native/_build
meson test -C native/_build --print-errorlogs
```

The `Native Foundation Contract` GitHub workflow enforces the same source/build/test boundary on exact PR heads.

Before any claim of native Glaze consumer acceptance or production eligibility, GoreeCloud Terminal still requires the applicable rendered Linux review, keyboard/focus review, high-contrast review, large-text/reflow review, representative VTE content review, assistive-technology validation, native-platform/physical-device evidence where required, and normal GoreeCloud release approval.

## Lifecycle boundary

Glaze source integration does not promote either Terminal implementation track to Stable.

The transitional package remains Release Candidate `50.2-rc.2` with `stable_approved=false`. The original native implementation remains Development source until its independent migration, packaging, accessibility, platform-system, supported-workstation, and release gates are satisfied.
