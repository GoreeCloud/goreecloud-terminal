# GoreeCloud Terminal Release Candidate Readiness

## Candidate

The current GoreeCloud Terminal candidate line is `50.2-rc.2`. The upstream foundation remains Ptyxis 50.2.

Release Candidate means this identified GoreeCloud source and package line is intended to become Stable if final acceptance finds no blocking defect or unmet requirement. It does not mean Stable, production-approved, or generally deployed.

`50.2-rc.2` is a separately versioned follow-up candidate. The published `50.2-rc.1` tag, prerelease, source revision, bundle, checksum, and OSTree identity remain immutable historical evidence.

The authoritative machine-readable source state is `release/status.json`.

## Required automated RC gates

One exact candidate revision must pass all applicable source/package contracts without source drift, including:

1. Fork Foundation source/provenance/build/test validation.
2. Flatpak Lifecycle Contract validation.
3. Development Flatpak package acceptance.
4. Release Readiness lifecycle, Privacy Shield, local API, Glaze UI, and privacy validation.
5. Production Identity RC Flatpak acceptance using `com.goreecloud.Terminal`.
6. Wardveil Accessibility Contract validation.
7. Supported Workstation Acceptance Contract validation.
8. Flatpak Permission Contract validation.
9. Production no-host-filesystem candidate/transition validation while that narrower permission candidate remains under evaluation.
10. No unresolved source-level release blocker introduced by the RC changes.

A passing gate on an older commit is not acceptance evidence for a newer candidate.

## Production-identity RC package

The canonical RC package manifest is:

```text
com.goreecloud.Terminal.json
```

It uses:

- production application ID `com.goreecloud.Terminal`;
- canonical launcher `goreecloud-terminal`;
- GNOME Platform/SDK 50;
- `-Ddevelopment=false`;
- the same pinned Flatpak support dependencies as the accepted development package;
- explicit terminal-oriented permissions that remain subject to final minimization review.

The RC2 source-preparation line does not silently adopt the no-host-filesystem candidate. The canonical production/development manifests remain aligned while supported-workstation permission-minimization acceptance is incomplete.

The RC bundle is acceptance material, not a Stable release.

## Source-readiness contracts

### Glaze UI

The RC targets Glaze UI 1.4.0 Stable through a native GTK/libadwaita mapping. Terminal rendering remains under VTE control. Source defines explicit light presentation plus reduced-motion and increased-contrast behavior while preserving native GTK controls and accessibility semantics. Explicit supported-workstation palette/transparency, contrast, and reduced-motion acceptance remains open where applicable.

### Wardveil Security

The typed Local, Remote, Container, and Elevated presentation model is present. Wardveil is context presentation only and is not an authorization authority.

The post-RC1 semantic-label correction passed live supported-workstation exact AT-SPI accessible-name verification for `Remote terminal session`, `Containerized terminal session`, and `Elevated superuser terminal session` under their corresponding real detector-driven states before it was merged to authoritative source. The exact RC2 package must preserve those accepted names and Local/unknown hidden behavior during follow-up regression acceptance.

### Privacy Shield

The RC declares only `telemetry-minimization` and `data-minimization`. It is local-first, adds no GoreeCloud analytics or remote tracker telemetry, and excludes private terminal/session material from release evidence. Runtime acceptance remains required and production approval remains false.

### Local API

`goreecloud-terminal api status` is the supported built-in API. It is read-only, local CLI JSON, schema-versioned, and intentionally contains no terminal contents, commands, credentials, host aliases, or identifiers. RC2 reports `release_candidate` as `50.2-rc.2`.

### Flatpak permission transition

The source includes an isolated generator that derives a production-identity candidate from `com.goreecloud.Terminal.json` and permits exactly one temporary manifest delta: removal of `--filesystem=host`.

The repository-backed transition workflow verifies the immutable published RC1 baseline, builds a distinct candidate, performs a real Flatpak repository update, verifies host-filesystem access is absent, rolls back to the exact published baseline OSTree commit, and verifies a synthetic application-data marker survives update, rollback, and ordinary removal.

That automated transition evidence is not sufficient to modify the canonical production manifest. Affected supported-workstation PTY, shell, working-directory, external-storage, host-agent, container, graphics/display, clipboard, OpenSSH, coexistence, and recovery behavior must still pass before the narrower permission set can be adopted.

### Observability

Release tooling records non-secret source/package identity and pass/fail evidence. It does not add terminal-content telemetry. Inherited local GLib/GTK/VTE diagnostics remain available for troubleshooting.

## Evidence matrix

Automated source/package evidence can prove:

- exact source identity;
- maintained-fork provenance;
- compile/test success;
- staged identity consistency;
- development and production Flatpak buildability;
- package metadata and application identity;
- non-graphical launcher/API/SSH-help/profile-help smoke behavior;
- exact local-bundle hashes and OSTree commits;
- development exact-artifact replacement/removal behavior without deleting application data;
- source-level Privacy Shield, Glaze UI, Wardveil accessibility, and Flatpak permission-review contracts;
- an isolated production no-host-filesystem candidate differs only by the authorized permission removal;
- repository-backed update to a distinct candidate and exact rollback to the published RC1 baseline in CI;
- synthetic application-data marker preservation through that automated transition.

Automated CI cannot by itself prove:

- exact RC2 supported-workstation graphical behavior;
- exact RC2 real terminal input/rendering/font/Unicode/clipboard regression behavior;
- exact RC2 preservation of live Wardveil assistive-technology behavior;
- final light/dark/transparency/contrast/reduced-motion appearance and behavior;
- settings migration/persistence on an approved workstation;
- permission minimization against real user workflows;
- production-package coexistence and recovery on the supported workstation;
- post-rollback compatibility of real settings/profile/non-secret application data;
- crash/recovery acceptance.

## Stable blockers

`release/status.json` records the current Stable blockers explicitly. They are acceptance work, not documentation placeholders. Stable and production approval must remain false until every applicable blocker is resolved with evidence.

No automation may convert RC to Stable merely because CI is green.

## Rollback boundary

The development lifecycle harness proves exact verified local-bundle replacement using ordinary Flatpak removal without `--delete-data`, followed by installation of the exact verified artifact. This matches real Flatpak 1.14.6 local-bundle behavior.

The production transition harness additionally proves repository-backed update to a cryptographically distinct generated candidate and exact repository rollback to the published RC1 baseline in CI. A Stable cross-version claim still requires the selected release packages and applicable supported-workstation data-compatibility/recovery evidence.

## Promotion rule

The source may be recorded as Release Candidate only when:

- the final exact candidate revision passes every required RC workflow;
- the production-identity RC artifact and its digest are retained as evidence;
- repository and GoreeCloud Drive lifecycle documentation identify the same candidate/source;
- a rollback point is identifiable;
- no known source-level blocker remains.

Publication of RC2 additionally requires exact package identity, applicable supported-workstation regression evidence, governed tag/prerelease creation, and independent post-publication tag/artifact verification.

Stable promotion requires the remaining runtime, configuration, permission, package, rollback-data-compatibility, recovery, and supported-workstation acceptance gates.
