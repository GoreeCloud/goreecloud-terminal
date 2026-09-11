# GoreeCloud Terminal Native Foundation

This directory contains the original GoreeCloud-owned GoreeCloud Terminal implementation required by the platform-wide native-application mandate.

## Boundary

The native implementation is intentionally isolated from the inherited Ptyxis product source. Ptyxis remains a temporary migration, compatibility, behavioral-reference, testing, and historical source while native capability replaces inherited product code.

The native foundation may use narrowly scoped mature platform libraries where independently replacing them would increase terminal, rendering, accessibility, operating-system, standards, or interoperability risk. The current foundation uses GTK 4 and VTE as supporting libraries; it does not import Ptyxis application architecture, UI, workflows, branding, or general application logic.

VTE remains the terminal-emulation authority. GoreeCloud owns the surrounding product chrome, session state, application actions, accessibility semantics, Glaze UI mapping, terminal theme policy, and product-level context menu.

## Current Development slice

The verified source on this branch provides:

- an original GTK application entry point and GoreeCloud-owned development identity;
- an embedded VTE terminal surface with local default-shell spawning;
- GoreeCloud-owned local session, tab, close, and window presentation;
- explicit local-running and local-exited session labels backed by runtime state;
- Glaze UI V1.3 / `1.3.0` application-chrome mapping pinned to the canonical Stable tag commit;
- a GoreeCloud Terminal Theme Engine with `follow-system`, `light`, `dark`, and `deep-dark` modes;
- persistence of the selected Theme Engine mode across launches through a private XDG configuration file;
- fail-safe fallback to Follow System when persisted theme state is missing or invalid;
- separation between Glaze-owned application chrome and Terminal-owned VTE presentation policy;
- Terminal-owned foreground, background, cursor, and selection theme resolution without silent ANSI semantic-palette remapping;
- high-contrast chrome strengthening without Glaze styling terminal content;
- compact, form-factor-aware pointer/keyboard desktop chrome while preserving the shared 48px touch-oriented reference target and the reserved 56px Touch Assistance target;
- visible focus treatment and accessible names for native controls;
- a rebuilt GoreeCloud-owned right-click terminal menu containing Copy, Paste, Select All, Clear, New Session, and Close Session;
- no `sudo apt update` or other package-management shell command in the native right-click menu;
- Clear behavior that clears the visible terminal display without executing a shell command or writing to shell history;
- `Ctrl+Shift+T` for a new local session and `Ctrl+Shift+W` for closing the current session;
- no custom application motion that bypasses reduced-motion behavior;
- a machine-readable Glaze adoption manifest, Glaze contract tests, Theme Engine persistence tests, and drift validation; and
- an isolated Meson build with strict compiler warnings and native unit tests.

The exact Glaze boundary and remaining acceptance work are documented in `GLAZE_UI_13.md`.

## Theme Engine boundary

The Theme Engine is a GoreeCloud Terminal subsystem, not a replacement for Glaze UI. Glaze remains authoritative for shared application-chrome design rules and accessibility precedence. The Theme Engine owns terminal-specific presentation policy exposed through VTE APIs.

The current Theme Engine persists only the selected mode identifier. Production defaults to `$XDG_CONFIG_HOME/goreecloud/terminal/theme.ini`; the test suite may override that path through `GOREE_TERMINAL_THEME_SETTINGS_PATH` so automated validation never writes real user configuration. The preference directory is created with private permissions and the settings file is restricted to the user where the platform permits it.

The persisted file must not contain terminal contents, shell commands, shell history, working directories, hostnames, SSH aliases, credentials, private keys, tokens, or other session data. Invalid persisted state fails safe to Follow System.

Arbitrary third-party theme import, theme export, cross-device synchronization, wallpaper-derived themes, custom ANSI semantic-palette authoring, and native Personalization adapter acceptance remain separate future work.

## Context-menu boundary

The native right-click menu is application-owned and contains only explicit terminal/session actions. It must not expose one-click package-management or privileged shell commands. In particular, `sudo apt update` is prohibited from the native menu contract.

Clear is an internal terminal-display action rather than a shell command. This keeps the action deterministic and avoids adding `clear` or equivalent commands to shell history.

## Laptop and workstation acceptance

Physical-device review is required before the native implementation can qualify for Stable promotion. From the repository root, run the acceptance entry point with Bash:

```sh
bash scripts/testing/native-workstation-acceptance.sh
```

The runner checks the exact checked-out Git revision, refuses a dirty working tree, validates the Glaze contract, builds the native application, runs the unit tests, and then launches two isolated physical-device passes. It uses a temporary `XDG_CONFIG_HOME`, so testing the Theme Engine does not overwrite the user's normal GoreeCloud Terminal preferences.

The first launch requires review of all four Theme Engine modes, terminal readability, the rebuilt six-action context menu, absence of `sudo apt update`, Copy/Paste/Select All/Clear, New Session/Close Session, shortcuts, focus, high contrast where available, representative VTE behavior, and assistive technology where available. The first pass intentionally ends on Deep Dark. The second launch uses the same isolated preference store and must restore Deep Dark automatically.

The first Zorin OS 17.3 / Wayland Flatpak rendering of candidate `2e497f9` failed visual acceptance because desktop header and tab geometry incorrectly applied touch-sized target dimensions. That failure is retained as evidence. The current development line uses compact desktop density and must be revalidated physically before any rendered acceptance claim.

For the Fedora 44 reference environment used by CI, the required development packages are:

```sh
sudo dnf install gcc git glib2-devel gtk4-devel meson ninja-build pkgconf-pkg-config vte291-gtk4-devel
```

Other supported Linux distributions must provide equivalent GLib 2.76+, GTK 4.14+, VTE GTK4 0.76+, GCC, Meson, Ninja, pkg-config, Git, and Python 3 development tooling.

The acceptance runner does not silently mark the application Stable. Human visual/accessibility results must be recorded and the applicable GoreeCloud release, platform-integration, migration, packaging, supported-workstation, and promotion gates must still be satisfied.

## Explicitly unclaimed integrations

The native source does not yet claim implemented SSH/remote state, disconnected-state presentation, elevated-context warnings, Wardveil Security state, Privacy Shield authorization state, Everkeep recovery state, or verified reduced-transparency platform adaptation. Those capabilities require their own runtime contracts and evidence before UI can present them as real.

## Lifecycle

This is **Development source only**. It is not a replacement package, release candidate, production deployment, current Stable implementation, or workstation replacement.

Passing repository-local build and contract tests is necessary but does not establish rendered accessibility acceptance, assistive-technology acceptance, physical-device/native-platform acceptance, Glaze consumer conformance, or GoreeCloud release approval.

## Required next slices

The native implementation still needs broader settings persistence beyond the Theme Engine, profile/workspace persistence, optional theme import/export policy, SSH and remote-session workflows, evidence-backed remote/disconnected/elevated/recovery state, Wardveil Security integration, Privacy Shield controls, Everkeep continuity/recovery, richer shell/context integration, split-pane workflows, dangerous-paste protection, migration from the transitional application, packaging, rendered accessibility validation, and supported-workstation acceptance before it can replace the transitional Ptyxis-derived line.
