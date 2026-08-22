# GoreeCloud Terminal

GoreeCloud Terminal is the GoreeCloud-maintained Linux terminal application built from the open-source Ptyxis foundation.

It preserves Ptyxis's mature GTK, VTE, PTY, container, accessibility, session, and process-tracking capabilities while adding GoreeCloud-owned product identity, Glaze UI, Wardveil Security context presentation, official GoreeCloud artwork, and GoreeCloud-specific terminal workflows.

> **Status:** Active development. Source and CI validation are not equivalent to Stable or production acceptance.

## Development model

GoreeCloud Terminal is a maintained open-source fork, not a clean-room rewrite.

- GoreeCloud repository: `GoreeCloud/goreecloud-terminal`
- Canonical upstream: `https://gitlab.gnome.org/chergert/ptyxis`
- Imported upstream baseline: `c1ba62b71295f569e0fc144b25770f2315b30e00`
- Imported upstream project version: `50.2`
- License: GPL-3.0-or-later

The repository preserves upstream copyright, contributor, translator, licensing, and source-history obligations. GoreeCloud-specific work is layered on top of that foundation through reviewed branches and pull requests.

See `GORECLOUD_FORK.md` for the maintained-fork governance and upstream synchronization model.

## GoreeCloud product identity

The canonical GoreeCloud runtime identities are:

- Production application ID: `com.goreecloud.Terminal`
- Development application ID: `com.goreecloud.Terminal.Devel`
- Production GSettings namespace: `com.goreecloud.Terminal`
- Development GSettings namespace: `com.goreecloud.Terminal.Devel`
- Canonical command-line launcher: `goreecloud-terminal`

The inherited runtime executable remains `ptyxis` and the helper remains `ptyxis-agent` as compatibility-sensitive implementation details. The installed `goreecloud-terminal` launcher delegates ordinary arguments to `ptyxis`, giving GoreeCloud Terminal a first-party CLI without prematurely breaking upstream-sensitive launch, helper, packaging, or rollback behavior.

A later packaging migration may remove or rename compatibility entry points only after launch, D-Bus, host-helper, container, upgrade, and rollback behavior are validated.

See `docs/product-identity.md` for the complete identity and migration contract.

## Command-line usage

After installation, the preferred user-facing command is:

```bash
goreecloud-terminal
```

Existing compatibility workflows may continue to use:

```bash
ptyxis
```

Both currently reach the same inherited native runtime. New GoreeCloud documentation and desktop actions should prefer `goreecloud-terminal` unless a compatibility test specifically requires the inherited executable name.

### Standard OpenSSH workflows

Milestone 4 includes optional first-party launch conveniences that still use the system OpenSSH client and the user's normal OpenSSH configuration.

Open an SSH session in a new GoreeCloud Terminal window:

```bash
goreecloud-terminal ssh server-alias
```

Open an SSH session in a new tab:

```bash
goreecloud-terminal ssh-tab server-alias
```

Arguments after the GoreeCloud subcommand are passed to `ssh` unchanged, so normal OpenSSH ordering applies. For example:

```bash
goreecloud-terminal ssh -p 2222 server-alias
```

`server-alias` may be an ordinary hostname, address, `user@hostname`, or a `Host` alias from `~/.ssh/config`.

GoreeCloud Terminal does not store SSH passwords or private keys and does not replace OpenSSH host configuration, host-key verification, authentication, forwarding, proxy, or policy behavior.

### Host profiles and workspaces

GoreeCloud Terminal can optionally organize OpenSSH `Host` aliases using a private local metadata file with three TAB-separated fields:

```text
WORKSPACE<TAB>PROFILE<TAB>SSH_HOST_ALIAS
```

The metadata layer stores organization only. It does not duplicate hostnames, credentials, key paths, ports, proxy settings, or authentication policy from OpenSSH configuration.

List workspaces and profiles:

```bash
goreecloud-terminal workspaces
goreecloud-terminal profiles
goreecloud-terminal profiles Infrastructure
```

Launch a configured profile:

```bash
goreecloud-terminal profile primary-vps
goreecloud-terminal profile-tab primary-vps
```

Malformed metadata, duplicate profile IDs, option-like aliases, unknown profiles/workspaces, and missing configuration fail before the terminal runtime starts.

See `docs/administration-workflows.md` and `docs/host-profiles-and-workspaces.md`.

## GoreeCloud layers

### Glaze UI

GoreeCloud Terminal includes a Glaze UI application-chrome foundation covering headers, controls, popovers, search surfaces, palette presentation, focus-visible behavior, and terminal-adjacent UI while preserving VTE ownership of terminal colors and rendering.

See `docs/glaze-ui.md`.

### Wardveil Security context

The application includes a typed GoreeCloud session-context model for Local, Remote, Container, and Elevated terminal states. Verified sensitive contexts can be presented through accessible Glaze UI context chips without treating context detection as authorization or as proof that a session is "protected."

Wardveil context presentation does not store credentials, bypass `sudo`, intercept commands, or replace operating-system authorization.

See `docs/wardveil-session-context.md` and `docs/wardveil-runtime-acceptance.md`.

### Terminal context-menu workflows

The current development stack includes GoreeCloud-specific context-menu actions for full-buffer copy behavior and a convenience action that sends the literal `sudo apt update -y` command to the active terminal. The normal shell and `sudo` authentication boundary remains authoritative.

See `docs/context-menu-actions.md`.

## Upstream capabilities retained

GoreeCloud Terminal continues to inherit and maintain major Ptyxis capabilities including:

- GTK 4 and libadwaita desktop integration;
- VTE-based terminal rendering;
- Podman, Toolbox, Distrobox, and related container workflows;
- configurable profiles and keyboard shortcuts;
- palette management and light/dark support;
- tabs, tab overview, pinned tabs, and saved sessions;
- foreground-process tracking for remote and elevated contexts;
- transparent terminal backgrounds;
- terminal inspection tools;
- out-of-process `ptyxis-agent` PTY/helper architecture;
- accessibility support inherited from GTK and VTE.

## Build and test

The repository's GitHub Actions foundation uses Fedora Rawhide for native validation. A representative local development build is:

```bash
meson setup _build \
  --buildtype=debugoptimized \
  --prefix=/usr \
  -Ddevelopment=true

meson compile -C _build
meson test -C _build --print-errorlogs
```

To validate a staged install without installing onto the host:

```bash
DESTDIR="$PWD/_install" meson install -C _build
```

The development build should generate GoreeCloud application, D-Bus, AppStream, icon, and GSettings artifacts under the `com.goreecloud.Terminal.Devel` identity, install the `goreecloud-terminal` launcher, and retain the current compatibility executable/helper pair.

## Security and privacy boundaries

Terminal software is inherently security-sensitive because it launches shells, executes commands, interacts with credentials, enters remote systems, and can operate with elevated privileges.

GoreeCloud-specific features therefore follow these boundaries:

- no reusable credentials, private keys, tokens, or passwords are committed to source;
- OpenSSH remains responsible for SSH configuration and authentication;
- host profiles/workspaces remain non-secret organizational metadata and do not grant access;
- Wardveil context is informational and must not become an authorization mechanism;
- `sudo` remains responsible for privilege authentication;
- terminal content and clipboard behavior are not treated as telemetry;
- source validation must be kept distinct from runtime and production acceptance;
- inherited upstream security and compatibility fixes are reviewed through controlled synchronization rather than merged blindly.

## Validation and release boundary

A green source build is only one acceptance layer. Stable or production approval additionally requires supported-workstation validation for:

- desktop and `goreecloud-terminal` command-line launch;
- compatibility `ptyxis` launch;
- D-Bus activation;
- application identity and icon resolution;
- settings persistence and any settings migration;
- terminal rendering and input;
- clipboard and selection behavior;
- direct and profile-based SSH behavior;
- workspace/profile metadata behavior and permissions;
- container and elevated-session behavior;
- accessibility;
- package installation and removal;
- upgrade and rollback;
- Glaze UI light/dark presentation;
- Wardveil runtime context transitions.

Until those gates are recorded, development branches and pull requests remain acceptance candidates rather than Stable releases.

## Upstream attribution

Ptyxis was created and is maintained upstream by Christian Hergert and contributors through GNOME GitLab. GoreeCloud Terminal builds on that project under the GPL-3.0-or-later licensing boundary.

Canonical upstream source: `https://gitlab.gnome.org/chergert/ptyxis`

The upstream project remains the authoritative source for Ptyxis history. GoreeCloud's repository is authoritative only for GoreeCloud Terminal-specific maintenance and divergence.

## License

This repository is licensed under GPL-3.0-or-later. The GPL text is preserved in `COPYING`.
