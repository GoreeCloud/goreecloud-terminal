# Wardveil Security session context

GoreeCloud Terminal uses Wardveil Security by GoreeCloud as the security identity for terminal-specific protective presentation. This layer is intentionally informational: it does not replace or weaken OpenSSH, sudo, container-runtime, shell, operating-system, or host authorization controls.

## Initial context model

The maintained Ptyxis foundation already identifies foreground process-leader context through the terminal agent and exposes three materially different execution environments:

- `remote` — a remote session, including SSH workflows detected by the inherited process model;
- `superuser` — an elevated or root execution context;
- `container` — a containerized shell or foreground process context.

GoreeCloud Terminal preserves that mature detection path and applies GoreeCloud-maintained symbolic presentation to those states. The first Wardveil layer therefore adds no credential inspection, shell command interception, remote telemetry, or independent privilege detection.

## Presentation rules

Session-context indicators must remain calm, persistent enough to be useful, and visually distinguishable without warning fatigue. They are context indicators, not claims that a session is safe.

- Remote context uses a network/remote-terminal mark.
- Elevated context uses an explicit privilege/key mark and should receive the strongest attention treatment of the three contexts.
- Container context uses an isolated-workload/container mark.
- Unknown or ordinary local-shell context receives no Wardveil-specific protection claim.

The indicators must not use `Protected by Wardveil` merely because a session type was detected. That phrase is reserved for bounded Wardveil protection states backed by current authoritative security evidence.

## Authority and privacy boundary

Wardveil presentation is read-only. The underlying terminal agent remains authoritative for the process-leader classification consumed by the UI. GoreeCloud Terminal does not store SSH private keys, sudo passwords, tokens, or other reusable credentials as part of this feature.

The session-context layer must not log command contents or credentials for presentation purposes. Any future history, auditing, or diagnostic capability must be separately reviewed against GoreeCloud privacy, sensitive-information separation, and Wardveil evidence-minimization requirements.

## Future work

Later Milestone 3 work may add accessible text labels, tab-level context chips, host identity presentation, risk-appropriate confirmations for narrowly defined high-risk GoreeCloud actions, and richer context for nested remote/container/elevated sessions. Those additions must continue using standard Linux and OpenSSH behavior rather than creating a proprietary shell or credential store.
