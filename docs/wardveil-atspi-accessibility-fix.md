# Wardveil AT-SPI Accessibility Fix

## Purpose

This document records the corrective source work for the Wardveil session-context accessibility discrepancy discovered during supported-workstation acceptance of GoreeCloud Terminal 50.2-rc.1.

## Observed runtime discrepancy

On the supported Linux workstation, the published 50.2-rc.1 Flatpak displayed the detector-driven `Remote` Wardveil chip correctly and exposed the expected tooltip `Remote terminal session`. The desktop accessibility service reported `org.a11y.Status.IsEnabled=true`, and GoreeCloud Terminal was restarted after the accessibility bus was active. A direct read-only AT-SPI D-Bus traversal still did not find an accessible object named `Remote terminal session`.

The result is treated as an accessibility acceptance failure for the published Release Candidate, not as a Stable or production-approved state. The immutable 50.2-rc.1 artifact is not modified in place.

## Source analysis

The Wardveil chip is a `GtkBox`. On GTK 4.12 and newer, `GtkBox` uses the generic accessible role by default. The published source wrote `GTK_ACCESSIBLE_PROPERTY_LABEL` to that generic container. The production runtime uses GTK 4.22, so the corrective design must not depend on a semantically empty generic container being retained as a named AT-SPI object.

The first corrective candidate changed that container to `GTK_ACCESSIBLE_ROLE_GROUP`, placed the exact accessible description on the group, and marked both visible children `GTK_ACCESSIBLE_ROLE_NONE`. That candidate passed source and package CI but failed the live supported-workstation Remote exact-name check: AT-SPI still did not expose `Remote terminal session`. The group-only design is therefore rejected.

## Corrective contract

The first corrective candidate used a semantic `GROUP` container and marked both visible children `NONE`. Live supported-workstation AT-SPI verification still failed to expose `Remote terminal session`, so that design is rejected.

The revised session-context accessibility contract must:

- remain a non-actionable, read-only presentation element;
- keep the `GtkBox` as a generic layout container rather than making the container the semantic announcement target;
- keep the symbolic `GtkImage` non-semantic because it is decorative duplication of the visible context text;
- expose the visible `GtkLabel` with `GTK_ACCESSIBLE_ROLE_LABEL`;
- override that semantic label's accessible name with the exact context description (`Remote terminal session`, `Containerized terminal session`, or `Elevated superuser terminal session`) while leaving its visible text concise (`Remote`, `Container`, or `Elevated`);
- reset the explicit accessible label when Local/unknown context is active and keep the entire chip hidden so Local does not make a Wardveil protection claim;
- keep the visible text-plus-icon presentation so context is not conveyed by color alone;
- preserve normal terminal rendering, VTE palette ownership, keyboard focus order, OpenSSH, sudo, and shell behavior.

## Validation

Source/CI acceptance must verify that the layout container remains generic, the decorative icon remains non-semantic, the visible text label retains semantic label role, and the exact context description is applied to that label's accessible name. Supported-workstation acceptance must then rebuild/install a new reviewed candidate and verify the live AT-SPI tree with accessibility enabled. Passing source CI alone does not close the runtime accessibility gate.

The published 50.2-rc.1 tag and bundle remain immutable. Any corrected package is a new candidate and must follow the normal GoreeCloud release and acceptance process.
