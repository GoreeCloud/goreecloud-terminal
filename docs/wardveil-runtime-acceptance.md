# Wardveil runtime acceptance

This procedure validates the GoreeCloud Terminal Wardveil session-context presentation after the native build and test suite pass. It is intentionally separate from automated CI because the remaining acceptance criteria require a real graphical session, window resizing, interaction, accessibility inspection, and real terminal-process transitions.

## Development-only visual preview

Development builds configured with `-Ddevelopment=true` support a visual preview override through `GORECLOUD_SESSION_CONTEXT_PREVIEW`. The override exists only when `DEVELOPMENT_BUILD` is enabled; production builds ignore it completely. It does not change process privileges, network state, container state, authorization, or terminal behavior. It only selects the existing read-only presentation contract so each chip can be visually inspected without fabricating production security evidence.

Supported values are:

- `local` — hides the Wardveil context chip;
- `remote` — shows the Remote chip;
- `container` — shows the Container chip;
- `elevated` — shows the Elevated chip.

Example launches from a configured build tree:

```sh
GORECLOUD_SESSION_CONTEXT_PREVIEW=remote meson devenv -C _build ./src/ptyxis
GORECLOUD_SESSION_CONTEXT_PREVIEW=container meson devenv -C _build ./src/ptyxis
GORECLOUD_SESSION_CONTEXT_PREVIEW=elevated meson devenv -C _build ./src/ptyxis
GORECLOUD_SESSION_CONTEXT_PREVIEW=local meson devenv -C _build ./src/ptyxis
```

Close every running development instance between preview launches so application uniqueness does not route a later invocation into an already-running process with the earlier environment.

## Visual acceptance matrix

For Remote, Container, and Elevated preview states, verify all of the following:

- the chip appears in application chrome rather than inside VTE terminal content;
- the intended symbolic icon is visible and aligned with its label;
- the label is readable at normal scaling and does not clip vertically;
- the chip remains legible in both light and dark appearance;
- the chip remains usable at normal, narrow, maximized, and fullscreen window sizes;
- the chip does not overlap tab controls, window controls, menu controls, or terminal content;
- the chip treatment is distinguishable without relying on color alone;
- Remote, Container, and Elevated remain visually related as one Glaze UI component family;
- Elevated receives stronger emphasis without resembling an error dialog or destructive-action warning;
- Local hides the chip and does not claim Wardveil protection.

## Accessibility acceptance

Hover each visible chip and verify the tooltip matches its accessible description. With GTK accessibility inspection or the desktop screen reader enabled, verify the component exposes one meaningful label:

- Remote terminal session
- Containerized terminal session
- Elevated superuser terminal session

The icon must not be the only carrier of meaning. Keyboard focus must not become trapped on this informational indicator, and the chip must not impersonate an actionable button.

## Real process-context validation

The preview override validates presentation only. Final acceptance also requires the inherited Ptyxis process-leader detector to drive the same states without the override.

Start the application with `GORECLOUD_SESSION_CONTEXT_PREVIEW` unset. Verify an ordinary local shell shows no chip. Then test the contexts available on the validation machine:

```sh
unset GORECLOUD_SESSION_CONTEXT_PREVIEW
meson devenv -C _build ./src/ptyxis
```

For elevated context, enter a root shell using the operating system's normal authorization path, such as `sudo -s`, and confirm the active tab changes to Elevated. Leave the root shell and confirm the chip clears when the process classification returns to ordinary local context.

For container context, start an interactive container using an already-installed supported runtime, such as Podman, and confirm Container appears while the containerized foreground process is authoritative. Exit the container and confirm the state clears.

For remote context, establish a normal SSH session to a host the tester is authorized to access and confirm Remote appears from the inherited detector. Exit SSH and confirm the state clears. Do not introduce test credentials, private keys, or hostnames into repository files or screenshots.

## Tab-switch validation

Create at least two tabs with materially different detected contexts. Switch repeatedly between them and verify the chip follows the active tab without lag, stale state, duplicate chips, or state leakage. Close the active contextual tab and verify the indicator immediately reflects the newly active tab.

## Palette and regression validation

For each context, switch among representative terminal palettes and both desktop appearances. Confirm the Wardveil chip remains legible while VTE foreground/background colors, cursor visibility, selection colors, transparency behavior, tab overview, menus, and visual bell continue to behave as before.

## Acceptance record

Record the exact commit SHA tested, distribution/runtime environment, display server, desktop appearance, scaling factor, and which real contexts were exercised. Screenshots may be attached to the pull request when they contain no terminal secrets, credentials, private host information, command history that should remain private, or other sensitive information.

### 2026-08-21 development-preview runtime record

Candidate source head: `df9df7ed72ac7f69b9e0890e3824967351f318f9` on `agent/wardveil-session-context`.

Automated validation: GitHub Actions Fork Foundation run `32416020361` completed successfully for the exact candidate head. A local GNOME SDK 50 development build also compiled successfully, and the Meson test suite passed 24/24 tests with zero failures, including `ptyxis:goreecloud-session-context`.

Local development environment: Zorin OS 17.3 Pro host; GNOME SDK 50 Flatpak development shell; Wayland graphical session; dark terminal appearance. The application was installed into a private development prefix under the repository and launched with the development-only preview override. The private development setup was used only to exercise the candidate source and did not represent a production package or workstation replacement.

Observed visual-preview results:

- Remote normal-window presentation passed: icon and `Remote` label rendered as a compact informational chip in the header, remained separate from VTE content, and did not collide with the title or controls.
- Container normal-window presentation passed: container icon and `Container` label rendered with distinct caution treatment and preserved terminal-canvas layout.
- Elevated normal-window presentation passed: superuser icon and `Elevated` label rendered with the strongest of the three context treatments without becoming an error dialog or destructive-action prompt.
- Local preview passed: no session-context chip was displayed.
- Remote narrow-window presentation passed: the title truncated gracefully while the complete Remote chip and window controls remained readable and non-overlapping.
- Remote maximized presentation passed: header alignment, chip readability, and VTE content remained intact.
- Remote fullscreen presentation passed: the application entered true fullscreen, retained the Remote context chip in the fullscreen header, hid normal title buttons as designed, exposed the fullscreen-exit control, and preserved terminal content and command execution.
- The fullscreen terminal remained operational for ordinary host-shell commands and normal `sudo` authorization; no command interception or VTE rendering regression was observed during this check.

This record is presentation evidence only. The following acceptance items remain open: real detector-driven Local/Remote/Container/Elevated transitions with the preview override unset, mixed-context tab switching, tooltip and accessibility inspection, keyboard-focus behavior, light-appearance validation, representative palette/transparency regression checks, and final product-identity integration. Milestone 3 remains Draft and no Stable or production acceptance is claimed.

Milestone 3 remains Draft until the graphical checks, real process-context transitions, tab switching, accessibility behavior, and palette regression checks have been recorded for the exact candidate head.
