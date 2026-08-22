# GoreeCloud Terminal Fork Foundation

## Status

GoreeCloud Terminal is a GoreeCloud-maintained open-source fork of Ptyxis. The repository has progressed beyond the unchanged fork baseline into GoreeCloud product identity, official artwork, Glaze UI, Wardveil Security context presentation, configuration migration/acceptance tooling, GoreeCloud administration workflows, Flatpak package acceptance, and exact-artifact lifecycle acceptance tooling. Production acceptance remains separate from source and CI validation.

## Canonical upstream

- Project: Ptyxis
- Authoritative upstream repository: https://gitlab.gnome.org/chergert/ptyxis
- Upstream hosting authority: GNOME GitLab
- GoreeCloud repository: https://github.com/GoreeCloud/goreecloud-terminal
- Initial imported upstream head: c1ba62b71295f569e0fc144b25770f2315b30e00
- Imported project version at fork foundation: 50.2

GitHub mirrors and third-party forks are not authoritative upstream sources for this project.

## Remote model

Local development clones should retain two remotes:

```text
origin   git@github.com:GoreeCloud/goreecloud-terminal.git
upstream https://gitlab.gnome.org/chergert/ptyxis.git
```

`origin` is the GoreeCloud-controlled development repository. `upstream` is the canonical Ptyxis source used for provenance, release comparison, security review, and controlled synchronization.

## Licensing and attribution

The imported source includes the GNU General Public License, version 3, in `COPYING`, while upstream application metadata declares `GPL-3.0-or-later`. Existing copyright, contributor, translator, license, and attribution records must be preserved unless a later legal review establishes that a particular item may be changed.

GoreeCloud rebranding does not authorize removal of upstream copyright or licensing obligations. Product-facing Ptyxis branding may be replaced where legally permitted, while required attribution remains available in legal, About, acknowledgments, source, history, and maintained-fork documentation surfaces.

## Development lifecycle

The approved progression is:

```text
GNOME Ptyxis upstream
        ↓
GoreeCloud-maintained fork
        ↓
GoreeCloud Terminal
        ↓
Selective fork-to-native evolution when technically justified
```

A fork-to-native transition is optional. Mature terminal-emulation, PTY, accessibility, compatibility, and security-sensitive components will not be rewritten merely for branding or ownership.

## Branch model

- `main`: controlled GoreeCloud integration branch. The initial `main` import preserves the unchanged Ptyxis baseline.
- `agent/fork-foundation`: Milestone 0 governance and CI foundation.
- `agent/terminal-artwork`: GoreeCloud Terminal artwork layer.
- `agent/glaze-ui-foundation`: Glaze UI application-chrome layer.
- `agent/wardveil-session-context`: Wardveil session-context layer.
- `agent/terminal-context-menu-actions`: GoreeCloud terminal context-menu workflow layer.
- `agent/product-identity-foundation`: canonical GoreeCloud application-ID, metadata, CLI, and repository-facing identity layer.
- `agent/runtime-identity-acceptance`: staged/installed identity acceptance harness layer.
- `agent/settings-migration-rollback`: explicit fail-closed Ptyxis-to-GoreeCloud settings migration and rollback layer.
- `agent/ssh-launch-workflows`: Milestone 4 standard OpenSSH launch workflow layer.
- `agent/host-profiles-workspaces`: optional non-secret OpenSSH-alias profile and workspace-organization layer.
- `agent/flatpak-packaging-acceptance`: Milestone 5 development Flatpak packaging and package smoke-acceptance layer.
- `agent/flatpak-lifecycle-acceptance`: Milestone 5 exact-artifact reinstall, upgrade-transition, and rollback acceptance layer.

The active development model uses narrowly scoped stacked branches and draft pull requests so each layer can be reviewed and validated independently before integration.

Direct GoreeCloud feature development should not occur on an upstream-tracking remote branch.

## Upstream synchronization

Upstream changes are reviewed, not merged blindly. A normal review cycle is:

```bash
git fetch upstream
git log --oneline --decorate --graph main..upstream/main
git diff main...upstream/main
```

Before integrating upstream work, review at minimum:

- security fixes and vulnerability-related changes;
- terminal engine, PTY, VTE, and process-tracking changes;
- GTK, libadwaita, accessibility, and input changes;
- build-system and dependency changes;
- application IDs, schemas, packaging, and Flatpak changes;
- new or reintroduced upstream branding;
- privacy, telemetry, networking, history, or persistence behavior;
- translations and user-facing strings affected by GoreeCloud divergence.

After review, integrate through a dedicated branch and pull request with build/test evidence and a documented rollback path.

## Milestone boundaries

### Milestone 0 — Fork Foundation

Preserve upstream history and licensing, document provenance and maintenance rules, establish GitHub validation, and prove the unchanged source baseline can be built and tested.

Status: source foundation implemented on the stacked development chain; integration to `main` remains pending review and acceptance.

### Milestone 1 — Product Identity

Introduce GoreeCloud Terminal naming, application identifiers, official artwork, package metadata, repository presentation, About presentation, and controlled upstream attribution.

Current source state:

- official GoreeCloud Terminal artwork is integrated;
- canonical production application ID is `com.goreecloud.Terminal`;
- canonical development application ID is `com.goreecloud.Terminal.Devel`;
- GoreeCloud GSettings namespaces are defined;
- desktop and AppStream metadata identify GoreeCloud Terminal;
- canonical `goreecloud-terminal` CLI and man page are implemented while inherited `ptyxis` remains available for compatibility;
- repository-facing README identity is GoreeCloud Terminal;
- inherited gettext domain and compatibility runtime/helper remain intentionally Ptyxis-named pending separate migration review;
- final About-dialog attribution/presentation review and production packaging acceptance remain open.

See `docs/product-identity.md`.

### Milestone 2 — Glaze UI

Adopt the stable GoreeCloud Glaze UI baseline across application chrome and controls while preserving terminal readability, accessibility, performance, and native GTK behavior.

Status: initial source foundation implemented and awaiting complete stacked-branch runtime/integration acceptance.

### Milestone 3 — Wardveil Security

Introduce clear and non-disruptive security context for SSH, elevated/root, container, and other sensitive terminal environments.

Status: typed source model and initial graphical runtime evidence are implemented; detector-driven transition, accessibility, palette, and final integrated-product acceptance remain open.

### Milestone 4 — GoreeCloud Administration Workflows

Add optional host profiles, SSH launch workflows, workspace organization, and GoreeCloud infrastructure conveniences without replacing standard OpenSSH or shell configuration.

Current source state:

- `goreecloud-terminal ssh OPENSSH_ARGUMENT ...` launches the system OpenSSH client in a new GoreeCloud Terminal window while preserving normal OpenSSH argument ordering;
- `goreecloud-terminal ssh-tab OPENSSH_ARGUMENT ...` launches the same standard OpenSSH workflow in a new tab;
- optional `profiles.tsv` metadata stores only a workspace label, unique profile ID, and OpenSSH `Host` alias;
- `goreecloud-terminal workspaces` and `goreecloud-terminal profiles [WORKSPACE]` expose user-controlled organization without redefining access-control boundaries;
- `goreecloud-terminal profile PROFILE` and `profile-tab PROFILE` resolve only the stored OpenSSH alias and then launch the standard system `ssh` client;
- malformed rows, duplicate profile IDs, option-like aliases, unknown profiles/workspaces, and missing profile configuration fail before the runtime starts;
- OpenSSH remains authoritative for actual hostnames, usernames, ports, private keys, agents, host-key policy, proxy configuration, forwarding, and authentication;
- isolated SSH and host-profile/workspace tests validate the source behavior without opening network connections or using credentials;
- recent-destination persistence is intentionally not implemented because it would create additional privacy and retention obligations;
- real controlled-host runtime acceptance and any future graphical host/workspace selector remain separate acceptance/development layers.

See `docs/administration-workflows.md` and `docs/host-profiles-and-workspaces.md`.

### Milestone 5 — Packaging and Acceptance

Produce installable Linux builds, complete functional/security testing, validate upgrades and rollback, and perform workstation acceptance testing.

Current source state:

- a first-party development Flatpak manifest is defined as `com.goreecloud.Terminal.Devel.json`;
- the package uses GNOME Platform/SDK 50 and the isolated `com.goreecloud.Terminal.Devel` identity so acceptance testing cannot collide with a future production identity;
- the manifest uses `goreecloud-terminal` as the canonical command and enables the inherited Flatpak-specific libc compatibility path for `ptyxis-agent`;
- external Flatpak support dependencies are pinned by exact commit or checksum, bundled libraries use the Flatpak `/app/lib` layout, and the GoreeCloud application build disables implicit Meson dependency downloads;
- terminal-specific Flatpak permissions are explicit and documented, including the broad host-filesystem and Flatpak host-integration permissions that require later minimization review rather than being treated as invisible defaults;
- `.github/workflows/flatpak-acceptance.yml` builds an exact-head bundle, validates application icon composition, installs it in CI, inspects identity/runtime metadata, runs non-graphical canonical-launcher smoke checks, uninstalls it, verifies removal, calculates SHA-256, validates an exact-artifact reinstall lifecycle, and retains the exact `.flatpak` bundle as temporary acceptance evidence;
- `tools/validate-flatpak-lifecycle.sh` accepts only the isolated development application identity and requires explicit local bundles plus caller-supplied SHA-256 values before installation;
- reinstall mode verifies exact-artifact install, packaged smoke behavior, identical OSTree commit after reinstall, and clean removal without `--delete-data`;
- transition mode requires cryptographically distinct baseline/candidate bundles and distinct installed OSTree commits, then requires rollback to restore the exact recorded baseline commit;
- `tools/test-flatpak-lifecycle.sh` validates lifecycle fail-closed behavior with a fake Flatpak backend, including production-ID refusal, hash mismatch refusal, identical-artifact transition refusal, pre-existing-installation refusal, exact rollback, and cleanup after smoke failure;
- a real cross-version upgrade/rollback claim is intentionally pending a second distinct accepted package candidate; fake-backend transition tests do not substitute for that evidence;
- native distribution packaging remains a later option and must not force unsafe replacement of host desktop libraries merely to satisfy GoreeCloud Terminal dependencies;
- graphical workstation acceptance, real host/SSH/container behavior, permission minimization, real cross-version package transition, data compatibility after rollback, and Stable release authorization remain open.

See `docs/flatpak-packaging-and-acceptance.md` and `docs/flatpak-upgrade-and-rollback.md`.

### Milestone 6 — Selective Native Evolution

Evaluate inherited components individually and replace only those for which a GoreeCloud-native implementation provides a documented material benefit.

Status: no broad rewrite is authorized merely for branding or ownership. Mature terminal-emulation and security-sensitive foundations remain inherited unless a later controlled evaluation justifies replacement.

## Compatibility boundary

The executable `ptyxis`, helper `ptyxis-agent`, inherited gettext domain `ptyxis`, and several internal source filenames remain compatibility-sensitive implementation details. They do not define the canonical product name.

Renaming them is not required merely to make the product visibly GoreeCloud Terminal. Any later rename must be justified by practical benefit and validated against desktop launch, D-Bus activation, Flatpak/host helper behavior, shell usage, packaging, upgrades, and rollback.

## Production boundary

Source import, successful CI, a generated Flatpak bundle, successful package reinstall, fake-backend lifecycle tests, or successful local builds do not by themselves make GoreeCloud Terminal production-ready. Production acceptance requires later functional, security, accessibility, packaging, permission, real cross-version upgrade, exact-artifact rollback, supported-workstation, settings-migration, SSH-workflow, host-profile/workspace, data-compatibility, and runtime validation.
