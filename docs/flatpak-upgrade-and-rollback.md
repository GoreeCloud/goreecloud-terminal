# GoreeCloud Terminal Flatpak Upgrade and Rollback Acceptance

## Purpose

This document defines the Milestone 5 package-lifecycle acceptance model for GoreeCloud Terminal.

Package creation and package lifecycle are separate acceptance concerns. A bundle can build and install successfully while still having an unsafe or unverified reinstall, upgrade, or rollback path. GoreeCloud Terminal therefore treats exact-artifact lifecycle validation as its own gate.

The initial lifecycle tooling is restricted to the isolated development application identity:

```text
com.goreecloud.Terminal.Devel
```

It does not authorize production-package modification, Stable promotion, or workstation replacement.

## Tool

The lifecycle harness is:

```text
tools/validate-flatpak-lifecycle.sh
```

The harness accepts only explicit local `.flatpak` bundle paths and caller-supplied SHA-256 values. It verifies each bundle cryptographically before installation.

The harness refuses to start when `com.goreecloud.Terminal.Devel` is already installed. This fail-closed rule prevents CI-oriented acceptance tooling from silently modifying an unrelated pre-existing installation.

The Flatpak executable may be overridden with `GORECLOUD_TERMINAL_FLATPAK_BIN` for isolated automated tests. Normal acceptance uses the system `flatpak` command.

## Development-identity boundary

The first lifecycle harness intentionally accepts only:

```text
com.goreecloud.Terminal.Devel
```

Passing `com.goreecloud.Terminal` or another application ID is an error.

Production lifecycle tooling may be designed later, but it must have a separate approval path and must not inherit development-package assumptions automatically.

## Reinstall mode

Reinstall mode validates one exact artifact:

```bash
tools/validate-flatpak-lifecycle.sh reinstall \
  --bundle goreecloud-terminal-devel-<source>.flatpak \
  --sha256 <expected-sha256>
```

The harness:

1. verifies that the bundle exists;
2. verifies the exact SHA-256 before Flatpak is invoked;
3. refuses a pre-existing development installation;
4. installs the exact bundle into the user Flatpak installation;
5. records the installed OSTree commit;
6. validates GNOME Platform 50 identity and the development application ref;
7. runs non-graphical canonical-launcher, SSH-help, and profile-help smoke checks without making an SSH connection;
8. reinstalls the exact same bundle;
9. requires the installed OSTree commit to remain identical;
10. repeats the non-graphical identity smoke checks;
11. uninstalls the application without deleting user data;
12. verifies that the application is no longer installed.

A successful reinstall test proves exact-artifact reinstall and ordinary removal behavior. It does **not** prove cross-version upgrade or rollback.

## Transition mode

Transition mode is reserved for two cryptographically distinct accepted package artifacts:

```bash
tools/validate-flatpak-lifecycle.sh transition \
  --baseline baseline.flatpak \
  --baseline-sha256 <baseline-sha256> \
  --candidate candidate.flatpak \
  --candidate-sha256 <candidate-sha256>
```

The harness:

1. verifies both bundle hashes before installation;
2. requires the two bundle SHA-256 values to differ;
3. installs and smoke-tests the baseline;
4. records the exact baseline OSTree commit;
5. installs the candidate through Flatpak's reinstall path and smoke-tests it;
6. requires the candidate OSTree commit to differ from the baseline commit;
7. reinstalls the exact baseline artifact;
8. requires the installed commit after rollback to exactly match the original baseline commit;
9. smoke-tests the restored baseline;
10. uninstalls the application and verifies removal.

If the two bundle hashes or installed OSTree commits are identical, the harness refuses to describe the operation as an accepted upgrade. This prevents an identical rebuild or duplicate artifact from being misrepresented as cross-version lifecycle evidence.

## Current acceptance state

The first accepted development package candidate has complete build/bundle/install/remove CI evidence, but there is currently only one approved exact package candidate in the lifecycle evidence set.

Therefore:

- exact-artifact reinstall can be validated now;
- ordinary uninstall without user-data deletion can be validated now;
- the transition algorithm can be tested against an isolated fake Flatpak backend now;
- a **real cross-version upgrade and rollback claim remains pending a second distinct accepted package candidate**.

The repository must not manufacture a second package merely to satisfy an acceptance checkbox. The next package candidate should arise from legitimate reviewed source changes, then be retained with its exact source revision and SHA-256 before transition acceptance is run.

## User-data boundary

The lifecycle harness deliberately does not use:

```text
flatpak uninstall --delete-data
```

Ordinary package removal or rollback must not silently destroy GoreeCloud Terminal user data.

Settings migration and configuration rollback remain separate controlled mechanisms. Package rollback must not be treated as permission to overwrite or delete dconf state, profile metadata, SSH configuration, shell configuration, scrollback, or other user-controlled data.

A later workstation acceptance plan must explicitly test how settings created by a newer candidate behave after rollback to an older candidate.

## Failure cleanup

If lifecycle validation fails after the harness has installed the development application, the harness attempts to remove the installation it created while preserving user data.

The cleanup path does not override the fail-closed pre-existing-installation rule. If an installation existed before the harness started, the harness refuses to begin rather than claiming ownership of that installation.

## Automated contract tests

The repository provides:

```text
tools/test-flatpak-lifecycle.sh
```

The test uses a fake Flatpak backend and synthetic non-secret bundle files. It validates:

- exact-artifact reinstall success;
- distinct baseline/candidate transition and exact rollback;
- development-only application-ID enforcement;
- SHA-256 mismatch refusal;
- identical-artifact transition refusal;
- pre-existing-installation refusal;
- cleanup after packaged smoke-check failure;
- the prohibition on destructive `--delete-data` lifecycle removal.

These tests validate the harness logic without installing software, opening network connections, or using credentials.

## CI package lifecycle

The development Flatpak acceptance workflow uses reinstall mode against the exact package bundle it has just built.

This ties reinstall evidence to the same exact source revision and bundle digest as the package build. The workflow must continue to retain the exact bundle and digest as temporary CI evidence.

When a second accepted package candidate exists, a separate transition acceptance run should supply both retained exact artifacts and their recorded digests. That run must be evidence-bound to both source revisions and both bundle hashes.

## Security and privacy

Lifecycle evidence may record:

- source revision;
- workflow run ID;
- bundle filenames;
- SHA-256 values;
- Flatpak OSTree commits;
- runtime/application identity;
- pass/fail results.

It must not record reusable passwords, private keys, private SSH configuration, tokens, private terminal contents, or other secrets.

The lifecycle harness does not perform SSH connections, execute privileged administrative commands, migrate settings automatically, or infer host trust.

## Required workstation acceptance

CI lifecycle acceptance is not workstation acceptance. Before Stable promotion, an approved workstation must still validate:

- installation from the exact approved artifact;
- graphical application launch and D-Bus activation;
- local shell and PTY behavior;
- rendering, Unicode/fonts, clipboard, shortcuts, tabs, accessibility, and crash recovery;
- direct and profile-based OpenSSH behavior;
- containers and inherited host-helper integration;
- Wardveil runtime transitions and Glaze UI presentation;
- settings persistence and controlled migration;
- permission minimization;
- reinstall behavior with real user state;
- cross-version upgrade from an accepted baseline;
- exact-artifact rollback to the accepted baseline;
- data compatibility after rollback;
- coexistence with upstream Ptyxis where intentionally supported;
- clean package removal without unintended user-data loss.

## Production boundary

A green lifecycle harness proves only the lifecycle operations it actually executed against the supplied exact development artifacts.

Reinstall acceptance is not upgrade acceptance. Fake-backend transition tests are not real package transition acceptance. A future two-bundle transition is not supported-workstation acceptance. Stable promotion requires the remaining runtime, security, accessibility, configuration, permission, upgrade, rollback, and workstation gates defined by the GoreeCloud Terminal project specification.
