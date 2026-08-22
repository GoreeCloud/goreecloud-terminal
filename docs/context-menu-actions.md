# Terminal context-menu actions

This document defines the GoreeCloud Terminal right-click actions added above the inherited Ptyxis context menu.

## Copy

The right-click **Copy** action copies the complete terminal text buffer, including available scrollback history, without requiring the user to select text first.

This behavior is intentionally different from the existing keyboard copy-selection workflow. The normal copy shortcut continues to copy only selected text. When a selection exists, the context menu also exposes **Copy Selection** for the same selection-scoped behavior. **Copy as HTML** remains selection-scoped.

The implementation writes the current VTE terminal contents to an in-memory output stream and places the resulting text on the desktop clipboard. It does not alter the selection, scroll position, terminal process, command history, or session context.

## Run sudo apt update -y

The right-click **Run sudo apt update -y** action sends the literal command below to the active terminal child and submits it:

```sh
sudo apt update -y
```

The action does not bypass `sudo`, store a password, inject credentials, elevate GoreeCloud Terminal itself, or change the operating system's authorization policy. If `sudo` requires authentication, the normal terminal password prompt remains authoritative.

The action is disabled while terminal input is read-only. It sends text to the process currently attached to the terminal, so it should be used from a normal shell prompt. If the active terminal is inside SSH, a container, a root shell, or another interactive process, the command is delivered to that active context rather than silently redirected to the local host.

Wardveil Security session-context presentation remains read-only and does not authorize or validate this command. The action must never be described as protected merely because Wardveil identifies the current session type.

## Validation requirements

Before this feature is accepted beyond source validation, verify that:

- right-click **Copy** works without a selection;
- copied text includes scrollback older than the visible screen;
- **Copy Selection** appears only when text is selected and copies only that selection;
- the standard keyboard copy shortcut remains selection-scoped;
- **Run sudo apt update -y** enters and executes the exact command from an ordinary shell prompt;
- normal `sudo` authentication remains unchanged;
- the command action is unavailable in read-only mode;
- the context menu remains usable in normal, narrow, maximized, and fullscreen layouts;
- no Wardveil, Glaze UI, VTE palette, clipboard, selection, or terminal-rendering regression is introduced.
