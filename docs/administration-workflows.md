# GoreeCloud Terminal Administration Workflows

## Purpose

This document defines the GoreeCloud Terminal Milestone 4 administration-workflow foundation.

The goal is to make routine GoreeCloud administration faster without creating a proprietary shell, SSH implementation, credential store, second source of SSH connection configuration, or application-specific access-control system.

The current source foundation includes:

- first-party launch conveniences for standard OpenSSH sessions;
- optional user-controlled host profiles that reference OpenSSH `Host` aliases;
- optional workspaces that organize those profiles without changing authorization boundaries.

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

## Direct OpenSSH launch commands

Open a standard OpenSSH session in a new GoreeCloud Terminal window:

```bash
goreecloud-terminal ssh server-alias
```

Open the session as a new tab in the existing GoreeCloud Terminal instance:

```bash
goreecloud-terminal ssh-tab server-alias
```

Arguments after `ssh` or `ssh-tab` are passed to the system `ssh` command unchanged, so normal OpenSSH argument ordering applies. Options belong before the destination:

```bash
goreecloud-terminal ssh -p 2222 server-alias
```

A remote command may follow the destination:

```bash
goreecloud-terminal ssh server-alias uname -a
```

The launcher converts these conveniences into the existing Ptyxis command-execution path:

```text
goreecloud-terminal ssh OPENSSH_ARGUMENT ...
    -> ptyxis --new-window -- ssh OPENSSH_ARGUMENT ...

goreecloud-terminal ssh-tab OPENSSH_ARGUMENT ...
    -> ptyxis --tab -- ssh OPENSSH_ARGUMENT ...
```

The inherited `ptyxis` runtime remains the compatibility implementation while GoreeCloud Terminal provides the canonical first-party entry point.

## Host profiles and workspaces

Milestone 4 now includes an optional GoreeCloud Terminal metadata file for organizing OpenSSH aliases into user-controlled workspaces.

The metadata file contains only:

```text
WORKSPACE<TAB>PROFILE<TAB>SSH_HOST_ALIAS
```

It deliberately does not contain passwords, private keys, host transport settings, usernames, ports, proxy policy, identity-file paths, or other SSH authentication/connection policy.

For example, synthetic local metadata may contain:

```text
Infrastructure	primary-vps	vps-admin
Virtualization	primary-hypervisor	hypervisor-admin
Storage	primary-storage	storage-admin
```

The corresponding `vps-admin`, `hypervisor-admin`, and `storage-admin` values remain ordinary OpenSSH `Host` aliases. OpenSSH resolves the actual host, user, key, port, jump host, and other options.

List workspaces:

```bash
goreecloud-terminal workspaces
```

List profiles:

```bash
goreecloud-terminal profiles
```

Filter profiles by workspace:

```bash
goreecloud-terminal profiles Infrastructure
```

Launch a profile in a new window:

```bash
goreecloud-terminal profile primary-vps
```

Launch a profile in a new tab:

```bash
goreecloud-terminal profile-tab primary-vps
```

The profile layer is optional. Direct `ssh`, `goreecloud-terminal ssh`, and inherited `ptyxis` workflows remain available independently.

See `docs/host-profiles-and-workspaces.md` for the metadata format, validation rules, privacy boundary, and runtime-acceptance requirements.

## Profile fail-closed behavior

Before any profile is listed or launched, GoreeCloud Terminal validates the profile metadata.

It refuses malformed rows, duplicate profile IDs, aliases containing whitespace, aliases beginning with `-`, missing profile configuration, unknown workspaces, and unknown profiles before launching the terminal runtime.

This prevents an ambiguous or malformed metadata record from silently selecting an unintended administrative destination.

## Privacy and security

The administration-workflow layer does not:

- inspect private-key contents;
- read or copy password values;
- parse SSH agent keys;
- maintain connection credentials;
- bypass host-key verification;
- disable OpenSSH warnings;
- add telemetry or remote logging;
- log command arguments into a GoreeCloud service;
- change sudo or remote authorization behavior;
- treat workspace/profile membership as trust or authorization;
- force Wardveil Remote presentation merely because a profile was selected;
- represent a successful terminal launch as proof that a remote host is trusted or authorized.

As with any shell command, ordinary shell history or desktop process inspection may expose command-line arguments. Reusable secrets therefore must not be placed directly in SSH command arguments.

Workspace labels and aliases can reveal infrastructure organization even when they contain no credentials. The local profile file should therefore be treated as private configuration and should not be copied into public source, issues, or CI logs without review.

## Existing compatibility

All ordinary GoreeCloud Terminal/Ptyxis command-line arguments still pass through unchanged when the first argument is not one of the GoreeCloud administration subcommands.

For example:

```bash
goreecloud-terminal --version

goreecloud-terminal --new-window

goreecloud-terminal -- bash -lc 'printf "hello\n"'
```

continue to use the inherited command-line behavior.

## Automated acceptance

The repository provides two isolated Milestone 4 test layers:

```text
tools/test-ssh-launch-workflows.sh
tools/test-host-profiles-workspaces.sh
```

The SSH routing test verifies:

- ordinary CLI arguments remain transparent pass-through;
- `ssh` requests a new window;
- `ssh-tab` requests a new tab;
- OpenSSH option ordering is preserved;
- `user@host` destinations are not rewritten;
- remote-command arguments preserve order and values;
- local SSH subcommand help does not start the terminal runtime;
- a missing OpenSSH argument list fails before the runtime starts.

The host-profile/workspace test verifies:

- stable workspace listing;
- complete profile listing;
- workspace filtering;
- new-window and new-tab profile routing;
- remote-command preservation;
- unknown-workspace and unknown-profile refusal;
- duplicate-profile refusal;
- option-like alias refusal;
- missing-configuration refusal;
- no terminal runtime launch when validation rejects metadata.

Both tests use a fake Ptyxis runtime. They do not open a network connection and do not require credentials.

## Runtime acceptance still required

Before Milestone 4 is accepted for a supported workstation, validation must exercise:

- approved direct OpenSSH `Host` alias launch;
- an approved private profile metadata file containing only non-secret aliases and organization labels;
- normal host-key verification;
- approved key or agent authentication;
- a failed authentication attempt without GoreeCloud-specific credential handling;
- direct new-window and new-tab SSH behavior;
- profile new-window and new-tab behavior;
- profile listing and workspace filtering;
- failure after a configured alias is removed or renamed;
- remote Wardveil context detection after the actual SSH process becomes active;
- confirmation that profile metadata itself does not force a Wardveil state;
- terminal rendering, keyboard input, clipboard, disconnect, reconnect, and exit behavior;
- coexistence with direct `ssh` use outside GoreeCloud Terminal;
- coexistence with ordinary `goreecloud-terminal ssh` workflows.

No reusable key material, passwords, tokens, private addresses, or private session output should be added to public acceptance evidence.

## Remaining Milestone 4 direction

The source foundation now covers the specification's SSH launch, optional host-profile, and workspace-organization requirements.

A future graphical host/workspace selector may consume the same non-secret metadata model when doing so provides a clear usability benefit and can preserve the same fail-closed OpenSSH authority boundary.

Recent-destination persistence is intentionally not added yet. A new history store would create additional privacy, retention, and migration responsibilities and should be justified independently before implementation.

## Production boundary

The Milestone 4 administration workflow layer is source-level development. Passing CI proves argument-routing, metadata-validation, and regression behavior; it does not prove remote-host authentication, network reachability, security-policy compliance, graphical usability, or production workstation readiness.
