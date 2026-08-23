# GoreeCloud Terminal Fork Foundation

## Status

GoreeCloud Terminal is a GoreeCloud-maintained open-source fork of Ptyxis. The active source line is **Release Candidate 50.2-rc.2** for controlled follow-up acceptance. Stable and production approval remain separate and false until the remaining supported-workstation requirements are completed.

The published `50.2-rc.1` release remains immutable historical evidence. RC2 is a separately versioned source/package line carrying the validated Wardveil accessibility correction and post-RC1 stabilization work.

## Canonical upstream

- Project: Ptyxis
- Authoritative upstream repository: https://gitlab.gnome.org/chergert/ptyxis
- Upstream hosting authority: GNOME GitLab
- GoreeCloud repository: https://github.com/GoreeCloud/goreecloud-terminal
- Initial imported upstream head: `c1ba62b71295f569e0fc144b25770f2315b30e00`
- Imported/upstream foundation version: `50.2`
- GoreeCloud candidate: `50.2-rc.2`

GitHub mirrors and third-party forks are not authoritative upstream sources.

## Remote model

Local development clones should retain:

```text
origin   git@github.com:GoreeCloud/goreecloud-terminal.git
upstream https://gitlab.gnome.org/chergert/ptyxis.git
```

`origin` is GoreeCloud-controlled. `upstream` is the canonical Ptyxis source for provenance, release comparison, security review, and controlled synchronization.

## Licensing and attribution

The imported source preserves GPL-3.0-or-later licensing, copyright, contributor, translator, and source-history obligations. GoreeCloud branding does not authorize removal of required upstream attribution.

## Development lifecycle

```text
GNOME Ptyxis upstream
        ↓
GoreeCloud-maintained fork
        ↓
GoreeCloud Terminal
        ↓
Selective fork-to-native evolution only when justified
```

Terminal-emulation, PTY, accessibility, compatibility, and security-sensitive components are not rewritten merely for branding or ownership.

## Branch model

- `main`: controlled GoreeCloud integration branch.
- `release/terminal-50.2-rc.2`: governed follow-up Release Candidate integration branch.
- `agent/fork-foundation`: Milestone 0 governance and CI.
- `agent/terminal-artwork`: official artwork.
- `agent/glaze-ui-foundation`: Glaze UI application chrome.
- `agent/wardveil-session-context`: typed Wardveil context presentation.
- `agent/terminal-context-menu-actions`: GoreeCloud terminal context-menu workflows.
- `agent/product-identity-foundation`: canonical application identity and CLI.
- `agent/runtime-identity-acceptance`: staged/installed identity validation.
- `agent/settings-migration-rollback`: fail-closed settings migration and rollback.
- `agent/ssh-launch-workflows`: standard OpenSSH launch conveniences.
- `agent/host-profiles-workspaces`: non-secret OpenSSH-alias organization.
- `agent/flatpak-packaging-acceptance`: development Flatpak build/package acceptance.
- `agent/flatpak-lifecycle-acceptance`: exact-artifact lifecycle and rollback logic.
- `agent/release-candidate-readiness`: production-identity RC lifecycle, API, Privacy Shield, Glaze 1.4, and final source-readiness gates.

The project uses narrowly scoped branches and pull requests for review and validation. Direct GoreeCloud feature work does not occur on an upstream-tracking branch.

## Upstream synchronization

Upstream changes are reviewed rather than merged blindly. Review includes security fixes, PTY/VTE/process tracking, GTK/libadwaita/accessibility, dependencies, identity/packaging, privacy/network/history behavior, and branding/translations affected by GoreeCloud divergence.

## Milestones

### Milestone 0 — Fork Foundation

Upstream history/licensing/provenance and CI validation are established. Source integration remains controlled through governed history.

### Milestone 1 — Product Identity

Implemented source state includes official GoreeCloud artwork, production ID `com.goreecloud.Terminal`, development ID `com.goreecloud.Terminal.Devel`, GoreeCloud GSettings namespaces, desktop/AppStream metadata, canonical `goreecloud-terminal`, and preserved compatibility `ptyxis`/`ptyxis-agent` names where renaming would create risk.

### Milestone 2 — Glaze UI

The RC targets Glaze UI 1.4.0 Stable through a native GTK/libadwaita mapping. Terminal canvas rendering remains controlled by VTE. Source includes explicit light, reduced-motion, increased-contrast, focus-visible, and target-size invariants. Explicit supported-workstation palette/transparency, contrast, and reduced-motion acceptance remains open where applicable.

See `docs/glaze-ui.md`.

### Milestone 3 — Wardveil Security

Typed Local, Remote, Container, and Elevated context presentation is implemented. Wardveil remains informational and is not authorization. The revised semantic-label implementation passed live supported-workstation exact AT-SPI accessible-name validation for Remote, Container, and Elevated under their corresponding real detector-driven states before it was integrated to authoritative source. RC2 must preserve that behavior on its exact package; Local/unknown must remain free of a misleading context indicator.

### Milestone 4 — GoreeCloud Administration Workflows

`ssh`/`ssh-tab` launch the standard system OpenSSH client. Optional profiles store only a workspace label, unique profile ID, and OpenSSH `Host` alias. OpenSSH remains authoritative for actual host/user/port/key/agent/proxy/forwarding/authentication policy. Malformed profile metadata fails before runtime launch, and recent-destination persistence remains intentionally absent.

See `docs/administration-workflows.md` and `docs/host-profiles-and-workspaces.md`.

### Milestone 5 — Packaging and Acceptance

Current source state includes:

- development Flatpak manifest `com.goreecloud.Terminal.Devel.json` using GNOME Platform/SDK 50;
- production-identity RC manifest `com.goreecloud.Terminal.json` using `com.goreecloud.Terminal`, `goreecloud-terminal`, and `-Ddevelopment=false`;
- pinned external Flatpak support dependencies and disabled implicit Meson dependency downloads;
- explicit terminal-oriented permissions that remain subject to final minimization review;
- exact-head development package build/install/smoke/removal evidence;
- exact-artifact lifecycle validation with explicit SHA-256 checks;
- data-preserving local-bundle replacement that matches observed Flatpak 1.14.6 behavior;
- a read-only local API (`goreecloud-terminal api status`) that reports only static non-secret status metadata;
- a minimized Privacy Shield adapter declaring only `telemetry-minimization` and `data-minimization`;
- machine-readable `release/status.json` lifecycle state and explicit Stable blockers;
- dedicated source-readiness, Wardveil accessibility, supported-workstation contract, Flatpak permission, development-package, and production-identity RC package workflows;
- an isolated production no-host-filesystem candidate generator that permits exactly one temporary manifest delta: removal of `--filesystem=host`;
- repository-backed production transition acceptance that verifies immutable RC1 baseline identity, updates to a cryptographically distinct generated candidate, confirms host-filesystem access is absent, rolls back to the exact published RC1 OSTree commit, and preserves synthetic application data through update, rollback, and ordinary removal.

The canonical production and development manifests remain unchanged at the RC2 source-preparation boundary. The no-host-filesystem candidate cannot become the canonical permission set until affected supported-workstation workflows pass.

Repository-backed transition CI is real package-lifecycle evidence, but selected-release cross-version acceptance still requires applicable exact-package supported-workstation data-compatibility and recovery checks.

See `docs/flatpak-packaging-and-acceptance.md`, `docs/flatpak-upgrade-and-rollback.md`, and `docs/release-readiness.md`.

### Milestone 6 — Selective Native Evolution

No broad rewrite is authorized merely for branding. Mature inherited components remain unless a controlled evaluation documents a material GoreeCloud benefit and preserves compatibility/security.

## Privacy and API boundaries

Privacy Shield governs GoreeCloud-owned privacy minimization; it does not replace Wardveil, OpenSSH, the shell, or OS authorization. Release evidence must not contain terminal contents, commands, clipboard data, credentials, raw private SSH configuration, or private infrastructure inventory.

The local API is read-only local CLI JSON. It does not open a listener or expose session/profile/host contents.

## Compatibility boundary

`ptyxis`, `ptyxis-agent`, the inherited gettext domain, and internal filenames remain compatibility-sensitive implementation details. Any later rename requires explicit launch, D-Bus, host-helper, container, package, update, and rollback acceptance.

## Release boundary

Release Candidate 50.2-rc.2 is a separately versioned follow-up candidate. It does not alter or supersede the immutable historical identity of published 50.2-rc.1.

CI success alone cannot establish Stable or production approval. RC2 publication requires exact candidate package identity, applicable supported-workstation regression evidence, governed tag/prerelease creation, and independent post-publication verification.

Stable still requires applicable remaining supported-workstation Glaze UI, settings, permission, production-package, selected-package repository-backed upgrade/rollback, post-rollback data-compatibility, and crash/recovery evidence.

The authoritative source lifecycle is `release/status.json`.
