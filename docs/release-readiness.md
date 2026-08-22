# GoreeCloud Terminal Release Candidate Readiness

## Candidate

The current GoreeCloud Terminal candidate line is `50.2-rc.1`. The upstream foundation remains Ptyxis 50.2.

Release Candidate means this identified GoreeCloud source and package line is intended to become Stable if final acceptance finds no blocking defect or unmet requirement. It does not mean Stable, production-approved, or generally deployed.

The authoritative machine-readable source state is `release/status.json`.

## Required automated RC gates

One exact candidate revision must pass all of the following without source drift:

1. Fork Foundation source/provenance/build/test validation.
2. Flatpak Lifecycle Contract validation.
3. Development Flatpak package acceptance.
4. Release Readiness lifecycle, Privacy Shield, local API, Glaze UI, and privacy validation.
5. Production Identity RC Flatpak acceptance using `com.goreecloud.Terminal`.
6. No unresolved source-level release blocker introduced by the RC changes.

A passing gate on an older commit is not acceptance evidence for a newer candidate.

## Production-identity RC package

The RC package manifest is:

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

The RC bundle is acceptance material, not a Stable release.

## Source-readiness contracts

### Glaze UI

The RC targets Glaze UI 1.4.0 Stable through a native GTK/libadwaita mapping. Terminal rendering remains under VTE control. The RC source defines explicit light presentation plus reduced-motion and increased-contrast behavior while preserving native GTK controls and accessibility semantics.

### Wardveil Security

The typed Local, Remote, Container, and Elevated presentation model is present. Wardveil is context presentation only and is not an authorization authority. Detector-driven real-runtime transitions and mixed-tab behavior remain final acceptance work.

### Privacy Shield

The RC declares only `telemetry-minimization` and `data-minimization`. It is local-first, adds no GoreeCloud analytics or remote tracker telemetry, and excludes private terminal/session material from release evidence. Runtime acceptance remains required and production approval remains false.

### Local API

`goreecloud-terminal api status` is the supported built-in API. It is read-only, local CLI JSON, schema-versioned, and intentionally contains no terminal contents, commands, credentials, host aliases, or identifiers.

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
- source-level Privacy Shield and Glaze UI contracts.

Automated CI cannot by itself prove:

- supported-workstation graphical behavior;
- real terminal input/rendering/font/Unicode/clipboard behavior;
- actual OpenSSH authentication and host-key flows;
- real container/elevated/remote Wardveil transitions;
- accessibility with a real assistive-technology stack;
- final light/dark/transparency appearance;
- settings migration/persistence on an approved workstation;
- permission minimization against real user workflows;
- repository-backed cross-version update and exact rollback using two distinct accepted releases;
- data compatibility after rollback.

## Stable blockers

`release/status.json` records the current Stable blockers explicitly. They are acceptance work, not documentation placeholders. Stable and production approval must remain false until every applicable blocker is resolved with evidence.

No automation may convert RC to Stable merely because CI is green.

## Rollback boundary

The development lifecycle harness proves exact verified local-bundle replacement using ordinary Flatpak removal without `--delete-data`, followed by installation of the exact verified artifact. This matches real Flatpak 1.14.6 local-bundle behavior.

Repository-backed `flatpak update`, cross-version upgrade, and rollback remain separate acceptance paths. A real cross-version claim requires two cryptographically distinct accepted packages and distinct installed OSTree commits.

## Promotion rule

The source may be recorded as Release Candidate only when:

- the final exact candidate revision passes every required RC workflow;
- the production-identity RC artifact and its digest are retained as evidence;
- repository and GoreeCloud Drive lifecycle documentation identify the same candidate/source;
- a rollback point is identifiable;
- no known source-level blocker remains.

Stable promotion requires the remaining runtime, security, accessibility, configuration, permission, upgrade/rollback, data-compatibility, and supported-workstation acceptance gates.
