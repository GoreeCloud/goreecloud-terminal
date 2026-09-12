# GoreeCloud Terminal — Feature Roadmap

**Status:** Active roadmap control  
**As of:** 2026-09-11  
**Authoritative project record:** Project Specification — Terminal  
**Canonical repository:** GoreeCloud/goreecloud-terminal  
**Drive control:** `GoreeCloud/Feature Roadmap/GoreeCloud Terminal/FEATURE-ROADMAP.docx`

## Purpose

This file is the repository-side feature roadmap control for GoreeCloud Terminal. It records current planned and recommended feature work without replacing the authoritative project record, implementation evidence, release gates, or GoreeCloud Tasks Management.

The roadmap separates verified implementation from proposed work. A roadmap entry marked implemented does not by itself establish Stable, production, accessibility, security, privacy, or workstation acceptance.

## Roadmap

| ID | Feature / obligation | Priority | Current state |
| --- | --- | --- | --- |
| FR-001 | Reconcile and maintain every current planned or recommended GoreeCloud Terminal feature from the authoritative project record and verified repository evidence in this roadmap. | High | Ongoing control |
| FR-002 | Move actionable feature obligations into GoreeCloud Tasks Management when required, preserving priority, dependency, and lifecycle disposition. | High | Ongoing control |
| FR-003 | Do not mark features implemented, complete, cancelled, or superseded without authoritative evidence and synchronized repository/Drive roadmap updates. | High | Ongoing control |
| FR-004 | Replace inherited product architecture with original GoreeCloud-owned GTK/VTE native application architecture while retaining mature terminal/rendering libraries only where justified. | Critical | In progress; native foundation exists |
| FR-005 | Implement current-authority Glaze UI presentation for native application chrome with exact design-system version pinning, drift validation, accessibility contracts, and VTE/content isolation. | Critical | In progress — issue #73 |
| FR-006 | Provide first-class profiles and workspaces for shells, working directories, environment policy, appearance preferences, and reusable launch contexts without storing secrets in profile metadata. | High | In progress: private profile/workspace persistence, bounded host launch contexts, strict runtime-catalog validation, main-window profile selection, profile-specific launch settings, and live saved-workspace activation are implemented in Development source; richer profile/workspace management UX and rendered/physical acceptance remain open |
| FR-007 | Implement SSH and remote-session workflows with explicit local/remote identity, connection lifecycle, reconnect behavior, host verification, and evidence-backed disconnected states. | Critical | Planned |
| FR-008 | Add trustworthy context states for local, remote, disconnected, child-exited, elevated, restored, and recovery sessions only when runtime evidence supports each state. | Critical | Partially implemented: local host, host-unavailable/disconnected, and child-exited states exist; remote/elevated/restored/recovery remain open |
| FR-009 | Integrate Wardveil Security through its versioned platform contract for applicable terminal/session protections without inventing a Terminal-local security authority. | Critical | Planned |
| FR-010 | Integrate Privacy Shield authorization and privacy controls for remote/networked operations, fail closed at trust/time boundaries, and avoid terminal-content telemetry or credential exposure. | Critical | Planned |
| FR-011 | Integrate Everkeep for eligible session/workspace continuity and recovery state without duplicating Everkeep persistence or presenting unverified recovery claims. | High | Planned |
| FR-012 | Add settings persistence for appearance, profiles, shortcuts, startup behavior, scrollback policy, terminal preferences, and approved integration controls. | High | In progress: Theme Engine, terminal preferences, profile metadata, and workspace metadata persist privately with restrictive permissions; shortcut/startup/integration-control settings remain open |
| FR-013 | Expand keyboard-first workflows with deterministic shortcuts, logical focus order, discoverable commands, conflict review, and full terminal-input preservation. | High | In progress: new/close window/session, open-tabs, search, and search navigation shortcuts exist; workspace runtime includes reusable pane-focus/resize primitives, but user-facing pane keyboard bindings and full conflict review remain open |
| FR-014 | Add split-pane and multi-session workspace layouts with keyboard navigation, pane resizing, clear active-pane state, and continuity-safe restoration. | High | In progress: bounded split metadata, deterministic runtime planning, GTK split rendering, saved-workspace activation, and explicit focus-driven active-pane state are implemented in Development source; reusable focus/resize primitives exist, while user-facing keyboard bindings, rendered acceptance, and continuity-safe restoration remain open |
| FR-015 | Add shell/context integration for working-directory awareness, safe title updates, foreground-process state, and supported shell hooks without logging command content. | High | Proposed |
| FR-016 | Add safe search, copy, paste, selection, open-link, and clipboard workflows with explicit dangerous-paste protections and no background collection of terminal content. | High | In progress: native search, Copy/Paste/Select All/Clear surfaces, asynchronous clipboard inspection, multiline/control-character confirmation, invalid-text rejection, and fail-closed paste handling implemented in Development; open-link policy remains open |
| FR-017 | Add scalable scrollback controls, configurable retention, memory bounds, and privacy-aware clearing behavior suitable for long-running sessions. | Medium | In progress: bounded persisted scrollback policy and non-history-mutating visible Clear behavior implemented in Development; richer retention/clearing controls remain open |
| FR-018 | Complete accessibility acceptance for keyboard, focus, high contrast, reduced motion/transparency, 200% text/reflow where applicable, screen readers, target sizing, and representative VTE content. | Critical | In progress: source contracts and isolated laptop acceptance runner exist; physical-device rendered and assistive-technology results remain pending |
| FR-019 | Add repository-controlled product identity, iconography, desktop integration, notifications where justified, and Linux desktop conventions without importing inherited product branding. | High | Planned |
| FR-020 | Complete native packaging, migration tooling, settings/profile migration, rollback, release validation, and supported-workstation acceptance before replacing the transitional Ptyxis-derived line. | Critical | In progress: supported-workstation acceptance runner and bounded host-agent packaging assets exist; full native packaging, migration/rollback, physical-device qualification, and release promotion remain pending |
| FR-021 | Maintain explicit Development/Validation/Promotion/Packaged/Stable lifecycle separation; never treat source completion or passing CI alone as Stable evidence. | Critical | Ongoing control |
| FR-022 | Maintain a GoreeCloud Terminal-owned Theme Engine that maps Glaze-approved appearance modes to terminal-specific presentation without duplicating Glaze UI authority or silently remapping ANSI semantics. | High | In progress: Follow System/Light/Dark/Deep Dark engine, tests, and private XDG mode persistence implemented; rendered acceptance and optional import/export policy remain open |
| FR-023 | Replace inherited/right-click convenience actions with a GoreeCloud-owned terminal context menu; prohibit privileged/package-management shortcuts such as `sudo apt update`; provide Copy, Paste, Select All, Clear, New Session, and Close Session. | High | Implemented in native Development source; rendered acceptance pending |

## Near-term sequence

The recommended implementation order is:

1. finish user-facing keyboard pane navigation/resizing, rendered split-workspace acceptance, continuity-safe restoration, and richer profile/workspace management on top of the now-wired runtime catalog;
2. finish issue #73 Glaze UI native chrome and repository-local validation against the current Stable shared design-system contract, including the new pane/workspace surfaces;
3. implement SSH/remote lifecycle, host verification, reconnect behavior, Privacy Shield authorization boundaries, and trustworthy remote context-state presentation without enabling network operations that lack accepted authority;
4. resolve and integrate current authoritative Wardveil Security, Privacy Shield, Everkeep, GoreeCloud Identity, GoreeCloud Mesh, and GoreeCloud Manager contracts without manufacturing authority locally;
5. add shell/context integration, safe open-link policy, richer scrollback clearing/retention controls, and continuity-safe workspace restoration;
6. complete deterministic accessibility and terminal-input acceptance, then run the repository-controlled physical workstation/assistive-technology acceptance path;
7. complete product identity, desktop integration, native packaging, migration/settings-profile migration, rollback, and exact-artifact release validation; and
8. promote only after supported-workstation, Integral Platform System, packaging, rollback, and Stable qualification gates are backed by current authoritative evidence.

## Maintenance and synchronization

This roadmap and the corresponding Drive `FEATURE-ROADMAP.docx` must remain materially synchronized with one another and with the authoritative project or service record. Update both copies whenever feature scope, priority, dependency, implementation status, cancellation, supersession, or verification state materially changes.

No feature may be represented as complete or Stable solely because it appears in this roadmap. Completion and lifecycle claims require the applicable authoritative implementation, validation, review, release, and production evidence.

## Reconciliation rule

At each material feature change, reconcile this roadmap against the current authoritative project record, repository implementation state, applicable platform-system requirements, and GoreeCloud Tasks Management. Missing obligations, stale status, duplicated work, roadmap drift, or undocumented disposition changes are defects to correct.
