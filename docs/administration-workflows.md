# GoreeCloud Terminal Administration Workflows

## Purpose

This document defines the first Milestone 4 GoreeCloud administration workflow implemented in GoreeCloud Terminal: first-party launch conveniences for standard OpenSSH sessions.

The goal is to make routine GoreeCloud administration faster without creating a proprietary shell, SSH implementation, credential store, or second source of host configuration.

## Governing boundary

GoreeCloud Terminal remains an interface to standard Linux administration tools.

For SSH workflows:

- OpenSSH remains the SSH protocol implementation;
- `~/.ssh/config` and normal OpenSSH configuration remain authoritative;
- SSH agents, operating-system key stores, approved password managers, and normal OpenSSH key files remain responsible for credentials;
- GoreeCloud Terminal does not store SSH passwords or private keys;
- GoreeCloud Terminal does not copy credentials into its own application settings;
- host aliases are resolved by OpenSSH rather than by a GoreeCloud credential or connection database;
- OpenSSH host-key verification, authentication, forwarding, proxy, port, and policy behavior remain unchanged.

The terminal application may make an approved workflow easier to launch, but it does not grant access to the destination system.

## Canonical SSH launch commands

Open a standard OpenSSH session in a new GoreeCloud Terminal window:

```bash
goreecloud-terminal ssh HOST
```

Open the session as a new tab in the existing GoreeCloud Terminal instance:

```bash
goreecloud-terminal ssh-tab HOST
```

`HOST` may be a hostname, address, `user@hostname`, or a `Host` alias already defined in the user's OpenSSH configuration.

Additional arguments after the target are passed to the system `ssh` command without GoreeCloud reinterpretation. For example:

```bash
goreecloud-terminal ssh server-alias -p 2222
```

The launcher converts these conveniences into the existing Ptyxis command-execution path:

```text
goreecloud-terminal ssh TARGET ...
    -> ptyxis --new-window -- ssh TARGET ...

goreecloud-terminal ssh-tab TARGET ...
    -> ptyxis --tab -- ssh TARGET ...
```

The inherited `ptyxis` runtime remains the compatibility implementation while GoreeCloud Terminal provides the canonical first-party entry point.

## Host profiles

The first Milestone 4 layer intentionally treats OpenSSH `Host` aliases as the initial host-profile authority instead of introducing a second GoreeCloud host database.

For example, a normal OpenSSH configuration may define:

```text
Host infrastructure-example
    HostName example.internal
    User administrator
    IdentityFile ~/.ssh/example-key
```

GoreeCloud Terminal can then launch:

```bash
goreecloud-terminal ssh infrastructure-example
```

The Terminal launcher sees only the alias argument. OpenSSH remains responsible for resolving the actual hostname, account, key, jump host, port, and other connection options.

No real GoreeCloud hostnames, private addresses, usernames, identity-file paths, or credentials are hard-coded into this repository by this feature.

## Privacy and security

The SSH launcher does not:

- inspect private-key contents;
- read or copy password values;
- parse SSH agent keys;
- maintain connection credentials;
- bypass host-key verification;
- disable OpenSSH warnings;
- add telemetry or remote logging;
- log command arguments into a GoreeCloud service;
- change sudo or remote authorization behavior;
- represent a successful terminal launch as proof that a remote host is trusted or authorized.

As with any shell command, ordinary shell history or desktop process inspection may expose command-line arguments. Reusable secrets therefore must not be placed directly in SSH command arguments.

## Existing compatibility

All ordinary GoreeCloud Terminal/Ptyxis command-line arguments still pass through unchanged when the first argument is not `ssh` or `ssh-tab`.

For example:

```bash
goreecloud-terminal --version

goreecloud-terminal --new-window

goreecloud-terminal -- bash -lc 'printf "hello\n"'
```

continue to use the inherited command-line behavior.

## Automated acceptance

The repository provides:

```text
tools/test-ssh-launch-workflows.sh
```

The test uses a fake Ptyxis runtime and verifies:

- ordinary CLI arguments remain transparent pass-through;
- `ssh` requests a new window;
- `ssh-tab` requests a new tab;
- `user@host` values are not rewritten;
- additional OpenSSH arguments preserve order and values;
- local SSH subcommand help does not start the terminal runtime;
- a missing target fails before the runtime starts.

The test does not open a network connection and does not require credentials.

## Runtime acceptance still required

Before this workflow is accepted for a supported workstation, validation must exercise:

- an approved OpenSSH `Host` alias;
- a direct hostname or test address where appropriate;
- normal host-key verification;
- approved key or agent authentication;
- a failed authentication attempt without GoreeCloud-specific credential handling;
- new-window behavior;
- new-tab behavior;
- remote Wardveil context detection after the actual SSH process becomes active;
- terminal rendering, keyboard input, clipboard, disconnect, reconnect, and exit behavior;
- coexistence with direct `ssh` use outside GoreeCloud Terminal.

No reusable key material, passwords, tokens, or private session output should be added to acceptance evidence.

## Future Milestone 4 work

Later Administration Workflow layers may add user-facing host selection, workspace grouping, recent destinations, and other GoreeCloud infrastructure conveniences when they can be implemented without duplicating OpenSSH authority or increasing credential exposure.

Any graphical host selector should prefer non-secret OpenSSH configuration and user-controlled metadata, remain optional, and preserve direct standard-command access.

## Production boundary

This SSH launcher is a source-level Milestone 4 foundation. Passing CI proves argument-routing behavior and regression compatibility; it does not prove remote-host authentication, network reachability, security-policy compliance, or production workstation readiness.
