# GoreeCloud Terminal Flatpak Upgrade and Rollback Acceptance

## Purpose

This document defines the Milestone 5 package-lifecycle acceptance model for GoreeCloud Terminal.

Package creation and package lifecycle are separate acceptance concerns. A bundle can build and install successfully while still having an unsafe or unverified reinstall, replacement, upgrade, or rollback path. GoreeCloud Terminal therefore treats exact-artifact lifecycle validation as its own gate.

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

## Flatpak 1.14.6 local-bundle compatibility

The first real lifecycle CI run exposed an important Flatpak-version behavior on the Ubuntu 24.04 acceptance runner. Flatpak 1.14.6 successfully installed the local `.flatpak` bundle, but a second command using:

```text
flatpak install --reinstall local-bundle.flatpak
```

returned that the application was already installed instead of replacing the installed local bundle.

GoreeCloud Terminal does not hide this behavior and does not weaken artifact verification to work around it. For local single-file bundle acceptance, the lifecycle harness therefore uses the portable exact-artifact replacement sequence that the tested Flatpak baseline supports:

1. verify the local bundle SHA-256 before installation;
2. uninstall only the application ref **without** `--delete-data`;
3. verify that the application ref is absent;
4. install the already-verified exact local bundle;
5. validate the installed runtime, application ref, OSTree commit, and non-graphical product smoke checks.

The application data directory is deliberately not removed. CI additionally places a synthetic marker beneath `~/.var/app/com.goreecloud.Terminal.Devel` and requires that marker to survive exact-artifact replacement and final application removal.

This behavior is an exact local-bundle replacement/reinstall acceptance path. It is **not** equivalent to a repository-backed `flatpak update`, and it must not be described as normal in-place distribution upgrade acceptance.

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
8. removes the application ref without deleting application data;
9. verifies that the application ref is absent;
10. installs the exact same already-verified bundle again;
11. requires the installed OSTree commit to match the original exact commit;
12. repeats the non-graphical identity smoke checks;
13. removes the application ref again without `--delete-data`;
14. verifies that the application is no longer installed.

A successful reinstall test proves exact-artifact remove/install replacement and ordinary removal behavior while preserving Flatpak application data. It does **not** prove cross-version repository-backed upgrade or rollback.

## Replacement-transition mode

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
5. removes only the baseline application ref while preserving data;
6. installs and smoke-tests the exact candidate bundle;
7. requires the candidate OSTree commit to differ from the baseline commit;
8. removes only the candidate application ref while preserving data;
9. reinstalls the exact baseline artifact;
10. requires the installed commit after rollback to exactly match the original baseline commit;
11. smoke-tests the restored baseline;
12. removes the application ref and verifies removal without deleting user data.

If the two bundle hashes or installed OSTree commits are identical, the harness refuses to describe the operation as an accepted distinct-artifact transition. This prevents an identical rebuild or duplicate artifact from being misrepresented as cross-version evidence.

A successful two-bundle transition would prove exact local-artifact replacement and exact-artifact rollback semantics. It still would not, by itself, prove normal repository-backed `flatpak update` behavior used by a future distribution channel.

## Current acceptance state

The first accepted development package candidate has complete build/bundle/install/remove CI evidence, but there is currently only one approved exact package candidate in the lifecycle evidence set.

Therefore:

- exact-artifact remove/install reinstall can be validated now;
- ordinary uninstall without user-data deletion can be validated now;
- synthetic Flatpak application-data preservation can be validated now;
- the distinct-artifact replacement-transition algorithm can be tested against an isolated fake Flatpak backend now;
- a **real cross-version upgrade and rollback claim remains pending a second distinct accepted package candidate**;
- repository-backed `flatpak update` acceptance remains a separate future distribution-lifecycle gate.

The repository must not manufacture a second package merely to satisfy an acceptance checkbox. The next package candidate should arise from legitimate reviewed source changes, then be retained with its exact source revision and SHA-256 before transition acceptance is run.

## User-data boundary

The lifecycle harness deliberately does not use:

```text
flatpak uninstall --delete-data
```

Ordinary package removal, exact-artifact replacement, or rollback must not silently destroy GoreeCloud Terminal user data.

Settings migration and configuration rollback remain separate controlled mechanisms. Package rollback must not be treated as permission to overwrite or delete dconf state, profile metadata, SSH configuration, shell configuration, scrollback, or other user-controlled data.

A later workstation acceptance plan must explicitly test how settings created by a newer candidate behave after rollback to an older candidate.

## Failure cleanup

If lifecycle validation fails after the harness has installed the development application, the harness attempts to remove the application ref it created while preserving user data.

The cleanup path does not override the fail-closed pre-existing-installation rule. If an installation existed before the harness started, the harness refuses to begin rather than claiming ownership of that installation.

If an exact-artifact replacement fails after the prior app ref has already been removed, the harness does not fabricate recovery success. The failure remains visible and the retained, hash-verified bundle is the recovery input for the next explicit action.

## Automated contract tests

The repository provides:

```text
tools/test-flatpak-lifecycle.sh
```

The test uses a fake Flatpak backend and synthetic non-secret bundle files. The fake backend intentionally models the observed local-bundle limitation by rejecting `--reinstall` and refusing a second install while an app ref already exists. This ensures the harness continues to remove the app ref explicitly before installing another local bundle.

The tests validate:

- exact-artifact remove/install reinstall success;
- distinct baseline/candidate replacement transition and exact rollback;
- synthetic user-data preservation across all lifecycle replacements;
- development-only application-ID enforcement;
- SHA-256 mismatch refusal;
- malformed 64-character digest refusal;
- identical-artifact transition refusal;
- pre-existing-installation refusal without modification;
- cleanup after packaged smoke-check failure;
- no use of unsupported local-bundle `--reinstall`;
- the prohibition on destructive `--delete-data` lifecycle removal.

These tests validate the harness logic without installing software, opening network connections, or using credentials.

## CI package lifecycle

The development Flatpak acceptance workflow runs reinstall mode against the exact package bundle it has just built.

Before the lifecycle harness runs, CI creates a synthetic marker under:

```text
~/.var/app/com.goreecloud.Terminal.Devel
```

After the exact-artifact replacement and final application removal complete, CI requires that marker to still exist with its original content. The probe is then removed explicitly. This proves that the tested lifecycle path removes only the application ref and does not erase the Flatpak application-data directory.

This ties reinstall evidence to the same exact source revision and bundle digest as the package build. The workflow must continue to retain the exact bundle and digest as temporary CI evidence.

When a second accepted package candidate exists, a separate transition acceptance run should supply both retained exact artifacts and their recorded digests. That run must be evidence-bound to both source revisions and both bundle hashes.

A future distribution channel must additionally validate repository-backed update and rollback semantics rather than treating local-bundle replacement as a substitute for `flatpak update`.

## Security and privacy

Lifecycle evidence may record:

- source revision;
- workflow run ID;
- bundle filenames;
- SHA-256 values;
- Flatpak OSTree commits;
- runtime/application identity;
- pass/fail results;
- synthetic data-probe status.

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
- cross-version transition from an accepted baseline to a distinct accepted candidate;
- exact-artifact rollback to the accepted baseline;
- repository-backed update/rollback behavior for the chosen distribution channel;
- data compatibility after rollback;
- coexistence with upstream Ptyxis where intentionally supported;
- clean package removal without unintended user-data loss.

## Production boundary

A green lifecycle harness proves only the lifecycle operations it actually executed against the supplied exact development artifacts.

Exact local-bundle remove/install reinstall is not repository-backed upgrade acceptance. Fake-backend transition tests are not real package transition acceptance. A future two-bundle replacement transition is not supported-workstation or distribution-channel acceptance. Stable promotion requires the remaining runtime, security, accessibility, configuration, permission, cross-version, rollback, repository-update, and workstation gates defined by the GoreeCloud Terminal project specification.
