# GoreeCloud Terminal Fork Foundation

## Status

GoreeCloud Terminal is a GoreeCloud-maintained open-source fork of Ptyxis. The active source line is **Release Candidate 50.2-rc.1** for controlled final acceptance. Stable and production approval remain separate and false until the remaining supported-workstation requirements are completed.

## Canonical upstream

- Project: Ptyxis
- Authoritative upstream repository: https://gitlab.gnome.org/chergert/ptyxis
- Upstream hosting authority: GNOME GitLab
- GoreeCloud repository: https://github.com/GoreeCloud/goreecloud-terminal
- Initial imported upstream head: `c1ba62b71295f569e0fc144b25770f2315b30e00`
- Imported/upstream foundation version: `50.2`
- GoreeCloud candidate: `50.2-rc.1`

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

The project uses narrowly scoped stacked branches and pull requests for review and validation. Direct GoreeCloud feature work does not occur on an upstream-tracking branch.

## Upstream synchronization

Upstream changes are reviewed rather than merged blindly. Review includes security fixes, PTY/VTE/process tracking, GTK/libadwaita/accessibility, dependencies, identity/packaging, privacy/network/history behavior, and branding/translations affected by GoreeCloud divergence.

## Milestones

### Milestone 0 — Fork Foundation

Upstream history/licensing/provenance and CI validation are established. Source integration remains controlled through the stacked history.

### Milestone 1 — Product Identity

Implemented source state includes official GoreeCloud artwork, production ID `com.goreecloud.Terminal`, development ID `com.goreecloud.Terminal.Devel`, GoreeCloud GSettings namespaces, desktop/AppStream metadata, canonical `goreecloud-terminal`, and preserved compatibility `ptyxis`/`ptyxis-agent` names where renaming would create risk.

### Milestone 2 — Glaze UI

The RC targets Glaze UI 1.4.0 Stable through a native GTK/libadwaita mapping. Terminal canvas rendering remains controlled by VTE. Source includes explicit light, reduced-motion, increased-contrast, focus-visible, and target-size invariants. Real workstation appearance/accessibility remains a Stable gate.

See `docs/glaze-ui.md`.

### Milestone 3 — Wardveil Security

Typed Local, Remote, Container, and Elevated context presentation is implemented. Wardveil remains informational and is not authorization. Real detector-driven transitions, mixed-tab presentation, palette/accessibility, and integrated workstation acceptance remain open.

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
- data-preserving local-bundle replacement that matches observed Flatpak 1.14.6 behavior: ordinary application removal without `--delete-data`, verification that the app ref is gone, installation of the exact verified bundle, exact OSTree-commit verification, and smoke validation;
- transition logic that requires cryptographically distinct artifacts and distinct installed OSTree commits before any upgrade claim, then requires exact baseline rollback;
- a read-only local API (`goreecloud-terminal api status`) that reports only static non-secret status metadata;
- a minimized Privacy Shield adapter declaring only `telemetry-minimization` and `data-minimization`;
- machine-readable `release/status.json` lifecycle state and explicit Stable blockers;
- dedicated source-readiness and production-identity RC package workflows.

The unsupported local-bundle `flatpak install --reinstall <bundle>` behavior discovered during CI is not used as a false acceptance shortcut. Repository-backed `flatpak update`, real cross-version update/rollback, data compatibility after rollback, production package workstation behavior, permission minimization, graphical runtime, and Stable authorization remain separate gates.

See `docs/flatpak-packaging-and-acceptance.md`, `docs/flatpak-upgrade-and-rollback.md`, and `docs/release-readiness.md`.

### Milestone 6 — Selective Native Evolution

No broad rewrite is authorized merely for branding. Mature inherited components remain unless a controlled evaluation documents a material GoreeCloud benefit and preserves compatibility/security.

## Privacy and API boundaries

Privacy Shield governs GoreeCloud-owned privacy minimization; it does not replace Wardveil, OpenSSH, the shell, or OS authorization. Release evidence must not contain terminal contents, commands, clipboard data, credentials, raw private SSH configuration, or private infrastructure inventory.

The local API is read-only local CLI JSON. It does not open a listener or expose session/profile/host contents.

## Compatibility boundary

`ptyxis`, `ptyxis-agent`, the inherited gettext domain, and internal filenames remain compatibility-sensitive implementation details. Any later rename requires explicit launch, D-Bus, host-helper, container, package, update, and rollback acceptance.

## Release boundary

Release Candidate 50.2-rc.1 is intended for final controlled acceptance if all exact-head RC source/package workflows pass. CI success alone cannot establish Stable or production approval.

Stable still requires applicable supported-workstation functional, security, accessibility, Glaze UI, Wardveil, settings, permission, real OpenSSH, production-package, repository-backed upgrade/rollback, data-compatibility, and recovery evidence.

The authoritative source lifecycle is `release/status.json`.
