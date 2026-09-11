# GoreeCloud Terminal

GoreeCloud Terminal is GoreeCloud's Linux terminal application. The repository currently contains **two explicitly separated implementation tracks**:

1. a transitional Ptyxis-derived Release Candidate line used for current packaging, compatibility, and workstation acceptance; and
2. an original GoreeCloud-owned native GTK 4/VTE implementation under `native/`, which is the active architecture-migration path.

The native implementation does not import Ptyxis application architecture, UI, workflows, branding, or general product logic. GTK 4 and VTE remain mature supporting platform libraries because replacing terminal emulation/rendering independently would increase compatibility, accessibility, and standards risk.

> **Release lifecycle:** `50.2-rc.2` remains the current transitional Release Candidate recorded by `release/status.json`. `production_approved=false` and `stable_approved=false`. The `native/` implementation is **Development source only** and is not a replacement package, Release Candidate, Stable build, or workstation replacement.

## Current development direction

GoreeCloud Terminal is moving from a maintained-fork product architecture toward an original GoreeCloud-owned application architecture while preserving mature terminal/runtime foundations where justified.

The native Development source currently provides:

- GoreeCloud-owned GTK application, window, tab, and local-session architecture;
- VTE-owned terminal emulation/rendering and local default-shell execution;
- accessible local-session and exited-session state presentation;
- multi-session tabs and independent windows;
- deterministic `Ctrl+Shift+T` new-session and `Ctrl+Shift+W` close-session actions;
- current-authority Glaze UI V1.3 / `1.3.0` application-chrome integration work;
- System, Light, and Dark application appearance behavior using GTK semantic theme roles;
- high-contrast application-chrome treatment, visible focus behavior, and Glaze interactive-target contracts;
- a machine-readable Glaze adoption manifest, repository-local drift validator, and unit tests; and
- isolated Meson build/test definitions under `native/`.

See `native/README.md`, `native/GLAZE_UI_13.md`, and `FEATURE-ROADMAP.md`.

## Architecture and rendering boundary

GoreeCloud owns the native application shell, product identity, application actions, session lifecycle, accessibility semantics, and product-level presentation.

**VTE remains authoritative for terminal content**, including glyph rendering, ANSI colors, palettes, cursor behavior, selection, and terminal input/output semantics. Glaze UI must not recolor, inspect, reinterpret, log, or take ownership of terminal content merely to create a branded appearance.

The native source currently supports truthful local/running and local/exited presentation. Remote/disconnected, elevated, Wardveil, Privacy Shield authorization, and Everkeep recovery states remain unimplemented in the native path until separate runtime contracts and evidence exist.

## Glaze UI

The current shared GoreeCloud design-system authority is **GLAZE UI V1.3 — Adaptive Resonance / `1.3.0`**. The native Terminal integration pins that target and exact release source revision in `native/data/glaze-ui-manifest.json`.

Native Glaze work is repository-local Development integration, not automatic consumer acceptance. Rendered Linux review, accessibility/assistive-technology acceptance, native-platform qualification, and normal GoreeCloud release approval remain separate gates.

The older transitional RC contains a historical native GTK/libadwaita Glaze mapping. Historical version labels in that line do not override the current shared Glaze authority and must not be interpreted as current shared-design-system conformance.

See `docs/glaze-ui.md` and `native/GLAZE_UI_13.md`.

## Transitional Release Candidate line

The current packaged/release-candidate source remains based on the Ptyxis 50.2 foundation while the native implementation is developed and qualified. This transitional line retains compatibility, packaging, rollback, supported-workstation, Wardveil, Privacy Shield, and release-readiness evidence that must not be silently transferred to the new native implementation.

Key RC facts from `release/status.json`:

- Release Candidate: `50.2-rc.2`
- Production application ID: `com.goreecloud.Terminal`
- Development package application ID: `com.goreecloud.Terminal.Devel`
- Canonical launcher: `goreecloud-terminal`
- Production approved: false
- Stable approved: false

The upstream-derived line preserves required copyright, contributor, translator, licensing, attribution, and source-history obligations. See `GORECLOUD_FORK.md`.

## Wardveil Security, Privacy Shield, and Everkeep

Platform-system state is implementation-specific and must not be inferred across the two tracks.

The transitional RC contains existing Wardveil and Privacy Shield integrations documented under `docs/` and `privacy-shield/`. Those records remain evidence for that implementation line only.

The native implementation currently declares **extension points only** for Wardveil Security, Privacy Shield, and Everkeep. It does not yet claim native security-state, privacy-authorization, or recovery-state integration. Future work must use the corresponding canonical GoreeCloud platform contracts rather than introducing Terminal-local substitutes.

## SSH, profiles, and workspaces

The transitional RC retains existing OpenSSH-based workflows and non-secret host-profile/workspace metadata. OpenSSH remains responsible for SSH configuration, host keys, authentication, private keys, agents, ports, forwarding, proxying, and connection policy.

The original native implementation does not yet claim SSH/remote-session, remote-profile, or workspace parity. Those capabilities are planned in `FEATURE-ROADMAP.md` and must be implemented with explicit local/remote identity and evidence-backed lifecycle states before the native path can replace the transitional line.

## Build and test

### Original native Development source

```bash
meson setup native/_build native --buildtype=debugoptimized
meson compile -C native/_build
meson test -C native/_build --print-errorlogs
```

The `Native Foundation Contract` workflow also validates native architecture isolation, the exact Glaze adoption metadata, the native build, session lifecycle tests, and Glaze contract tests.

### Transitional Ptyxis-derived line

Representative Development validation for the transitional source remains:

```bash
meson setup _build \
  --buildtype=debugoptimized \
  --prefix=/usr \
  -Ddevelopment=true

meson compile -C _build
meson test -C _build --print-errorlogs
DESTDIR="$PWD/_install" meson install -C _build
```

The repository includes additional Flatpak, release-readiness, rollback, accessibility, permission, and supported-workstation workflows for the transitional package lifecycle.

## Security and privacy boundaries

Terminal software is security-sensitive. GoreeCloud Terminal therefore maintains these boundaries across current and future implementation work:

- no reusable credentials, private keys, passwords, tokens, or sensitive infrastructure inventory are committed to source;
- no GoreeCloud telemetry of private terminal/session content is introduced;
- VTE terminal contents and user-selected terminal palettes remain outside Glaze UI ownership;
- OpenSSH remains the authority for SSH authentication/configuration where SSH is implemented;
- operating-system authorization remains authoritative for privilege changes;
- Wardveil presentation must not be confused with command/session authorization;
- Privacy Shield authorization must fail closed at applicable remote/network trust and time boundaries when native remote operations are implemented; and
- release/test evidence must remain privacy-minimized and must not capture terminal contents, credentials, clipboard contents, private hosts, or command history.

## Roadmap

Near-term native priorities are:

1. complete issue #73's Glaze UI native-chrome and repository-local acceptance work;
2. finish deterministic accessibility and input contracts for the native window/session layer;
3. add profiles/workspaces and settings persistence;
4. implement SSH/remote lifecycle with truthful remote/disconnected state;
5. integrate Wardveil Security, Privacy Shield, and Everkeep through their canonical versioned contracts;
6. add split panes, shell/context integration, safe clipboard/search workflows, and long-session quality controls; and
7. complete migration, packaging, rendered accessibility, supported-workstation qualification, rollback, and release gates before replacing the transitional line.

See `FEATURE-ROADMAP.md` for the controlled roadmap.

## Release and Stable boundary

Neither passing source CI nor completing a Development feature promotes GoreeCloud Terminal to Stable.

The transitional RC remains subject to the blockers in `release/status.json`. The native implementation has its own additional acceptance obligations, including repository-local feature validation, rendered Linux behavior, accessibility and assistive-technology review, platform-system integration evidence, migration/rollback validation, packaging, and supported-workstation qualification.

No acceptance evidence from the transitional implementation may be silently reused as native acceptance unless the applicable governing contract explicitly allows it and the evidence actually covers the native implementation.

## Upstream attribution and license

The transitional implementation derives from Ptyxis, created and maintained upstream by Christian Hergert and contributors through GNOME GitLab. Required upstream licensing, copyright, attribution, and source-history obligations remain preserved.

Canonical upstream source: `https://gitlab.gnome.org/chergert/ptyxis`

License: GPL-3.0-or-later. The license text remains in `COPYING`.
