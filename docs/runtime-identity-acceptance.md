# GoreeCloud Terminal Runtime Identity Acceptance

## Purpose

This document defines the repeatable acceptance workflow for validating GoreeCloud Terminal product identity after build or installation.

The goal is to distinguish machine-verifiable installation identity from graphical and behavioral runtime acceptance. Passing the automated harness does not by itself authorize Stable or production deployment.

## Automated harness

The repository provides:

```text
tools/validate-runtime-identity.sh
```

The harness validates the expected GoreeCloud application identity while preserving the inherited Ptyxis compatibility runtime.

It checks:

- the canonical `goreecloud-terminal` launcher is installed and executable;
- the inherited `ptyxis` runtime remains installed and executable;
- `ptyxis-agent` remains installed and executable;
- desktop, AppStream, GSettings, D-Bus, icon, and man-page artifacts exist;
- desktop launch uses `goreecloud-terminal`;
- D-Bus activation uses `/usr/bin/goreecloud-terminal --gapplication-service`;
- the GoreeCloud launcher delegates to `ptyxis` without argument rewriting;
- the requested application ID appears in desktop/runtime metadata;
- both `goreecloud-terminal(1)` and `ptyxis(1)` documentation are installed.

## Staged-build validation

For a development build staged through Meson:

```bash
DESTDIR="$PWD/_install" meson install -C _build

tools/validate-runtime-identity.sh \
  --root "$PWD/_install" \
  --app-id com.goreecloud.Terminal.Devel
```

This mode does not execute the installed application. It validates the staged filesystem contract only.

## Installed-system validation

After an approved local installation:

```bash
tools/validate-runtime-identity.sh \
  --root / \
  --app-id com.goreecloud.Terminal
```

For additional non-graphical live checks:

```bash
tools/validate-runtime-identity.sh \
  --root / \
  --app-id com.goreecloud.Terminal \
  --live
```

Live mode additionally verifies that `goreecloud-terminal` and `ptyxis` are discoverable through `PATH`, that `goreecloud-terminal --version` completes, and that the expected GSettings schema is registered when `gsettings` is available.

## Supported-workstation evidence collector

The runtime-identity harness is not the complete workstation acceptance authority. For the published `50.2-rc.1` production Flatpak, use the separate read-only evidence collector documented in `docs/supported-workstation-acceptance.md`:

```bash
python3 tools/collect-workstation-acceptance.py \
  --bundle /path/to/published-50.2-rc.1.flatpak \
  --manual-status /path/to/workstation-manual-status.json \
  --output workstation-acceptance-evidence.json
```

The collector binds workstation evidence to the exact published bundle SHA-256 and installed OSTree commit, validates the Release Candidate local API contract, records only normalized non-sensitive package/OS/session information, and accepts a fixed status-only manual checklist. It is read-only and has no authority to install packages, alter settings, connect to remote systems, deploy the application, or promote lifecycle state.

## Manual graphical acceptance

Automated identity checks must be followed by manual runtime validation on a supported GoreeCloud Linux workstation.

Required checks include:

1. Launch GoreeCloud Terminal from the desktop application menu.
2. Confirm the GoreeCloud Terminal name and official icon appear correctly.
3. Launch `goreecloud-terminal` directly from a shell.
4. Launch the inherited `ptyxis` compatibility command and confirm it reaches the same intended product runtime without an application-ID collision.
5. Exercise New Window, New Tab, and Preferences desktop actions.
6. Verify D-Bus activation and single-instance behavior.
7. Verify preferences and profiles persist under the GoreeCloud GSettings namespace.
8. Confirm an installed upstream Ptyxis application can coexist without desktop, icon, settings, or D-Bus collisions.
9. Verify light and dark Glaze UI presentation.
10. Verify Local, Remote, Container, and Elevated Wardveil context behavior.
11. Verify terminal rendering, keyboard input, clipboard, selection, SSH, container, and normal `sudo` workflows.
12. Verify package removal, upgrade, and rollback behavior before any Stable promotion.

## Evidence requirements

Acceptance evidence should record:

- exact release/source identity;
- exact package identity;
- application ID under test;
- non-sensitive workstation distribution and version;
- sanitized machine-check results;
- fixed manual pass/fail/pending statuses;
- rollback/recovery acceptance status when applicable.

The checked-in collector intentionally does not accept arbitrary free-form notes. Detailed troubleshooting information should remain private during testing and must be separately sanitized before any public issue or long-term evidence record is created.

Do not record reusable credentials, private keys, passwords, shell history, terminal contents, private SSH configuration, profile contents, usernames, hostnames, private addresses, or private host inventories.

## Production boundary

The runtime identity harness and workstation evidence collector are acceptance tools, not production-readiness declarations. Even a complete workstation evidence record does not by itself authorize Stable because the separate second-distinct-package repository-backed update/rollback and post-rollback data-compatibility gates remain required.
