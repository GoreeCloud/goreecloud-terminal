# GoreeCloud Terminal Native Host-Session Agent

**Lifecycle:** Development implementation  
**Stable/production approval:** Not established  
**Tracking:** #80 under Stable-qualification umbrella #79

## Purpose

The native host-session agent is an original GoreeCloud-owned Linux helper that gives the sandboxed GoreeCloud Terminal UI a real host PTY without granting broad host-filesystem access or treating a sandbox shell as a host shell.

The Development native client now uses this bridge when it detects Flatpak execution. If the bridge is unavailable, the application fails closed and presents the host session as unavailable; it does not silently substitute a shell running inside the sandbox.

## Trust boundary

The agent runs outside the application sandbox as the logged-in local user. It listens only on a Unix-domain socket under the user's runtime directory:

`$XDG_RUNTIME_DIR/goreecloud-terminal/host-agent.sock`

The implemented Development contract requires:

- GoreeCloud Terminal-specific runtime directory mode `0700`;
- socket mode `0600`;
- Linux `SO_PEERCRED` verification on both sides of the connection so the client and agent establish the same-user boundary;
- a versioned, local-only binary protocol;
- no TCP/UDP listener;
- no general-purpose arbitrary-command RPC;
- no terminal-content, command-history, credential, token, key, or shell-output logging;
- a host-side `openpty()` and controlling-terminal setup;
- PTY-master transfer to the client using `SCM_RIGHTS`;
- VTE attachment through a foreign PTY; and
- an independent control channel for exit/error/disconnect evidence.

The initial protocol can request only the authenticated local user's configured default shell. The agent does not accept an arbitrary command string from the sandbox.

## Flatpak boundary

The Development native Flatpak exposes only the dedicated runtime directory required for the bridge:

`--filesystem=xdg-run/goreecloud-terminal`

The normal host-session path does not require `--filesystem=host` and does not depend on `org.freedesktop.Flatpak` host-command authority.

## User service and packaging source

The host agent Meson project installs:

- `goreecloud-terminal-host-agent` in the configured `libexecdir`; and
- `goreecloud-terminal-host-agent.service` as a systemd user service.

The service is deliberately constrained to Unix-domain networking and applies user-service hardening including `NoNewPrivileges`, `RestrictAddressFamilies=AF_UNIX`, and a private `0077` umask. Source-level installation support does not by itself establish a production package or supported-distribution qualification.

## Build and contract validation

```sh
meson setup _host-agent-build native/host-agent --buildtype=debugoptimized
meson compile -C _host-agent-build
python3 native/host-agent/test-host-agent.py \
  _host-agent-build/goreecloud-terminal-host-agent
```

The `Native Host Session Contract` workflow also stages the installed host agent and user service, checks the narrow Flatpak permission boundary, validates the client/agent trust contract, and runs the PTY integration test.

## Current phase

The repository now contains the protocol, host agent, automated PTY contract, native client, foreign-PTY VTE attachment, fail-closed disconnected/unavailable state, narrowly scoped runtime-directory permission, and user-service installation source.

These are Development implementation claims only. Before production acceptance, #80 still requires physical validation of shell startup, job control, resize/SIGWINCH, sudo and other interactive prompts, Unicode, selection/clipboard, child exit, close/SIGHUP, service failure/restart, and recovery behavior. Exact-artifact packaging, migration/rollback, platform-system conformance, supported-workstation qualification, and the remaining #79 release gates also remain required.
