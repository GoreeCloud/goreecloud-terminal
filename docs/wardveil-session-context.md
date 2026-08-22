# Wardveil Security session context

GoreeCloud Terminal uses Wardveil Security by GoreeCloud as the security identity for terminal-specific protective presentation. This layer is intentionally informational: it does not replace or weaken OpenSSH, sudo, container-runtime, shell, operating-system, or host authorization controls.

## Initial context model

The maintained Ptyxis foundation already identifies foreground process-leader context through the terminal agent and exposes three materially different execution environments:

- `remote` — a remote session, including SSH workflows detected by the inherited process model;
- `superuser` — an elevated or root execution context;
- `container` — a containerized shell or foreground process context.

GoreeCloud Terminal preserves that mature detection path and applies GoreeCloud-maintained symbolic presentation to those states. The Wardveil layer therefore adds no credential inspection, shell command interception, remote telemetry, or independent privilege detection.

## Typed presentation contract

`goreecloud-session-context.[ch]` converts the inherited process-leader classification into one small, read-only presentation contract. Each state defines a short text label, an accessible description, an icon name when applicable, a CSS class, a relative presentation severity, and whether a visible context indicator is appropriate.

The current mappings are:

- ordinary/unknown → `Local`, accessible description `Local terminal session`, no Wardveil indicator;
- remote → `Remote`, accessible description `Remote terminal session`, informational severity;
- container → `Container`, accessible description `Containerized terminal session`, caution severity;
- superuser → `Elevated`, accessible description `Elevated superuser terminal session`, strongest context severity.

This separation keeps security-context wording centralized instead of scattering labels and severity decisions throughout GTK widgets. It also creates a stable source boundary for future tab chips, screen-reader names, tooltips, and compact adaptive presentation.

## Presentation rules

Session-context indicators must remain calm, persistent enough to be useful, and visually distinguishable without warning fatigue. They are context indicators, not claims that a session is safe.

- Remote context uses a network/remote-terminal mark.
- Elevated context uses an explicit privilege/key mark and receives the strongest attention treatment of the three detected contexts.
- Container context uses an isolated-workload/container mark.
- Unknown or ordinary local-shell context receives no Wardveil-specific protection claim.
- Text must accompany or be programmatically associated with icon-only presentation whenever the widget is exposed to assistive technology.
- Color must never be the sole carrier of session-context meaning.

The indicators must not use `Protected by Wardveil` merely because a session type was detected. That phrase is reserved for bounded Wardveil protection states backed by current authoritative security evidence.

## Host identity and nested context

Host identity is not inferred from the process-leader classification alone. A later UI layer may present an explicit remote host when a trustworthy source is available, but it must not fabricate or parse a host name from arbitrary command text merely for decoration.

Nested contexts require conservative presentation. For example, an elevated process inside a remote session or a container reached from SSH can contain more than one meaningful boundary. Until the process model exposes enough authoritative information to represent those combinations reliably, GoreeCloud Terminal should display the strongest currently verified context rather than implying a complete execution-path model.

## Authority and privacy boundary

Wardveil presentation is read-only. The underlying terminal agent remains authoritative for the process-leader classification consumed by the UI. GoreeCloud Terminal does not store SSH private keys, sudo passwords, tokens, or other reusable credentials as part of this feature.

The session-context layer must not log command contents or credentials for presentation purposes. Any future history, auditing, or diagnostic capability must be separately reviewed against GoreeCloud privacy, sensitive-information separation, and Wardveil evidence-minimization requirements.

## High-risk action boundary

Routine shell commands remain normal terminal input and are not intercepted by Wardveil. Future high-risk confirmations must be limited to GoreeCloud-owned graphical actions where the application itself initiates a materially destructive or security-sensitive operation. GoreeCloud Terminal must not become a proprietary command-approval shell layered over Bash, OpenSSH, sudo, or other standard tools.

## Future work

The next Milestone 3 integration step is to bind the typed session-context contract into tab/header widgets with accessible text, tooltips, adaptive Glaze UI chips, and screen-reader semantics. Host identity should be added only when a trustworthy data source exists. Runtime validation must verify transitions among local, remote, container, and elevated states without exposing command contents or credentials.
