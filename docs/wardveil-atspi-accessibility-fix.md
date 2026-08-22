# Wardveil AT-SPI Accessibility Fix

## Purpose

This document records the corrective source work for the Wardveil session-context accessibility discrepancy discovered during supported-workstation acceptance of GoreeCloud Terminal 50.2-rc.1.

## Observed runtime discrepancy

On the supported Linux workstation, the published 50.2-rc.1 Flatpak displayed the detector-driven `Remote` Wardveil chip correctly and exposed the expected tooltip `Remote terminal session`. The desktop accessibility service reported `org.a11y.Status.IsEnabled=true`, and GoreeCloud Terminal was restarted after the accessibility bus was active. A direct read-only AT-SPI D-Bus traversal still did not find an accessible object named `Remote terminal session`.

The result is treated as an accessibility acceptance failure for the published Release Candidate, not as a Stable or production-approved state. The immutable 50.2-rc.1 artifact is not modified in place.

## Source analysis

The Wardveil chip is a `GtkBox`. On GTK 4.12 and newer, `GtkBox` uses the generic accessible role by default. The current source writes `GTK_ACCESSIBLE_PROPERTY_LABEL` to that generic container. The runtime uses GTK 4.22, so the corrective design must not depend on a semantically empty generic container being retained as a named AT-SPI object.

## Corrective contract

The session-context chip must:

- remain a non-actionable, read-only presentation element;
- use a semantic accessible group role for the icon-plus-label grouping;
- expose the exact context description (`Remote terminal session`, `Containerized terminal session`, or `Elevated superuser terminal session`) as the group accessible label;
- keep the visible text-plus-icon presentation so context is not conveyed by color alone;
- mark purely decorative child presentation as non-semantic where practical so assistive technologies do not announce duplicate content;
- keep Local/unknown context hidden and free of a misleading Wardveil protection claim;
- preserve normal terminal rendering, VTE palette ownership, keyboard focus order, OpenSSH, sudo, and shell behavior.

## Validation

Source/CI acceptance must verify the semantic accessible role and exact accessible description mapping. Supported-workstation acceptance must then rebuild/install a new reviewed candidate and verify the live AT-SPI tree with accessibility enabled. Passing source CI alone does not close the runtime accessibility gate.

The published 50.2-rc.1 tag and bundle remain immutable. Any corrected package is a new candidate and must follow the normal GoreeCloud release and acceptance process.
