# GoreeCloud Terminal Product Identity

## Purpose

This document defines the source-level product identity for GoreeCloud Terminal while preserving the provenance, licensing, and authorship of the upstream Ptyxis foundation.

GoreeCloud Terminal is a GoreeCloud-maintained open-source fork of Ptyxis. Product identity changes do not convert inherited Ptyxis code into original GoreeCloud work and do not remove upstream copyright, contributor, translator, or license records.

## Canonical identity

| Surface | Production | Development |
| --- | --- | --- |
| Product name | GoreeCloud Terminal | GoreeCloud Terminal |
| Application ID | `com.goreecloud.Terminal` | `com.goreecloud.Terminal.Devel` |
| GSettings schema prefix | `com.goreecloud.Terminal` | `com.goreecloud.Terminal.Devel` |
| GSettings path | `/com/goreecloud/Terminal/` | `/com/goreecloud/Terminal/Devel/` |
| Canonical CLI launcher | `goreecloud-terminal` | `goreecloud-terminal` |
| Compatibility runtime binary | `ptyxis` | `ptyxis` |
| Repository | `GoreeCloud/goreecloud-terminal` | `GoreeCloud/goreecloud-terminal` |
| License | GPL-3.0-or-later | GPL-3.0-or-later |

The application ID is the authority for installed desktop files, D-Bus activation, icons, AppStream metadata, and the default GSettings namespace.

The canonical user-facing command is `goreecloud-terminal`. It is a deliberately thin launcher that executes the inherited `ptyxis` runtime with the original arguments unchanged. This establishes a GoreeCloud-owned command-line entry point without forcing a premature rename of compatibility-sensitive native binaries.

## Upstream compatibility boundary

The current native executable remains named `ptyxis`, and the helper remains `ptyxis-agent`. Those names are retained as compatibility-sensitive implementation details inherited from upstream. They are not the canonical user-facing GoreeCloud product name.

The installed `goreecloud-terminal` launcher delegates directly to `ptyxis` and preserves argument behavior. Desktop launch, desktop actions, and D-Bus service activation use the GoreeCloud launcher. The launcher then hands execution to the compatibility runtime, preserving the existing native process and helper boundary.

Removing or renaming the compatibility executable and helper entry points is a separate packaging/runtime migration because it can affect:

- shell scripts and command-line habits;
- D-Bus and single-instance behavior;
- host/Flatpak helper discovery;
- process detection and diagnostics;
- packaging manifests;
- downstream integrations;
- upgrade and rollback behavior.

The compatibility binary names must not be removed until a controlled migration proves that host, container, Flatpak, desktop-launch, command-line, and rollback workflows remain functional.

## Translation boundary

The inherited gettext domain remains `ptyxis` during this stage. This preserves the existing translation catalog and avoids silently discarding upstream translations. GoreeCloud-specific strings may continue to use that catalog until a deliberate translation-domain migration is designed and validated.

## Metadata boundary

Installed AppStream and desktop metadata identify the application as GoreeCloud Terminal and point maintenance/issue traffic to the GoreeCloud repository.

The metadata must still state that GoreeCloud Terminal is based on Ptyxis. The source repository continues to retain the upstream project URL, imported baseline, license, copyright records, contributor history, and translation provenance.

Upstream screenshots and upstream release entries are not presented as GoreeCloud release evidence. GoreeCloud-specific screenshots and release records must be added only when captured or issued from a validated GoreeCloud build.

## Settings migration risk

Changing the default application and GSettings identifiers intentionally separates GoreeCloud Terminal state from upstream Ptyxis state. Existing settings stored under `org.gnome.Ptyxis` are therefore not automatically treated as GoreeCloud Terminal settings.

Before production replacement, acceptance must determine whether a one-time, explicit migration is desirable for supported preferences and profiles. Any migration must be versioned, bounded, reversible, and must not overwrite the upstream namespace.

## Required acceptance

Source/CI acceptance must verify at minimum:

- production identity resolves to `com.goreecloud.Terminal`;
- development identity resolves to `com.goreecloud.Terminal.Devel`;
- staged desktop, D-Bus, AppStream, icon, and GSettings artifacts use the GoreeCloud application ID;
- AppStream validation passes;
- desktop-file validation passes;
- inherited gettext catalogs still compile;
- native compilation and tests pass;
- `goreecloud-terminal` is installed and executable;
- the GoreeCloud launcher forwards arguments to the inherited runtime without modification;
- desktop and D-Bus service launch paths resolve through `goreecloud-terminal`;
- the staged executable and helper remain available under their current compatibility names.

Runtime acceptance must additionally verify:

- launch from the desktop application menu;
- D-Bus activation and single-instance behavior through the GoreeCloud launcher;
- direct `goreecloud-terminal` CLI launch;
- direct compatibility `ptyxis` launch;
- new-window and new-tab desktop actions;
- preferences persistence under the GoreeCloud schema namespace;
- default-terminal integration;
- icon resolution in normal and development builds;
- no collision with an installed upstream Ptyxis application;
- upgrade and rollback behavior;
- light/dark Glaze UI presentation;
- Wardveil context presentation;
- terminal rendering, input, clipboard, selection, SSH, container, and sudo behavior.

## Production boundary

This identity layer is source-level development work. Successful CI does not by itself authorize Stable or production deployment. Production acceptance remains blocked until a compiled GoreeCloud build is exercised on a supported workstation and the packaging, settings, launch, upgrade, rollback, and runtime checks above are recorded.
