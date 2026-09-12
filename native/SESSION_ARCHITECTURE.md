# Native Session Architecture

GoreeCloud Terminal owns its window, tab, and session product architecture. GTK 4 provides native application/window controls and VTE provides terminal emulation/rendering only.

## Current Development architecture

The native implementation provides:

- GoreeCloud-owned application windows and session lifecycle state;
- multiple independent VTE-backed terminal sessions in a native tab container;
- accessible session labels and explicit new/close session controls;
- direct local default-shell spawning when the application is running unsandboxed;
- a same-user host-session bridge when the application is running inside Flatpak;
- host PTY creation outside the sandbox with the PTY master transferred over a private Unix-domain socket;
- foreign-PTY VTE attachment without broad host-filesystem access;
- runtime-evidence-backed running, exited, disconnected, host, and unavailable states;
- fail-closed input behavior after host-control loss or child exit; and
- tab selection without a second history, credential, or host database.

## Host-session boundary

A sandboxed native Terminal session is not permitted to claim that it is a host session merely because a shell process started. The host-session client must successfully validate the dedicated runtime socket and the host agent's peer UID, complete the versioned protocol exchange, and receive a PTY descriptor before the session may be presented as a verified host session.

If that process fails, the session is presented as unavailable/disconnected, input is disabled, and the existing terminal surface is preserved for status/output. The application does not fall back to a sandbox-local shell while presenting it as equivalent to the host environment.

The host-session control channel is distinct from terminal I/O. It carries lifecycle evidence only; it does not carry command text, shell output, terminal history, credentials, environment dumps, or other terminal contents.

## Ownership boundaries

VTE remains responsible for terminal emulation and PTY presentation. GoreeCloud Terminal owns session creation, state transitions, application actions, UI presentation, platform-system integration, recovery policy, and user-facing trust/context states. The host agent owns host-side PTY creation and child supervision for bridged local sessions.

The architecture intentionally does not import Ptyxis product UI, workflow, session management, profile handling, or application logic.

## Still required

SSH/remote sessions, profiles/workspaces, broader settings persistence, split panes, shell/context integration, dangerous-paste protection, search, scrollback policy, Wardveil Security, Privacy Shield, Everkeep, Manager, Mesh, Identity, migration/rollback, production packaging, physical-workstation PTY acceptance, and release qualification remain separate reviewed work until implemented and verified.

Closing a tab ends the UI ownership of that VTE session and closes the host-control channel when present. Production acceptance still requires physical validation of child shutdown/SIGHUP behavior, service failure/restart, recovery, and exact packaged artifacts; source-level lifecycle behavior alone is not sufficient evidence.
