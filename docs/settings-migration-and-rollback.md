# GoreeCloud Terminal Settings Migration and Rollback

## Purpose

This document defines the controlled migration path from the inherited Ptyxis settings namespace to the GoreeCloud Terminal settings namespace.

The migration is intentionally optional and administrator-invoked. Installing or launching GoreeCloud Terminal must not automatically copy, reset, delete, or overwrite settings belonging to an installed upstream Ptyxis application.

## Namespace contract

| Role | GSettings / dconf subtree |
| --- | --- |
| Upstream Ptyxis source | `/org/gnome/Ptyxis/` |
| GoreeCloud Terminal production target | `/com/goreecloud/Terminal/` |
| GoreeCloud Terminal development target | `/com/goreecloud/Terminal/Devel/` |

The current schema layout keeps application preferences, `Profiles/<uuid>/` data, and `Shortcuts/` data beneath the same application subtree. The migration therefore copies the complete relative subtree rather than translating individual profile UUIDs or setting values.

## Tool

The repository provides:

```text
tools/migrate-ptyxis-settings.sh
```

The tool requires the `dconf` command when used against a real user account.

The default command is read-only status inspection:

```bash
tools/migrate-ptyxis-settings.sh status
```

It reports only whether custom values are present in the source and target subtrees. It does not print the settings themselves.

## Dry-run migration

A migration without `--apply` is a dry run:

```bash
tools/migrate-ptyxis-settings.sh migrate
```

The dry run verifies that the upstream source contains custom values and that the GoreeCloud production target is empty. No settings are changed.

To inspect the isolated development target instead:

```bash
tools/migrate-ptyxis-settings.sh migrate --target development
```

## Applying a migration

Production target:

```bash
tools/migrate-ptyxis-settings.sh migrate --apply
```

Development target:

```bash
tools/migrate-ptyxis-settings.sh migrate --target development --apply
```

Before writing, the tool:

1. dumps the upstream source subtree;
2. dumps the selected GoreeCloud target subtree;
3. refuses to continue if the target already contains custom values;
4. creates a private migration-state directory;
5. stores the source snapshot and pre-migration target snapshot with private file permissions.

The tool then loads the complete relative source subtree into the selected target and validates that:

- the target dump exactly matches the captured source dump;
- the upstream source dump is unchanged after migration.

If validation fails, the tool automatically restores the target snapshot captured before migration and reports the migration-state directory for investigation.

## Why migration is fail-closed

The tool intentionally does not merge into a non-empty GoreeCloud target. A merge could silently combine incompatible profile UUID lists, default-profile references, shortcuts, window state, or later GoreeCloud-specific settings.

When target settings already exist, the administrator must decide which state is authoritative rather than allowing the migration tool to guess.

This also protects users who have already configured GoreeCloud Terminal independently from upstream Ptyxis.

## Rollback

A successful migration prints a state directory such as:

```text
~/.local/state/goreecloud-terminal/settings-migrations/migrate-YYYYMMDDTHHMMSSZ-XXXXXX
```

Preview rollback without changing anything:

```bash
tools/migrate-ptyxis-settings.sh rollback \
  --state ~/.local/state/goreecloud-terminal/settings-migrations/migrate-YYYYMMDDTHHMMSSZ-XXXXXX
```

Apply rollback:

```bash
tools/migrate-ptyxis-settings.sh rollback \
  --state ~/.local/state/goreecloud-terminal/settings-migrations/migrate-YYYYMMDDTHHMMSSZ-XXXXXX \
  --apply
```

For a development-target migration, include:

```bash
--target development
```

Rollback validates that the selected target matches the target recorded by the migration manifest. Before restoring the old target snapshot, it creates another private recovery state containing the target as it existed immediately before rollback.

Rollback never resets or loads the upstream `/org/gnome/Ptyxis/` subtree.

## State-file privacy

Migration snapshots can contain user-specific information such as:

- terminal profile labels;
- custom shell commands;
- container preferences;
- custom-link regular expressions and targets;
- keyboard shortcuts;
- window and interface preferences;
- other terminal-specific configuration values.

The migration state root and snapshots are therefore created with private permissions. They must not be committed to Git, attached to public issues, copied into CI logs, or added to ordinary GoreeCloud documentation.

The migration utility reports paths and validation state, not snapshot contents.

## Upstream coexistence

Migration is copy-only from the upstream namespace. It does not remove, reset, rename, or mutate upstream Ptyxis settings. This permits an installed upstream Ptyxis application to retain its own configuration while GoreeCloud Terminal uses its separate namespace.

A later GoreeCloud setting change does not synchronize back into Ptyxis, and a later Ptyxis setting change does not automatically synchronize into GoreeCloud Terminal.

## Automated validation

The repository provides:

```text
tools/test-settings-migration.sh
```

The test uses an isolated fake dconf backend and verifies:

- status is read-only;
- migration is dry-run by default;
- a complete subtree is copied on explicit apply;
- the source remains unchanged;
- a non-empty target causes fail-closed refusal;
- rollback is dry-run by default;
- applied rollback restores the exact pre-migration target;
- rollback preserves the source;
- the development target remains isolated from the production target.

These tests validate migration control flow and safety semantics without reading or modifying a real desktop user's settings.

## Runtime acceptance still required

Source and CI success do not prove that every inherited Ptyxis preference has identical runtime meaning in GoreeCloud Terminal.

Before production migration is approved on a supported workstation, acceptance must verify representative migrated settings including:

- default and additional profiles;
- profile labels and UUID references;
- login-shell and custom-command behavior;
- scrollback settings;
- palettes, opacity, and cell spacing;
- container preferences;
- keyboard shortcuts;
- interface style;
- window/session restoration preferences;
- accessibility and terminal interaction settings.

Acceptance must also verify upstream Ptyxis coexistence before and after migration and exercise rollback using a disposable or recoverable test state.

## Production boundary

This migration layer is an acceptance candidate. It must remain opt-in until workstation testing proves the migration and rollback behavior against the supported GoreeCloud Terminal package and the actual upstream Ptyxis version selected for migration.
