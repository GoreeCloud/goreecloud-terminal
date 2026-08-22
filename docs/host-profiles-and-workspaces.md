# GoreeCloud Terminal Host Profiles and Workspaces

## Purpose

This document defines the optional host-profile and workspace metadata layer used by GoreeCloud Terminal Milestone 4.

The layer exists to organize routine administration targets without creating a second SSH configuration authority, credential database, proprietary shell environment, or access-control system.

## Authority boundary

GoreeCloud Terminal host profiles contain only three pieces of metadata:

1. a user-controlled workspace label;
2. a unique local profile ID;
3. a standard OpenSSH `Host` alias.

OpenSSH remains authoritative for the connection itself.

The GoreeCloud profile file must not become the source of truth for:

- passwords;
- private keys or private-key contents;
- SSH agent material;
- real hostnames or IP addresses when an OpenSSH alias can represent them;
- usernames;
- ports;
- `ProxyJump` or proxy commands;
- forwarding rules;
- host-key policy;
- authentication policy;
- identity-file paths;
- certificates;
- tokens or other reusable secrets.

Those values remain in standard OpenSSH configuration, operating-system credential facilities, SSH agents, approved password managers, or other governing GoreeCloud security tooling.

A GoreeCloud Terminal profile is therefore an interface shortcut, not an authorization record.

## Configuration location

The default configuration file is:

```text
$XDG_CONFIG_HOME/goreecloud-terminal/profiles.tsv
```

When `XDG_CONFIG_HOME` is not set, the default is:

```text
~/.config/goreecloud-terminal/profiles.tsv
```

For isolated testing or controlled administration, the path may be overridden with:

```text
GORECLOUD_TERMINAL_PROFILES_FILE
```

The override changes only which local metadata file is read. It does not change OpenSSH configuration or credentials.

Because workspace labels and aliases may reveal administrative organization, the file should be treated as private local configuration and should normally be readable only by the owning user.

## File format

The file is UTF-8 text with exactly three TAB-separated fields per active line:

```text
WORKSPACE<TAB>PROFILE<TAB>SSH_HOST_ALIAS
```

Blank lines and lines whose first non-whitespace character is `#` are ignored.

Example using synthetic names:

```text
# Workspace<TAB>Profile<TAB>OpenSSH Host alias
Infrastructure	primary-vps	vps-admin
Virtualization	primary-hypervisor	hypervisor-admin
Storage	primary-storage	storage-admin
Development	build-host	build-admin
```

The example deliberately contains no real GoreeCloud hostname, address, username, key path, or credential.

### Workspace

The workspace is a user-controlled organizational label such as `Infrastructure`, `Virtualization`, `Storage`, or `Development`.

A workspace does not imply network membership, authorization, privilege, or trust. It is only presentation metadata.

### Profile

The profile is a unique local identifier used by launcher commands.

Allowed characters are:

```text
A-Z a-z 0-9 . _ -
```

The first character must be alphanumeric. Duplicate profile IDs are rejected so GoreeCloud Terminal cannot silently choose between ambiguous destinations.

### SSH Host alias

The third field is one standard OpenSSH `Host` alias.

The alias:

- must be a single token;
- must not contain whitespace;
- must not begin with `-`;
- is passed to the system `ssh` client without GoreeCloud rewriting.

The non-option requirement prevents a profile record from being interpreted as an OpenSSH command-line option.

## Commands

List configured workspaces:

```bash
goreecloud-terminal workspaces
```

List every configured profile:

```bash
goreecloud-terminal profiles
```

List profiles in one workspace:

```bash
goreecloud-terminal profiles Infrastructure
```

The profile listing is emitted as:

```text
PROFILE<TAB>WORKSPACE<TAB>SSH_HOST_ALIAS
```

Open one profile in a new terminal window:

```bash
goreecloud-terminal profile primary-vps
```

Open one profile in a new tab:

```bash
goreecloud-terminal profile-tab primary-vps
```

A remote command may follow the profile ID:

```bash
goreecloud-terminal profile primary-vps uname -a
```

The launcher resolves only the stored alias and then executes the equivalent standard workflow:

```text
ptyxis --new-window -- ssh vps-admin uname -a
```

OpenSSH connection options for a reusable profile should remain in standard OpenSSH configuration rather than being duplicated in the GoreeCloud profile file.

## Fail-closed validation

Before listing or launching a configured profile, the launcher validates the complete metadata file.

It refuses to proceed when it encounters:

- a malformed active line;
- a missing workspace;
- an invalid profile ID;
- a missing SSH Host alias;
- an alias beginning with `-`;
- an alias containing whitespace;
- a duplicate profile ID;
- an unknown requested workspace;
- an unknown requested profile;
- a missing profile configuration file.

Profile validation happens before the terminal runtime is launched. Invalid metadata therefore cannot silently fall through to a different destination.

## Privacy and history

The profile file is local user configuration. GoreeCloud Terminal does not upload or synchronize it and does not add telemetry for profile use.

The file should not contain reusable secrets. Even non-secret aliases and workspace labels may reveal infrastructure organization, so they should not be copied into public issues, CI logs, screenshots, or source control without review.

The current profile launcher does not create a recent-destination history database. Avoiding a new history store keeps Milestone 4 privacy behavior simple while the actual operational value of recent-destination persistence is evaluated separately.

## Relationship to Wardveil Security

Workspace/profile metadata is not used as proof that a terminal is remote, privileged, containerized, trusted, or protected.

Wardveil runtime presentation must continue to rely on verified runtime/process context. A profile launch may start an SSH process, but the profile label itself must not force the Remote indicator.

## Automated acceptance

The repository provides:

```text
tools/test-host-profiles-workspaces.sh
```

The isolated test verifies:

- stable workspace listing;
- complete profile listing;
- workspace filtering;
- new-window profile routing;
- new-tab profile routing;
- remote-command preservation;
- unknown-workspace refusal;
- unknown-profile refusal;
- duplicate-profile refusal;
- option-like alias refusal;
- missing-configuration refusal;
- no terminal runtime launch on rejected metadata.

The test uses synthetic aliases and a fake Ptyxis executable. It opens no network connection and requires no credentials.

## Runtime acceptance still required

Before host profiles are accepted for a supported workstation, validation must exercise an approved non-secret profile file against controlled OpenSSH `Host` aliases and verify:

- new-window launch;
- new-tab launch;
- normal OpenSSH host-key verification;
- approved authentication;
- expected workspace/profile listing;
- failure behavior after an alias is removed or renamed;
- Wardveil Remote detection from the actual SSH process rather than the profile metadata;
- terminal input, rendering, clipboard, disconnect, and exit behavior;
- coexistence with direct `ssh` and direct `goreecloud-terminal ssh` workflows;
- profile-file permissions and recovery behavior.

No reusable credential values should be captured in acceptance evidence.

## Production boundary

This metadata layer completes the source foundation for the optional host-profile/workspace portion of Milestone 4. Passing source and CI tests does not establish production workstation acceptance, network reachability, authentication correctness, or Stable release readiness.
