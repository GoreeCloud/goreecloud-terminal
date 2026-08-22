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

- exact source commit;
- build configuration;
- package or installation method;
- application ID under test;
- workstation distribution and version;
- harness output;
- manual checks completed;
- failures or regressions observed;
- rollback result when applicable.

Do not record reusable credentials, private keys, passwords, shell history containing sensitive values, or private terminal content.

## Production boundary

The runtime identity harness is an acceptance tool, not a production-readiness declaration. Stable or production approval remains blocked until the automated and manual checks applicable to the target package and workstation have been completed and recorded.
