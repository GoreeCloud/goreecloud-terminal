# GoreeCloud Terminal Flatpak Permission Minimization

## Purpose

This document records the source-review and isolated-candidate phases of the GoreeCloud Terminal Flatpak permission-minimization Stable blocker.

The current production and development manifests use the same eight permissions:

```text
--allow=devel
--device=dri
--filesystem=host
--share=ipc
--share=network
--socket=fallback-x11
--socket=wayland
--talk-name=org.freedesktop.Flatpak
```

Permission minimization is not complete. The canonical production and development manifests remain unchanged, and the current eight-permission set is still the authoritative package baseline until supported-workstation acceptance proves a narrower set.

## Reference baseline

The current Flathub packaging for upstream Ptyxis uses the same eight-permission set. That is useful evidence for compatibility with the inherited Ptyxis architecture, but matching upstream is not by itself GoreeCloud approval.

The review snapshot records the current Flathub manifest blob in `release/flatpak-permission-review.json` so later reviews can distinguish a changed upstream baseline from the evidence used here.

In particular, the current Flathub lint exception for broad host-filesystem access says that the permission predates the linter rule. That is historical compatibility evidence, not a positive demonstration that unrestricted host filesystem access is still the narrowest technically correct permission.

## Review conclusions

### `--filesystem=host`

This is the highest-priority minimization candidate because it grants broad host filesystem access.

It must not be removed or narrowed solely because it is broad. A general-purpose terminal has working-directory, path, file-link, drag-and-drop, external-mount, profile-directory, container, and host-shell behaviors that may depend on host filesystem visibility.

Source review now supports testing an isolated candidate without this permission:

- the Flatpak UI starts `ptyxis-agent` on the host through `flatpak-spawn --host`, with `org.freedesktop.Flatpak` access retained;
- the host-side session container validates requested working directories on the host before spawning the shell or command;
- command-line and preserved working directories are marshaled as URI/path metadata to the host-side spawn path rather than requiring the UI process to traverse the target directory;
- external URI opening uses the desktop portal;
- terminal drag and drop recognizes document-portal formats and can recover the document portal's host-path metadata for insertion into the shell;
- user palettes are stored under the application's XDG user-data directory rather than depending on unrestricted host filesystem visibility.

These findings make removal technically plausible; they do not prove it safe. A narrower candidate must still be tested against every affected behavior on the supported workstation before it can replace the current grant.

GoreeCloud Terminal must not use broad host access for telemetry, unrelated indexing, credential harvesting, background host scanning, or automatic upload.

### `--talk-name=org.freedesktop.Flatpak`

This is retained for the inherited host-agent architecture. Current Ptyxis Flatpak packaging uses host spawning so `ptyxis-agent` can perform host-side PTY setup and related host process/container work.

Removing this permission is therefore an architectural change, not a cosmetic sandbox reduction. It requires a replacement host-agent path or supported-workstation proof that all host-agent responsibilities remain functional without it.

### `--share=network`

This is retained as a required capability because GoreeCloud Terminal must support normal network-aware shell workflows, including standard OpenSSH.

Network access does not make GoreeCloud Terminal an authentication authority. Host-key verification, credentials, forwarding, proxies, and authentication remain controlled by OpenSSH and the operating system.

### `--socket=wayland`

This is retained as a required graphical capability because Wayland is the primary graphical session path for the supported Linux workstation environment.

### `--socket=fallback-x11`

This is retained pending supported-workstation testing. The current acceptance requirements still include X11 fallback. It may be removed only if GoreeCloud intentionally drops that compatibility requirement or validates a narrower alternative.

### `--device=dri`

This is retained pending real rendering and graphics acceptance. Removing it may affect accelerated GTK/VTE rendering, transparency, fullscreen behavior, and performance characteristics that source-only CI cannot prove.

### `--share=ipc`

This is retained pending graphical validation because it participates in the inherited GUI/X11 integration baseline. Any reduction must preserve Wayland/X11 behavior, rendering, selection, and clipboard functionality.

### `--allow=devel`

This is retained pending supported-workstation host-agent, PTY, container, and process-context validation. The inherited package architecture currently carries this capability; source inspection alone does not prove that removing it is safe.

## Development-only no-host-filesystem candidate

`tools/generate-flatpak-permission-candidate.py` derives a transient development manifest from `com.goreecloud.Terminal.Devel.json`. The generator permits exactly one delta: removal of `--filesystem=host`.

The generated file is `com.goreecloud.Terminal.Devel.NoHostFS.generated.json`. It is not committed as a canonical manifest and is not a release definition. The generator restores the original `finish-args` in memory and requires the resulting document to be byte-for-byte equivalent at the parsed JSON structure level, proving that no field outside the permission list changed.

`.github/workflows/flatpak-no-host-filesystem-candidate.yml` builds that exact-head candidate, exports and bundles it, installs it into the CI user Flatpak installation, verifies the installed permissions do not contain host filesystem access, runs non-graphical product/SSH/profile smoke checks, removes the candidate, and uploads the bundle as a short-lived development artifact.

Passing this workflow means only that the narrower sandbox is buildable, installable, inspectable, non-graphically smoke-testable, and suitable for controlled workstation evaluation. It does not establish graphical behavior, real PTY behavior, external-mount behavior, drag-and-drop behavior, preserved working directories, container behavior, or Stable acceptance.

The canonical production and development manifests remain unchanged. `50.2-rc.1` remains immutable and is not replaced by this experiment.

## Fail-closed review contract

`release/flatpak-permission-review.json` is the machine-readable review record.

`tools/validate-flatpak-permission-review.py` requires:

- production and development manifests to expose exactly the same canonical permission set;
- the review record to cover every canonical permission exactly once;
- no new canonical permission to appear silently;
- every permission to have a recorded disposition, reason, and affected acceptance checks;
- `--filesystem=host` to remain explicitly identified as the highest-priority minimization candidate;
- the no-host-filesystem experiment to be development-only, remove exactly one permission, add none, and leave canonical manifests unchanged;
- candidate CI to remain explicitly non-authoritative for production and Stable;
- Flatpak host-agent access to remain explicitly documented;
- `permission_set_minimized`, `production_approved`, and `stable_approved` to remain false.

A future canonical permission change must update the manifests, review record, documentation, exact-head package evidence, and supported-workstation evidence together.

## Supported-workstation minimization procedure

Permission changes must be evaluated against the exact package candidate being considered for acceptance. The safe sequence is:

1. Establish a known-good baseline package and record its exact source, bundle hash, OSTree commit, and permission set.
2. Change one permission change at a time in an isolated candidate so the behavioral effect can be attributed to that permission boundary.
3. Rebuild and pass the normal exact-head source, release-readiness, lifecycle, development-package, and production-package gates plus the candidate-specific package gate.
4. Exercise every behavior listed in that permission's `required_acceptance` contract on the supported workstation.
5. Record only fixed pass/fail/pending evidence and non-sensitive diagnostics.
6. Reject the narrower permission set if a required workflow regresses and the regression cannot be resolved without re-expanding the relevant capability.
7. Restore or retain the known-good permission state before testing a different reduction.

The repository must not automatically rewrite installed Flatpak overrides or mutate the workstation merely to conduct this source review.

## Evidence and privacy

Permission review evidence may record:

- application and runtime identity;
- exact package/source identity;
- normalized permission names;
- fixed pass/fail/pending acceptance outcomes;
- non-sensitive failure classifications.

It must not collect terminal contents, command history, private SSH configuration, host credentials, private keys, passwords, tokens, profile contents, or unrelated host-file contents.

## Release boundary

This source review and development-only candidate establish a controlled minimization experiment and prevent silent permission expansion. They do not prove that the current manifest is minimized and do not change the already published `50.2-rc.1` artifact.

The immutable `50.2-rc.1` release remains bound to its published permission set. Any narrower package that becomes canonical would be a new reviewed candidate that must receive its own exact package identity and acceptance evidence.

`production_approved` and `stable_approved` remain `false`. Issue #20 remains open until the supported workstation validates the final permission decision.
