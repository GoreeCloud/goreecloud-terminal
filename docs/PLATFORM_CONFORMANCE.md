# Mandatory Native and Platform Conformance

## Governing scope

GoreeCloud Terminal must be built and maintained as original GoreeCloud-owned native application software. Narrow mature foundations may remain only where independent reimplementation would materially reduce security, correctness, interoperability, standards compliance, rendering quality, or maintainability. GTK, GLib/GIO, VTE, OpenSSH, and operating-system facilities remain technical foundations rather than inherited product architecture.

Stable qualification is governed by all seven Integral Platform Systems:

- GoreeCloud Manager
- Privacy Shield
- Wardveil Security
- Everkeep
- Glaze UI
- GoreeCloud Mesh
- GoreeCloud Identity

Every applicable responsibility requires current implementation and evidence-backed acceptance. Genuine non-applicability requires explicit justification. A declaration, badge, menu, status surface, Mesh registration, or Manager display is not implementation evidence by itself.

The repository-root `goreecloud.platform.yaml` is the machine-readable Platform Contract declaration. Its computed result does not replace runtime, target-environment, security, privacy, accessibility, recovery, migration, rollback, package, or release acceptance.

## Current native state

GoreeCloud Terminal remains in the Development lifecycle and is nonconformant for Stable qualification.

### GoreeCloud Manager

**State: Applicable — Blocked.**

Native Terminal has not yet implemented or accepted the applicable Manager inventory, lifecycle, health, version, dependency, supported-configuration, or operational-evidence integration.

### Privacy Shield

**State: Applicable — Blocked on this source line.**

Native Terminal already follows privacy-minimizing local design practices, but local minimization is not equivalent to accepted Privacy Shield integration. The current source line represented by this declaration does not contain an accepted Privacy Shield runtime adapter or application-specific runtime acceptance. Remote/network authority must fail closed rather than be manufactured locally.

### Wardveil Security

**State: Applicable — Blocked.**

Wardveil evidence from the transitional Ptyxis-derived Terminal does not establish native conformance. The original native application still requires the current Wardveil runtime contract, relevant protection/trust evidence, understandable security presentation, and supported-runtime acceptance.

### Everkeep

**State: Applicable — Blocked.**

Eligible native preferences, profiles, workspaces, migration state, and future durable continuity data require an accepted Everkeep backup, restore, recovery, preservation, portability, and continuity model. Backup intent alone is not recovery evidence.

### Glaze UI

**State: Applicable — Migration Required.**

Native Terminal source is pinned to the verified Glaze UI 1.3.0 authority through `native/GLAZE_UI_13.md` and `native/data/glaze-ui-manifest.json`. The centrally pinned GoreeCloud Platform Contract v0.2 validator currently requires `compatibility.glaze_ui_required: 1.1.0`. The repository therefore records the central contract-baseline mismatch explicitly instead of falsely claiming platform conformance. Complete rendered/focus/high-contrast/reduced-transparency/assistive-technology and supported-workstation acceptance also remains open.

### GoreeCloud Mesh

**State: Applicable — Blocked.**

Native Terminal has not yet implemented or accepted its Mesh capabilities, dependencies, consumed/published events, or coordination relationships. Mesh discovery or registration must never be treated as authentication, Privacy Shield authorization, Wardveil protection, or permission to execute terminal operations.

### GoreeCloud Identity

**State: Applicable — Blocked pending final applicability and integration design.**

Terminal must evaluate application, service, device, and session identity responsibilities without replacing Linux user identity, PAM/sudo behavior, OpenSSH authentication, or operating-system authorization with an incompatible parallel authority. No accepted native Identity integration currently exists.

## Additional production blockers

The current GoreeCloud production-readiness standard also requires built-in supported API access. Native Terminal does not yet provide an accepted API or compatibility adapter, so API readiness remains a Stable blocker.

Physical supported-workstation validation remains incomplete for the final native product, including rendered Glaze UI, accessibility, PTY and shell behavior, keyboard/input preservation, Unicode/fonts, clipboard/selection, failure handling, and session lifecycle.

Native packaging, exact artifact versioning, coexistence/replacement of the transitional line, settings/profile migration, rollback, Everkeep recovery, independent artifact verification, host-session deployment/physical PTY acceptance, real authorized SSH workflows, and governed release promotion remain separate unresolved gates.

## Lifecycle rule

No native feature, passing CI result, Platform Contract validation result, roadmap entry, documentation update, successful launch, or isolated workstation test may be represented as Stable or production acceptance by itself.

The lifecycle must distinguish source implemented, source validated, build/package validated, integration validated, target-environment validated, security/privacy/recovery validated, upgrade/rollback validated, visual/accessibility accepted, production deployed, and production accepted states.

If this repository contains inherited or upstream-derived application code, that code is transitional or historical. It may remain for migration, compatibility, security maintenance, recovery, rollback, or behavior reference while the original native implementation advances, but it is not the approved final product architecture.

Repository CI, project specifications, feature roadmaps, change logs, release records, and Drive governance records must remain synchronized with verified reality.
