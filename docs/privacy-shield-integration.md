# GoreeCloud Terminal — Privacy Shield Integration

## Purpose

GoreeCloud Terminal adopts Privacy Shield as a narrow privacy contract around GoreeCloud-owned terminal features. Privacy Shield does not replace the terminal runtime, the shell, OpenSSH, operating-system authorization, or Wardveil Security.

The Release Candidate claims only the capabilities that are actually implemented and reviewable:

- `telemetry-minimization`
- `data-minimization`

The machine-readable contract is `privacy-shield/adapter.json`.

## Privacy boundary

GoreeCloud Terminal is local-first. GoreeCloud-specific features do not add analytics, remote behavior tracking, advertising telemetry, tracker learning, or remote session-history collection.

The following private material is excluded from status, acceptance, and release evidence:

- terminal scrollback and terminal contents;
- typed commands and shell history;
- clipboard contents;
- passwords, tokens, private keys, credentials, and authentication material;
- raw SSH configuration, private destinations, usernames, command history, or remote commands;
- environment variables and process command lines;
- private host/workspace contents, IP addresses, routes, or other infrastructure inventory.

Host profiles deliberately store only a workspace label, a local profile identifier, and an OpenSSH `Host` alias. OpenSSH remains authoritative for actual hostnames, users, ports, keys, agents, forwarding, proxy configuration, host-key policy, and authentication. Recent-destination persistence remains intentionally unimplemented because it would add privacy and retention obligations.

## Local API boundary

The supported GoreeCloud Terminal API is a read-only local CLI status contract. It reports static product, release, integration, and privacy metadata. It does not enumerate sessions, tabs, terminal buffers, profiles, workspaces, hosts, environment variables, processes, credentials, or command history.

See `docs/local-api.md`.

## Diagnostics and observability

The inherited GTK, GLib, VTE, and Ptyxis foundations retain their normal local diagnostic mechanisms. GoreeCloud release and acceptance tooling may record source revisions, package hashes, OSTree commits, runtime/application identity, and pass/fail results. GoreeCloud-specific diagnostics must not add reusable secrets or private terminal/session material to logs.

This boundary supplies useful release observability without converting terminal activity into telemetry.

## Relationship to Wardveil Security

Wardveil presents verified Local, Remote, Container, and Elevated context. It is informational context, not authorization. Privacy Shield governs privacy minimization around GoreeCloud-owned behavior and evidence. Neither system may be treated as proof that a command or session is safe.

## Release Candidate state

For `50.2-rc.1`, runtime acceptance remains required and `production_approved` is false. Privacy Shield source conformance is therefore an RC gate, while supported-workstation privacy acceptance remains a Stable gate.
