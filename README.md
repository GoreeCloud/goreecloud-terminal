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

The current executable remains `ptyxis` and the helper remains `ptyxis-agent` as compatibility-sensitive inherited implementation details. A later packaging migration may rename those entry points only after launch, D-Bus, host-helper, container, upgrade, and rollback behavior are validated.

See `docs/product-identity.md` for the complete identity and migration contract.

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

The development build should generate GoreeCloud application, D-Bus, AppStream, icon, and GSettings artifacts under the `com.goreecloud.Terminal.Devel` identity while retaining the current compatibility executables.

## Security and privacy boundaries

Terminal software is inherently security-sensitive because it launches shells, executes commands, interacts with credentials, enters remote systems, and can operate with elevated privileges.

GoreeCloud-specific features therefore follow these boundaries:

- no reusable credentials, private keys, tokens, or passwords are committed to source;
- Wardveil context is informational and must not become an authorization mechanism;
- `sudo` remains responsible for privilege authentication;
- terminal content and clipboard behavior are not treated as telemetry;
- source validation must be kept distinct from runtime and production acceptance;
- inherited upstream security and compatibility fixes are reviewed through controlled synchronization rather than merged blindly.

## Validation and release boundary

A green source build is only one acceptance layer. Stable or production approval additionally requires supported-workstation validation for:

- desktop and command-line launch;
- D-Bus activation;
- application identity and icon resolution;
- settings persistence and any settings migration;
- terminal rendering and input;
- clipboard and selection behavior;
- SSH, container, and elevated-session behavior;
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
