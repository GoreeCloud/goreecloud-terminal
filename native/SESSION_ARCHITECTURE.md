# Native Session Architecture

GoreeCloud Terminal owns its window, tab, and session product architecture. GTK 4 provides native application/window controls and VTE provides terminal emulation/rendering only.

The current Development slice provides:

- one GoreeCloud-owned application window model;
- multiple independent VTE-backed terminal sessions in a native tab container;
- accessible session labels and explicit new/close session controls;
- default-shell spawning per session;
- tab selection without a second history, credential, or host database.

The architecture intentionally does not import Ptyxis product UI, workflow, session management, profile handling, or application logic. Glaze UI, Wardveil Security, Privacy Shield, Everkeep continuity, profiles, SSH workflows, persistence, and recovery remain separate reviewed integrations.

Closing a tab ends the UI ownership of that VTE session. Production-grade child-process shutdown policy, session restoration, crash recovery, and settings persistence remain future acceptance work and are not claimed by this slice.
