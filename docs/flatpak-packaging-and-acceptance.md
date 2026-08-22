# GoreeCloud Terminal Flatpak Packaging and Acceptance

## Purpose

This document defines the initial Milestone 5 packaging and package-acceptance model for GoreeCloud Terminal.

The first packaged acceptance candidate is a Flatpak development build using the isolated application identity:

```text
com.goreecloud.Terminal.Devel
```

This package exists to prove that the maintained fork can be built, bundled, installed, inspected, exercised non-graphically, removed, and later tested graphically without colliding with a future production `com.goreecloud.Terminal` installation.

A successful package build is not a Stable release and is not authorization to replace the workstation's current terminal.

## Why Flatpak is the first Milestone 5 package

GoreeCloud Terminal requires a modern GTK/libadwaita/VTE stack while the approved Linux workstation prioritizes stability and should not be modified merely to satisfy development-library requirements.

Flatpak provides a controlled way to package the required GNOME runtime independently from the host distribution while still allowing the terminal-specific host integration that must be tested explicitly.

Native distribution packages remain an allowed later target when they provide a practical benefit and the supported distribution dependency baseline is compatible.

## Manifest

The development manifest is:

```text
com.goreecloud.Terminal.Devel.json
```

The manifest intentionally uses:

- application ID `com.goreecloud.Terminal.Devel`;
- GNOME Platform 50;
- GNOME SDK 50;
- canonical command `goreecloud-terminal`;
- `-Ddevelopment=true` so the GoreeCloud development application and GSettings identity are generated;
- `-Dlibc-compat=true` for the inherited Ptyxis agent compatibility model;
- pinned dependency revisions/checksums for the bundled support components.

The dependency and permission baseline is derived from the current maintained Flatpak packaging model for upstream Ptyxis, then adapted to GoreeCloud Terminal identity and source. Upstream packaging remains a reference for terminal-specific integration requirements; it does not define GoreeCloud branding, release status, or application identity.

## Initial sandbox permissions

The first acceptance manifest retains the upstream terminal-oriented permission baseline:

```text
--allow=devel
--device=dri
--filesystem=host
--share=ipc
--share=network
--socket=fallback-x11
--socket=wayland
--talk-name=org.freedesktop.Flatpak
```

These permissions are deliberately recorded rather than treated as invisible defaults.

### Permission-review rule

The initial goal is functional parity with the mature Ptyxis Flatpak host-integration model, not automatic approval of every permission forever.

Each permission must remain subject to runtime acceptance and later minimization review. A permission may be removed only after representative local shells, working-directory behavior, host PTY/helper behavior, OpenSSH, container workflows, clipboard, graphics, Wayland/X11 fallback, and other affected functions are tested.

Conversely, a failing feature does not automatically justify adding a broader permission. The failure must first be traced to a documented requirement.

### Broad host-filesystem access

`--filesystem=host` is a broad permission and receives special scrutiny.

It is retained in the first acceptance package because a general-purpose terminal must interact predictably with host files and working directories and because the upstream Ptyxis Flatpak model currently relies on broad host access.

GoreeCloud Terminal must not use this permission to introduce background indexing, telemetry, automatic file upload, credential harvesting, or unrelated host scanning.

### Network access

`--share=network` is required to test normal network-aware terminal workflows such as OpenSSH. Network access does not make GoreeCloud Terminal an SSH authority; OpenSSH configuration, host-key verification, authentication, and credentials remain external standard-system responsibilities.

### Flatpak host integration

`--talk-name=org.freedesktop.Flatpak` and the development capability are retained for the inherited host/helper integration used by Ptyxis. They must be validated against the actual packaged runtime and may not be repurposed as an authorization bypass.

## Build reproducibility boundary

The manifest pins external support sources by checksum or exact Git commit where the reference packaging does so.

The GoreeCloud Terminal module itself uses the checked-out source tree supplied to the build. GitHub Actions therefore binds package evidence to the exact workflow head commit that produced the artifact.

A later Stable release process should additionally bind the package to an approved immutable release tag or release commit and record package hashes as release evidence.

## Automated package acceptance

The repository provides a dedicated GitHub Actions workflow:

```text
.github/workflows/flatpak-acceptance.yml
```

The workflow must:

1. validate the manifest as JSON;
2. install the declared GNOME runtime and SDK from Flathub in the isolated CI user environment;
3. build the Flatpak from the exact checked-out GoreeCloud Terminal source;
4. export an OSTree repository;
5. create a `.flatpak` bundle for `com.goreecloud.Terminal.Devel`;
6. install that bundle into the CI user's Flatpak installation;
7. verify the installed application identity and runtime metadata;
8. execute the canonical launcher with `--version` without requiring a graphical session;
9. execute local SSH/profile help paths that must not open a network connection or require credentials;
10. uninstall the application and verify that it is no longer installed;
11. upload the exact `.flatpak` bundle as a development acceptance artifact.

The workflow must not install GoreeCloud Terminal onto a real GoreeCloud workstation.

## Artifact identity

The CI artifact is a development acceptance bundle, not a distribution release.

Expected application identity:

```text
com.goreecloud.Terminal.Devel
```

Expected canonical command:

```text
goreecloud-terminal
```

The inherited `ptyxis` runtime/helper compatibility names may remain inside the package while the canonical product presentation remains GoreeCloud Terminal.

## Required workstation acceptance

Before a Flatpak package can be considered accepted for an approved workstation, testing must cover at minimum:

- package installation from the exact approved artifact;
- desktop launcher name and icon;
- direct `goreecloud-terminal` launch;
- inherited `ptyxis` compatibility behavior where retained;
- D-Bus activation and single-instance behavior;
- local shell startup and working directories;
- shell environment behavior;
- terminal rendering, Unicode, font handling, selection, copy, paste, and scrollback;
- keyboard shortcuts and accessibility;
- direct SSH and profile-based SSH workflows;
- normal OpenSSH host-key verification and approved authentication;
- container and host-helper workflows;
- Wardveil Local/Remote/Container/Elevated runtime context transitions;
- Glaze UI light/dark and responsive presentation;
- settings persistence under the development namespace;
- settings migration in an isolated test account when explicitly invoked;
- clean removal without deleting unrelated user data;
- reinstall behavior;
- upgrade from an earlier packaged candidate;
- rollback to a previously accepted package;
- coexistence with upstream Ptyxis if both are intentionally installed.

## Evidence and privacy

Package acceptance evidence may record:

- exact source commit;
- workflow run ID;
- manifest identity;
- Flatpak runtime and SDK versions;
- bundle filename and cryptographic hash;
- pass/fail results;
- synthetic test-profile names;
- non-sensitive diagnostic output.

Evidence must not record reusable passwords, private-key material, private SSH configuration, real private session output, tokens, or other secrets.

## Native package direction

A `.deb` or other native distribution package may be added later when the supported workstation dependency baseline can satisfy GoreeCloud Terminal without unsafe library replacement or excessive host modification.

Native packaging must not vendor or overwrite core desktop libraries merely to force compatibility. If a native package cannot meet the documented host dependency and rollback requirements cleanly, Flatpak remains the safer supported delivery mechanism for that workstation generation.

## Production boundary

Passing Flatpak CI establishes a reproducible development package candidate and package-install/remove smoke evidence in CI. It does not establish graphical workstation acceptance, production replacement, Stable release status, or permission-minimization completion.

Promotion beyond Development requires the remaining Milestone 5 runtime, security, accessibility, upgrade, rollback, and supported-workstation gates to be recorded against the exact package artifact.
