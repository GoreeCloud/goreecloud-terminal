# GoreeCloud Terminal Supported-Workstation Acceptance

## Purpose

This document defines the evidence boundary for real supported-workstation acceptance of the published GoreeCloud Terminal `50.2-rc.1` Release Candidate.

Source CI, package CI, the immutable tag, and the public GitHub prerelease are already separate validated states. None of them proves that the published production package is suitable to replace the workstation terminal or that the release is Stable.

## Exact release under test

Workstation acceptance is bound to the published artifact, not to an arbitrary rebuild:

- release lifecycle: **Release Candidate**;
- tag: `50.2-rc.1`;
- release source: `ae5d10511a2aed4a504b189165e268a1e9c1040f`;
- production application ID: `com.goreecloud.Terminal`;
- bundle: `goreecloud-terminal-50.2-rc.1-ae5d10511a2aed4a504b189165e268a1e9c1040f.flatpak`;
- bundle SHA-256: `f6819b045319babfbde7e7cfb4c097ed441bc160727c513fd2432283d962cd64`;
- accepted OSTree application commit: `b381917f3ba700c3b1b903423f8241fa80b640bdebdfebb438f47212559f8af7`;
- runtime: GNOME Platform branch 50.

A rebuild from equivalent source is not silently interchangeable with the published release artifact.

## Evidence collector

Use:

```bash
python3 tools/collect-workstation-acceptance.py \
  --bundle /path/to/goreecloud-terminal-50.2-rc.1-ae5d10511a2aed4a504b189165e268a1e9c1040f.flatpak \
  --manual-status /path/to/workstation-manual-status.json \
  --output workstation-acceptance-evidence.json
```

The collector is intentionally read-only. It may:

- verify the caller-supplied bundle filename and SHA-256;
- inspect an already installed production Flatpak;
- verify the installed application ref, GNOME runtime branch, and exact OSTree commit;
- run `goreecloud-terminal --version` inside the installed Flatpak;
- run the read-only local `api status` contract;
- inspect and normalize the application-declared Flatpak permission contract;
- record only fixed manual pass/fail/pending statuses.

It does **not** install, remove, update, roll back, connect to SSH destinations, change settings, terminate sessions, create tags, edit Releases, modify `release/status.json`, deploy the application, or promote lifecycle state.

## Manual status file

Copy `release/workstation-acceptance-manual-status.example.json` to a local evidence workspace outside the repository and change only status values from `pending` to `pass` or `fail` as each controlled check is performed.

The schema intentionally accepts no free-form notes. Every accepted manual check has one fixed identifier and one of three values:

- `pending` — not yet accepted;
- `pass` — controlled workstation acceptance completed without an unresolved failure;
- `fail` — the check exposed a regression or unresolved acceptance problem.

Missing required checks are treated as `pending`.

## Required manual checks

The fixed checks cover:

- desktop product name/icon and launch behavior;
- D-Bus activation and single-instance behavior;
- New Window, New Tab, and Preferences actions;
- local PTY and shell startup;
- working-directory behavior;
- rendering, Unicode, and fonts;
- tabs and session lifecycle;
- keyboard shortcuts;
- selection and clipboard;
- GoreeCloud context-menu actions;
- normal `sudo` prompting/workflows;
- Glaze UI light/dark behavior;
- terminal palette/transparency boundary;
- focus and accessibility behavior;
- detector-driven Wardveil transitions with preview overrides disabled;
- mixed-context tab behavior;
- controlled real OpenSSH behavior;
- SSH failure/disconnect behavior;
- settings persistence;
- controlled settings migration and rollback;
- Flatpak permission-minimization review;
- intended upstream Ptyxis coexistence;
- production package install/remove/reinstall behavior;
- crash and recovery behavior.

Issues #20 and #23 through #28 separate the major workstation sub-gates. Issue #18 is the parent supported-workstation acceptance boundary.

## Flatpak permission-minimization sub-gate

Issue #20 is governed by `release/flatpak-permission-review.json` and `docs/flatpak-permission-minimization.md`.

The source-level review may identify candidate permission reductions, but the manual `flatpak_permission_minimization` check must remain `pending` until the final proposed permission set is exercised on the supported workstation. The broad `--filesystem=host` grant is the highest-priority minimization candidate in the current review; no source-only or CI result may convert that finding into a claim that the permission can safely be removed.

A permission-reduced rebuild is a new package candidate. It must not be substituted for the published `50.2-rc.1` artifact when validating the immutable release itself, and any future candidate must carry its own exact source, bundle hash, OSTree commit, and acceptance evidence.

## Privacy boundary

The collector does not read or record:

- terminal contents;
- shell history;
- arbitrary environment variables;
- usernames or hostnames;
- home-directory paths;
- IP addresses or private host inventories;
- SSH configuration or host aliases;
- profile contents;
- passwords, private keys, tokens, credentials, or other reusable secrets;
- arbitrary free-form notes.

The sanitized record may contain only the non-sensitive OS ID/version, session-type classification, package/runtime identities, normalized permission observations, fixed manual statuses, and acceptance booleans.

## Completion semantics

`workstation_acceptance_complete` can become true only when:

1. the exact published release bundle is supplied and matches its published SHA-256;
2. the installed production Flatpak matches the expected application ID, GNOME Platform branch 50, and exact published OSTree commit;
3. the local API still identifies Release Candidate `50.2-rc.1` and confirms terminal contents and credentials are excluded; and
4. every fixed manual workstation check is `pass`.

`--require-complete` makes the collector exit nonzero unless those conditions are satisfied.

Even a complete workstation record keeps `production_approved` and `stable_approved` false in the evidence. The collector has no authority to change them.

## Remaining Stable blockers

Supported-workstation completion still does not satisfy the separate requirements for:

- a second genuinely distinct accepted package with repository-backed cross-version update and rollback; and
- post-rollback data compatibility and recovery acceptance.

Those are tracked separately and require a future real candidate; `50.2-rc.1` alone cannot manufacture that evidence.
