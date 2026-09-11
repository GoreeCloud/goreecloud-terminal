# Native Glaze UI 1.3 Integration

## Status

**Lifecycle:** Development  
**Shared design-system target:** GLAZE UI V1.3 — Adaptive Resonance / `1.3.0`  
**Canonical tag:** `v1.3.0`  
**Canonical tag commit:** `ff34f232f295c9dcb07e4c681f66d4104d0b9323`  
**Terminal issue:** #73

This document defines the repository-local native mapping for GoreeCloud Terminal. It does not promote the native Terminal implementation, replace the transitional release line, or establish production acceptance.

## Architecture boundary

GoreeCloud owns the application chrome, session presentation, accessibility semantics, Terminal Theme Engine, product context menu, and product identity around the terminal surface. VTE remains the terminal-emulation authority.

Glaze UI must not inspect, reinterpret, log, or otherwise take ownership of shell output, terminal history, credentials, private hosts, raw process arguments, or terminal command content. Glaze owns application-chrome design behavior; it does not directly style the VTE content widget.

The presentation layer is intentionally split into:

- **application chrome:** GoreeCloud/Glaze-owned header, actions, tabs, menu surfaces, focus treatment, and supported session-state presentation;
- **terminal presentation policy:** GoreeCloud Terminal Theme Engine control of terminal foreground, background, cursor, selection, and theme resolution through VTE APIs; and
- **terminal emulation/content:** VTE-owned emulation, ANSI interpretation, PTY behavior, input/output processing, and terminal rendering engine.

## V1.3 contract mapping

The native mapping follows the current Stable shared Glaze target and records its exact authority in `data/glaze-ui-manifest.json` and `src/glaze-contract.h`.

Implemented repository-local contract points include:

- current Stable Glaze UI V1.3 / `1.3.0` authority pinned to the exact tag commit;
- the V1.3 personalization surface `follow-system`, `light`, `dark`, and `deep-dark` exposed through the Terminal Theme Engine;
- private persistence of the selected Terminal Theme Engine mode across launches;
- compact, form-factor-aware density for pointer/keyboard desktop chrome;
- a 48px touch-oriented reference target retained as shared Glaze contract metadata rather than forced onto normal desktop controls;
- a 56px Touch Assistance target floor for the reserved platform adapter state;
- visible keyboard-focus treatment;
- stronger non-color boundaries and solid application chrome for detected high-contrast themes;
- no custom motion or animation that could bypass reduced-motion behavior;
- explicit accessible names for native controls;
- explicit local-session and exited-session presentation backed by runtime state;
- a rebuilt Glaze-styled product context menu; and
- no Glaze selector targeting the VTE terminal content widget.

## Terminal Theme Engine

The Theme Engine is a GoreeCloud Terminal subsystem. It complements Glaze UI rather than replacing or duplicating it.

The current Theme Engine provides four bounded theme modes:

- `follow-system` — resolves to the captured platform light/dark preference;
- `light` — Terminal light presentation;
- `dark` — Terminal dark presentation; and
- `deep-dark` — Terminal deep-dark presentation permitted by the Glaze V1.3 personalization contract.

The Theme Engine owns terminal foreground, background, cursor, selection-background, theme resolution, and the persisted selected-mode preference. It deliberately does **not** silently remap the ANSI semantic palette in this development slice. Glaze continues to own shared application-chrome rules and accessibility precedence.

### Persistence contract

The production default preference path is `$XDG_CONFIG_HOME/goreecloud/terminal/theme.ini`. The automated tests may override the path using `GOREE_TERMINAL_THEME_SETTINGS_PATH` so CI never writes a real user preference. The preference directory is created with user-private permissions and the settings file is restricted to the current user where supported.

The persisted payload contains only the selected theme mode identifier. It must not contain terminal contents, commands, shell history, working directories, hostnames, SSH aliases, process arguments, credentials, private keys, tokens, or other session data. Missing state uses Follow System. Invalid state fails safe to Follow System rather than inventing or accepting an unrecognized mode.

Arbitrary user-authored theme import, theme export, cross-device synchronization, wallpaper-derived themes, custom ANSI semantic-palette authoring, and native Personalization adapter acceptance are not yet claimed.

## Appearance and accessibility behavior

The current GTK-native adapter snapshots the platform preference for dark application themes when a Terminal window is created. Follow System uses that baseline, while Light, Dark, and Deep Dark apply the corresponding application and terminal presentation policy. A persisted mode is restored before the initial Terminal presentation is applied.

High-contrast detection is based on the active GTK theme identity and strengthens application-chrome boundaries while removing translucent treatment. A portable reduced-transparency platform preference adapter is not yet verified; the corresponding CSS class is reserved but is not automatically asserted.

Desktop density is form-factor-aware. Pointer/keyboard desktop chrome must remain compact enough to preserve workspace and terminal-content priority. The shared 48px touch reference is not a mandatory desktop CSS minimum. Touch Assistance remains a distinct 56px target mode.

Accessibility outranks personalization. Theme selection must not remove visible focus, weaken applicable target sizes, override forced/high-contrast requirements, or become the only carrier of semantic state.

### Physical visual evidence

The first Zorin OS 17.3 / Wayland Flatpak rendering of exact candidate `2e497f9119a3678a5bb1b2abaa29c89b2e8c1bc2` failed rendered visual acceptance. The owner-provided screenshot showed an oversized title/header region, oversized tab strip, an overly large theme control, excessive spacing, and insufficient Glaze hierarchy/refinement for a desktop terminal application.

Source review traced the dominant sizing defect to an unconditional 56px header and 48px desktop control/tab minimums. Those values incorrectly treated touch references as normal pointer/keyboard desktop geometry. The development mapping has since been changed to compact desktop chrome with the 56px Touch Assistance override retained separately. That change requires a fresh physical-device re-test; no rendered pass is claimed until the new exact candidate is reviewed.

## Rebuilt right-click menu

The native terminal surface now uses a GoreeCloud-owned context menu rather than relying on inherited product actions.

The supported menu surface is:

- Copy;
- Paste;
- Select All;
- Clear;
- New Session; and
- Close Session.

Package-management and privileged convenience commands are not part of the product menu contract. `sudo apt update` is explicitly prohibited from the native right-click menu and is enforced by CI source checks.

**Clear** clears the visible terminal display and homes the cursor through VTE input handling. It does not execute the shell command `clear`, does not execute `sudo`, and does not write a command into shell history.

## Keyboard and focus contract

The native layer provides deterministic application-level shortcuts for common session operations while preserving normal VTE keyboard input for terminal content:

- `Ctrl+Shift+T` — open a new local terminal session.
- `Ctrl+Shift+W` — close the current terminal session.

Theme selection and all context-menu actions remain reachable through normal keyboard focus/menu navigation. No additional global shortcut is taken for theme switching.

## Runtime state truth

The native implementation may only present session states supported by runtime evidence.

Currently supported presentation truth:

- local session;
- running local session; and
- exited local session with preserved output.

Not yet represented as implemented:

- SSH/remote session identity;
- disconnected remote state;
- elevated-context warning;
- Everkeep-restored/recovery state;
- Wardveil Security state; and
- Privacy Shield authorization state.

Those remain future evidence-backed extension points and must not be shown as decorative completed integrations.

## Validation boundary

Repository-local validation includes the native build, session lifecycle tests, Glaze contract tests, Theme Engine selection/resolution/persistence tests, explicit context-menu source invariants, and `tools/validate-glaze-contract.py` drift checks.

Still required before any claim of Glaze consumer acceptance or production readiness:

- fresh rendered Linux acceptance across Follow System/Light/Dark/Deep Dark behavior and preference restoration after the failed `2e497f9` physical visual pass;
- rendered and keyboard review of the rebuilt context menu;
- direct runtime verification that Clear clears the visible display without adding a shell-history command;
- keyboard and focus traversal review;
- high-contrast review;
- large-text and reflow review;
- representative VTE rendering review;
- assistive-technology validation for the supported matrix;
- verified reduced-transparency platform integration if claimed;
- physical-device/native-platform acceptance where required; and
- normal GoreeCloud release and lifecycle approval.

Passing source CI is necessary but does not by itself establish Stable status, consumer conformance, workstation replacement, or production eligibility.
