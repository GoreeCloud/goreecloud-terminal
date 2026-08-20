# GoreeCloud Terminal Fork Foundation

## Status

GoreeCloud Terminal is a GoreeCloud-maintained open-source fork of Ptyxis. This repository is in the fork-foundation phase; the imported application remains substantially upstream Ptyxis until GoreeCloud-specific product identity, Glaze UI, Wardveil Security, and workflow changes are implemented and validated.

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

GoreeCloud rebranding does not authorize removal of upstream copyright or licensing obligations. Product-facing Ptyxis branding may be replaced during the product-identity milestone where legally permitted, while required attribution remains available in appropriate legal, About, acknowledgments, source, or history surfaces.

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
- `agent/fork-foundation`: Milestone 0 development and validation branch.
- Future feature branches should remain narrowly scoped and merge through reviewed pull requests.

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

### Milestone 1 — Product Identity

Introduce GoreeCloud Terminal naming, application identifiers, official artwork, package metadata, About presentation, and controlled upstream attribution.

### Milestone 2 — Glaze UI

Adopt the stable GoreeCloud Glaze UI baseline across application chrome and controls while preserving terminal readability, accessibility, performance, and native GTK behavior.

### Milestone 3 — Wardveil Security

Introduce clear and non-disruptive security context for SSH, elevated/root, container, and other sensitive terminal environments.

## Production boundary

Source import, successful CI, or successful local builds do not by themselves make GoreeCloud Terminal production-ready. Production acceptance requires later functional, security, accessibility, packaging, upgrade, rollback, and supported-workstation validation.
