# GoreeCloud Terminal Local API

## Supported API

GoreeCloud Terminal provides one intentionally narrow built-in API surface:

```bash
goreecloud-terminal api status
```

The API is a versioned, read-only local CLI contract. It does not open a network listener and does not expose a TCP, HTTP, WebSocket, or writable D-Bus service.

## Schema version 1

`api status` returns JSON containing static product and release metadata, canonical application identity, the local-CLI/read-only API contract, Privacy Shield minimization flags, declared Privacy Shield capabilities, and the Wardveil presentation boundary.

The response deliberately excludes:

- terminal contents and scrollback;
- shell commands and command history;
- environment variables and process command lines;
- clipboard contents;
- credentials, passwords, tokens, keys, or authentication material;
- SSH destinations, profile aliases, hostnames, usernames, or addresses;
- current tabs, sessions, workspaces, profiles, or remote systems;
- user or device identifiers.

## Compatibility

Consumers must check `schema_version`. Additive fields may be introduced in a compatible schema revision; incompatible changes require a new schema version.

`goreecloud-terminal api --help` describes the supported operation. Unknown API operations fail with usage status and are not forwarded to the inherited Ptyxis runtime.

## Authority boundary

The local API is an introspection surface only. It cannot execute terminal commands, start SSH connections, change profiles, mutate settings, grant privileges, or act as an authorization mechanism.

OpenSSH, the shell, `sudo`, the operating system, and the Ptyxis/VTE runtime remain authoritative for their respective functions.
