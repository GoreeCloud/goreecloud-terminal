# GoreeCloud Terminal Native Host-Session Agent

**Lifecycle:** Development foundation only  
**Stable/production approval:** Not established  
**Tracking:** #80 under Stable-qualification umbrella #79

## Purpose

The native host-session agent is an original GoreeCloud-owned Linux helper for giving a sandboxed GoreeCloud Terminal UI a real host PTY without granting broad host-filesystem access or treating a sandbox shell as a host shell.

The current Development Flatpak is intentionally isolated and therefore launches a shell inside the sandbox. That behavior is useful for UI/runtime testing, but it is not the production terminal model.

## Trust boundary

The agent runs outside the application sandbox as the logged-in local user. It listens only on a Unix-domain socket under the user's runtime directory:

`$XDG_RUNTIME_DIR/goreecloud-terminal/host-agent.sock`

The foundation requires:

- GoreeCloud Terminal-specific runtime directory mode `0700`;
- socket mode `0600`;
- Linux `SO_PEERCRED` verification that the connecting peer UID matches the agent UID;
- a versioned, local-only binary protocol;
- no TCP/UDP listener;
- no general-purpose arbitrary-command RPC;
- no terminal-content, command-history, credential, token, key, or shell-output logging;
- a host-side `openpty()` and controlling-terminal setup;
- PTY-master transfer to the client using `SCM_RIGHTS`.

The initial protocol can request only the authenticated local user's configured default shell. The agent does not accept an arbitrary command string from the sandbox.

## Why this exists instead of a broad sandbox escape

A terminal needs genuine PTY and job-control behavior. A production GoreeCloud design must preserve those semantics while keeping the sandbox permission surface narrow. The intended Flatpak integration therefore targets access only to the dedicated runtime socket, not `--filesystem=host` and not a normal-session dependency on the broad `org.freedesktop.Flatpak` host-command interface.

## Build

The host agent deliberately has no GTK or VTE dependency so it can be packaged for older supported Linux hosts separately from the sandboxed UI runtime.

```sh
meson setup _host-agent-build native/host-agent --buildtype=debugoptimized
meson compile -C _host-agent-build
python3 native/host-agent/test-host-agent.py \
  _host-agent-build/goreecloud-terminal-host-agent
```

## Current phase

This directory currently establishes only the host-agent/protocol foundation and its automated PTY contract. It does **not** yet wire the agent into the GTK/VTE application or change Flatpak permissions.

Before production acceptance, #80 still requires the native UI client, foreign-PTY attachment, lifecycle/recovery handling, user-service/package installation, narrowly scoped runtime-socket permission, and physical validation of interactive shell behavior including job control, resize, sudo prompts, Unicode, child exit, and close/recovery semantics.
