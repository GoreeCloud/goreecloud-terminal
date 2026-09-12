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
| FR-006 | Provide first-class profiles and workspaces for shells, working directories, environment policy, appearance preferences, and reusable launch contexts without storing secrets in profile metadata. | High | Planned |
| FR-007 | Implement SSH and remote-session workflows with explicit local/remote identity, connection lifecycle, reconnect behavior, host verification, and evidence-backed disconnected states. | Critical | Planned |
| FR-008 | Add trustworthy context states for local, remote, disconnected, child-exited, elevated, restored, and recovery sessions only when runtime evidence supports each state. | Critical | Partially implemented: local/exited only |
| FR-009 | Integrate Wardveil Security through its versioned platform contract for applicable terminal/session protections without inventing a Terminal-local security authority. | Critical | Planned |
| FR-010 | Integrate Privacy Shield authorization and privacy controls for remote/networked operations, fail closed at trust/time boundaries, and avoid terminal-content telemetry or credential exposure. | Critical | Planned |
| FR-011 | Integrate Everkeep for eligible session/workspace continuity and recovery state without duplicating Everkeep persistence or presenting unverified recovery claims. | High | Planned |
| FR-012 | Add settings persistence for appearance, profiles, shortcuts, startup behavior, scrollback policy, terminal preferences, and approved integration controls. | High | Partially implemented: Theme Engine mode persistence exists; broader settings remain planned |
| FR-013 | Expand keyboard-first workflows with deterministic shortcuts, logical focus order, discoverable commands, conflict review, and full terminal-input preservation. | High | In progress |
| FR-014 | Add split-pane and multi-session workspace layouts with keyboard navigation, pane resizing, clear active-pane state, and continuity-safe restoration. | High | Proposed |
| FR-015 | Add shell/context integration for working-directory awareness, safe title updates, foreground-process state, and supported shell hooks without logging command content. | High | Proposed |
| FR-016 | Add safe search, copy, paste, selection, open-link, and clipboard workflows with explicit dangerous-paste protections and no background collection of terminal content. | High | In progress: native Copy/Paste/Select All menu surface exists; dangerous-paste protection remains planned |
| FR-017 | Add scalable scrollback controls, configurable retention, memory bounds, and privacy-aware clearing behavior suitable for long-running sessions. | Medium | Proposed |
| FR-018 | Complete accessibility acceptance for keyboard, focus, high contrast, reduced motion/transparency, 200% text/reflow where applicable, screen readers, target sizing, and representative VTE content. | Critical | In progress: source contracts and isolated laptop acceptance runner exist; physical-device rendered and assistive-technology results remain pending |
| FR-019 | Add repository-controlled product identity, iconography, desktop integration, notifications where justified, and Linux desktop conventions without importing inherited product branding. | High | Planned |
| FR-020 | Complete native packaging, migration tooling, settings/profile migration, rollback, release validation, and supported-workstation acceptance before replacing the transitional Ptyxis-derived line. | Critical | In progress: supported-workstation acceptance runner exists; packaging, migration/rollback, physical-device qualification, and release promotion remain pending |
| FR-021 | Maintain explicit Development/Validation/Promotion/Packaged/Stable lifecycle separation; never treat source completion or passing CI alone as Stable evidence. | Critical | Ongoing control |
| FR-022 | Maintain a GoreeCloud Terminal-owned Theme Engine that maps Glaze-approved appearance modes to terminal-specific presentation without duplicating Glaze UI authority or silently remapping ANSI semantics. | High | In progress: Follow System/Light/Dark/Deep Dark engine, tests, and private XDG mode persistence implemented; rendered acceptance and optional import/export policy remain open |
| FR-023 | Replace inherited/right-click convenience actions with a GoreeCloud-owned terminal context menu; prohibit privileged/package-management shortcuts such as `sudo apt update`; provide Copy, Paste, Select All, Clear, New Session, and Close Session. | High | Implemented in native Development source; rendered acceptance pending |

## Near-term sequence

The recommended implementation order is:

1. finish issue #73 Glaze UI native chrome, Theme Engine, rebuilt context menu, and repository-local validation against the current Stable shared design-system contract;
2. run the repository-controlled laptop/workstation acceptance path and record physical-device results for theme switching/restoration, context menu, Clear, focus, high contrast, representative VTE behavior, and assistive technology;
3. complete remaining deterministic accessibility and input acceptance for the native window/session layer;
4. expand settings persistence beyond the Theme Engine, then implement profiles/workspaces and safe preference storage;
5. implement SSH/remote lifecycle and trustworthy context-state presentation;
6. integrate Wardveil Security, Privacy Shield, and Everkeep through their canonical versioned platform contracts;
7. add split-pane, shell-context, dangerous-paste protection, search, and long-session quality improvements; and
8. complete migration, packaging, supported-workstation qualification, and release promotion gates before Stable classification.

## Maintenance and synchronization

This roadmap and the corresponding Drive `FEATURE-ROADMAP.docx` must remain materially synchronized with one another and with the authoritative project or service record. Update both copies whenever feature scope, priority, dependency, implementation status, cancellation, supersession, or verification state materially changes.

No feature may be represented as complete or Stable solely because it appears in this roadmap. Completion and lifecycle claims require the applicable authoritative implementation, validation, review, release, and production evidence.

## Reconciliation rule

At each material feature change, reconcile this roadmap against the current authoritative project record, repository implementation state, applicable platform-system requirements, and GoreeCloud Tasks Management. Missing obligations, stale status, duplicated work, roadmap drift, or undocumented disposition changes are defects to correct.
