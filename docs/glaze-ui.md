# GoreeCloud Terminal — Glaze UI 1.4 Native Adoption

## Target

GoreeCloud Terminal targets **Glaze UI 1.4.0 Stable**. The canonical GoreeCloud Glaze reference reviewed for this RC is revision:

```text
883d40ff51d02885650024723c01d229de456285
```

Terminal is a native Linux GTK/libadwaita application. Glaze UI is therefore mapped into native semantic surfaces rather than copying web CSS or requiring a browser rendering layer.

## Supported form factor

- Desktop: supported and primary.
- Phone: unsupported in this RC.
- Tablet: unsupported in this RC.
- TV: unsupported in this RC.

## Semantic mapping

GoreeCloud Terminal maps Glaze semantics as follows:

- **Canvas** — application/window background and terminal-adjacent layout.
- **Solid** — accessibility/high-contrast fallbacks and native surfaces requiring opaque presentation.
- **Raised** — preferences cards and raised native controls.
- **Functional Glass** — header, search, popover, and tab-overview application chrome where translucency does not reduce readability.
- **Clear Glass** — not consumed by Terminal in this RC.
- **Overlay** — menus, popovers, dialogs, and transient native surfaces.
- **Focus** — strong focus-visible outlines independent of decorative glass edges.
- **Status** — Wardveil context chips and other typed state presentation.
- **Motion** — restrained application-chrome transitions that honor reduced-motion preference.
- **Targets** — interactive palette/theme controls retain at least 44×44 CSS-pixel-equivalent target geometry where Terminal adds custom sizing.

## Terminal canvas boundary

VTE owns terminal glyph rendering, palette colors, selection, cursor, and terminal canvas behavior. Glaze UI styles application chrome around the terminal but must not override the active terminal palette merely to create a branded visual effect.

This boundary protects readability, user-selected terminal themes, ANSI color meaning, and VTE accessibility behavior.

## Native controls

GTK/libadwaita widgets remain native controls. GoreeCloud styling may shape geometry, emphasis, surface treatment, and focus presentation, but does not replace native input semantics with custom web-style widgets.

## Light and dark presentation

The base Glaze treatment is optimized for the dark terminal shell while `src/style.css` contains an explicit light color-scheme mapping for headers and find surfaces. Final visual acceptance must verify both modes with real terminal palettes and transparency settings.

## Reduced motion

GTK 4.22 supports the CSS media preference used by the RC source. Under `prefers-reduced-motion: reduce`, GoreeCloud-added preference-card transitions are disabled. Terminal/runtime motion inherited from GTK, libadwaita, or VTE remains subject to their native accessibility behavior.

## Increased contrast

Under `prefers-contrast: more`, GoreeCloud glass surfaces fall back to solid native backgrounds with stronger borders and focus outlines. This deliberately prefers legibility over decorative translucency.

## Accessibility source invariants

The Glaze layer must preserve:

- keyboard navigation and native focus order;
- visible focus treatment;
- minimum custom target sizing;
- native semantic widget roles;
- readable text/icons independent of translucency;
- reduced-motion preference;
- increased-contrast preference;
- no reliance on color alone for Wardveil context meaning.

Source conformance does not replace real assistive-technology acceptance. Mixed-tab Wardveil presentation, source order, focus traversal, resize behavior, light/dark palettes, and real high-contrast behavior remain Stable acceptance gates.

## Performance and dependency boundary

The native mapping uses local GTK CSS and local artwork. It does not require blur shaders, remote assets, browser engines, remote fonts, analytics, or network access. Terminal rendering remains on the inherited VTE path.

## RC boundary

Glaze UI 1.4 source conformance is an RC requirement. Supported-workstation appearance and accessibility acceptance are still required before Stable promotion.
