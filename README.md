# GoreeCloud Terminal

GoreeCloud Terminal is the GoreeCloud-maintained Linux terminal application built from the open-source Ptyxis foundation. It preserves Ptyxis's mature GTK, VTE, PTY, container, accessibility, session, and process-tracking capabilities while adding GoreeCloud-owned product identity, Glaze UI, Wardveil Security context presentation, official GoreeCloud artwork, Privacy Shield boundaries, and GoreeCloud-specific administration workflows.

> **Status: Release Candidate 50.2-rc.1.** This identified source/package line is intended for final acceptance. **Stable and production approval remain separate** and are explicitly false until the remaining supported-workstation gates are completed.

The authoritative source lifecycle is `release/status.json`; final acceptance requirements are documented in `docs/release-readiness.md`.

## Maintained-fork model

- GoreeCloud repository: `GoreeCloud/goreecloud-terminal`
- Canonical upstream: `https://gitlab.gnome.org/chergert/ptyxis`
- Imported upstream baseline: `c1ba62b71295f569e0fc144b25770f2315b30e00`
- Upstream foundation version: `50.2`
- GoreeCloud candidate: `50.2-rc.1`
- License: GPL-3.0-or-later

The repository preserves upstream copyright, contributor, translator, licensing, attribution, and source-history obligations. GoreeCloud-specific work is layered through controlled branches and pull requests. See `GORECLOUD_FORK.md`.

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

The typed session-context model supports Local, Remote, Container, and Elevated presentation. Wardveil context is informational and does not replace `sudo`, OpenSSH, shell policy, or operating-system authorization.

See `docs/wardveil-session-context.md` and `docs/wardveil-runtime-acceptance.md`.

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

The `Production RC Flatpak Acceptance` workflow builds and smoke-tests an exact-head RC bundle. A successful RC package workflow is not Stable authorization.

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

The RC source adds automated gates for maintained-fork provenance, lifecycle safety, Privacy Shield, the local API, Glaze UI source invariants, development Flatpak acceptance, and production-identity RC Flatpak acceptance.

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

Release Candidate `50.2-rc.1` may be used for controlled final acceptance. Stable promotion still requires recorded supported-workstation evidence for applicable items including:

- graphical launch and D-Bus activation;
- terminal input, rendering, fonts, Unicode, selection, and clipboard;
- tabs/session lifecycle and crash recovery;
- real OpenSSH host-key/authentication behavior;
- real container/elevated/remote Wardveil transitions;
- mixed-tab Wardveil presentation and accessibility;
- assistive-technology behavior;
- Glaze UI light/dark/transparency/high-contrast/reduced-motion presentation;
- settings persistence and controlled migration;
- production package installation/removal/coexistence;
- Flatpak permission minimization;
- repository-backed cross-version update and exact rollback using distinct accepted packages;
- data compatibility after rollback.

No green CI result alone may promote the candidate to Stable.

## Upstream attribution

Ptyxis was created and is maintained upstream by Christian Hergert and contributors through GNOME GitLab. GoreeCloud Terminal builds on that project under GPL-3.0-or-later.

Canonical upstream source: `https://gitlab.gnome.org/chergert/ptyxis`

## License

GPL-3.0-or-later. The GPL text remains in `COPYING`.
