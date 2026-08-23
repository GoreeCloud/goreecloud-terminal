# GoreeCloud Terminal

GoreeCloud Terminal is the GoreeCloud-maintained Linux terminal application built from the open-source Ptyxis foundation. It preserves Ptyxis's mature GTK, VTE, PTY, container, accessibility, session, and process-tracking capabilities while adding GoreeCloud-owned product identity, Glaze UI, Wardveil Security context presentation, official GoreeCloud artwork, Privacy Shield boundaries, and GoreeCloud-specific administration workflows.

> **Status: Release Candidate 50.2-rc.2.** This follow-up candidate line carries the supported-workstation-validated Wardveil AT-SPI accessibility correction and post-RC1 stabilization work. **Stable and production approval remain separate** and are explicitly false until the remaining supported-workstation gates are completed.

The authoritative source lifecycle is `release/status.json`; final acceptance requirements are documented in `docs/release-readiness.md`.

## Maintained-fork model

- GoreeCloud repository: `GoreeCloud/goreecloud-terminal`
- Canonical upstream: `https://gitlab.gnome.org/chergert/ptyxis`
- Imported upstream baseline: `c1ba62b71295f569e0fc144b25770f2315b30e00`
- Upstream foundation version: `50.2`
- GoreeCloud candidate: `50.2-rc.2`
- License: GPL-3.0-or-later

The repository preserves upstream copyright, contributor, translator, licensing, attribution, and source-history obligations. GoreeCloud-specific work is layered through controlled branches and pull requests. See `GORECLOUD_FORK.md`.

## RC2 stabilization highlights

- Carries the revised Wardveil semantic-label accessibility implementation that passed live supported-workstation AT-SPI exact-name checks for Remote, Container, and Elevated detector-driven states before merge.
- Preserves Local/unknown behavior without a misleading Wardveil context indicator.
- Includes a fail-closed production no-host-filesystem candidate generator that permits exactly one temporary manifest delta: removal of `--filesystem=host`.
- Includes real repository-backed Flatpak update and exact rollback acceptance tooling using immutable `50.2-rc.1` as the rollback baseline.
- Keeps the canonical production and development Flatpak manifests unchanged while supported-workstation permission-minimization acceptance remains open.
- Keeps `production_approved=false` and `stable_approved=false`.

The published `50.2-rc.1` tag, prerelease, source, bundle, checksum, and OSTree identity remain immutable historical release evidence.

## Product identity

- Production application ID: `com.goreecloud.Terminal`
- Development application ID: `com.goreecloud.Terminal.Devel`
- Production GSettings namespace: `com.goreecloud.Terminal`
- Development GSettings namespace: `com.goreecloud.Terminal.Devel`
- Canonical launcher: `goreecloud-terminal`
- Compatibility runtime: `ptyxis`
- Compatibility helper: `ptyxis-agent`

The inherited executable/helper remain compatibility-sensitive implementation details. New GoreeCloud workflows should use `goreecloud-terminal`.

## Local API

GoreeCloud Terminal provides a deliberately narrow, built-in read-only local API:

```bash
goreecloud-terminal api status
```

It returns schema-versioned JSON with static product, release, identity, Privacy Shield, and Wardveil integration metadata. It opens no network listener and exposes no terminal content, commands, credentials, SSH destinations, profile aliases, or user/device identifiers.

See `docs/local-api.md`.

## Standard OpenSSH workflows

OpenSSH remains the authority for host configuration, host keys, authentication, private keys, agents, ports, proxying, forwarding, and connection policy.

```bash
goreecloud-terminal ssh server-alias
goreecloud-terminal ssh -p 2222 server-alias
goreecloud-terminal ssh-tab server-alias
```

Arguments after the GoreeCloud subcommand are passed to the system `ssh` command unchanged using normal OpenSSH argument ordering.

## Host profiles and workspaces

Optional local metadata may organize OpenSSH `Host` aliases using exactly three TAB-separated fields:

```text
WORKSPACE<TAB>PROFILE<TAB>SSH_HOST_ALIAS
```

```bash
goreecloud-terminal workspaces
goreecloud-terminal profiles
goreecloud-terminal profiles Infrastructure
goreecloud-terminal profile primary-vps
goreecloud-terminal profile-tab primary-vps
```

This metadata does not duplicate credentials, key paths, ports, proxy settings, hostnames, or authentication policy. Malformed configuration fails before the terminal runtime starts. See `docs/administration-workflows.md` and `docs/host-profiles-and-workspaces.md`.

## GoreeCloud platform layers

### Glaze UI 1.4

The RC targets **Glaze UI 1.4.0 Stable** through a native GTK/libadwaita mapping. Glaze styles terminal-adjacent application chrome while VTE retains ownership of terminal glyph rendering and palette behavior. The RC adds explicit light-scheme, reduced-motion, increased-contrast, focus-visible, and custom target-size source invariants.

See `docs/glaze-ui.md`.

### Wardveil Security

The typed session-context model supports Local, Remote, Container, and Elevated presentation. Wardveil context is informational and does not replace `sudo`, OpenSSH, shell policy, or operating-system authorization. The RC2 source line contains the corrected semantic accessible-label implementation that passed live Remote, Container, and Elevated exact-name validation before integration; the exact RC2 package must preserve that behavior in follow-up acceptance.

See `docs/wardveil-session-context.md`, `docs/wardveil-runtime-acceptance.md`, and `docs/wardveil-atspi-accessibility-fix.md`.

### Privacy Shield

The RC adopts a minimized Privacy Shield adapter with only:

- `telemetry-minimization`
- `data-minimization`

GoreeCloud Terminal adds no analytics or remote tracker telemetry. Release evidence excludes terminal contents, command history, clipboard contents, credentials, raw SSH configuration, private infrastructure inventory, and similar sensitive material.

See `privacy-shield/adapter.json` and `docs/privacy-shield-integration.md`.

## Packaging

### Development acceptance package

`com.goreecloud.Terminal.Devel.json` remains the isolated development package and lifecycle-test identity. Its exact-artifact lifecycle harness verifies hashes, application/runtime identity, packaged smoke behavior, data-preserving local-bundle replacement, and clean ordinary removal without `--delete-data`.

See `docs/flatpak-packaging-and-acceptance.md` and `docs/flatpak-upgrade-and-rollback.md`.

### Production-identity RC package

`com.goreecloud.Terminal.json` is the production-identity Release Candidate manifest. It uses:

- `com.goreecloud.Terminal`;
- GNOME Platform/SDK 50;
- canonical `goreecloud-terminal` command;
- `-Ddevelopment=false`;
- the same pinned support dependencies as the accepted development package;
- explicit terminal-oriented permissions that remain subject to Stable permission-minimization acceptance.

The canonical RC2 manifest intentionally retains the existing host-filesystem permission at this source-preparation boundary. The no-host-filesystem build remains an isolated acceptance candidate until the affected supported-workstation behavior is verified.

The `Production RC Flatpak Acceptance` workflow builds and smoke-tests an exact-head RC2 bundle. A successful RC package workflow is not Stable authorization.

## Build and test

Representative native development validation:

```bash
meson setup _build \
  --buildtype=debugoptimized \
  --prefix=/usr \
  -Ddevelopment=true

meson compile -C _build
meson test -C _build --print-errorlogs
DESTDIR="$PWD/_install" meson install -C _build
```

The RC source adds automated gates for maintained-fork provenance, lifecycle safety, Privacy Shield, the local API, Glaze UI source invariants, Wardveil accessibility, supported-workstation evidence contracts, Flatpak permission review, development Flatpak acceptance, production-identity RC Flatpak acceptance, isolated no-host-filesystem candidate builds, and repository-backed transition/rollback validation.

## Security and privacy boundaries

Terminal software is security-sensitive. GoreeCloud-specific features therefore preserve these boundaries:

- no reusable credentials, private keys, tokens, or passwords are committed to source;
- OpenSSH remains responsible for SSH configuration/authentication;
- `sudo` and the operating system remain responsible for privilege authorization;
- host profiles remain non-secret organizational metadata and do not grant access;
- Wardveil context does not authorize commands or sessions;
- Privacy Shield prohibits GoreeCloud telemetry of private terminal/session activity;
- the local API is read-only, local, and privacy-minimized;
- Flatpak permissions remain explicit and reviewable;
- package/release evidence is limited to non-secret source/package identity and pass/fail data;
- upstream security and compatibility fixes are reviewed through controlled synchronization.

## RC and Stable boundary

Release Candidate `50.2-rc.2` may be used for controlled final acceptance. Stable promotion still requires recorded supported-workstation evidence for remaining applicable items including:

- exact RC2 regression coverage for the corrected Wardveil accessible names and accepted core terminal behavior;
- explicit Glaze UI light/dark palette, transparency, contrast, and reduced-motion behavior;
- settings persistence and controlled migration/rollback;
- final Flatpak permission minimization against affected real workflows;
- production package installation/removal/reinstall and intended upstream Ptyxis coexistence;
- a second distinct accepted package with repository-backed update and exact rollback;
- post-rollback data compatibility;
- crash and recovery behavior.

No green CI result alone may promote the candidate to Stable.

## Upstream attribution

Ptyxis was created and is maintained upstream by Christian Hergert and contributors through GNOME GitLab. GoreeCloud Terminal builds on that project under GPL-3.0-or-later.

Canonical upstream source: `https://gitlab.gnome.org/chergert/ptyxis`

## License

GPL-3.0-or-later. The GPL text remains in `COPYING`.
