# GoreeCloud Terminal Native Install and Packaging Boundary

## Status

Development / staged-install validation only.

This document defines the current native GoreeCloud Terminal install layout and the evidence boundary around it. It does not declare a published package, supported-workstation deployment, production approval, or Stable status.

## Canonical identities

The authoritative Terminal project identity is:

- Production application ID: `com.goreecloud.Terminal`
- Development application ID: `com.goreecloud.Terminal.Devel`
- Canonical executable: `goreecloud-terminal`
- Host-session helper: `goreecloud-terminal-host-agent`

The former native-only `.Native` / `.Native.Devel` application identities are not valid package identities for the current native product line. The Development Flatpak manifest is `native/flatpak/com.goreecloud.Terminal.Devel.json` and must use `com.goreecloud.Terminal.Devel` with command `goreecloud-terminal`.

## Staged application layout

With `--prefix=/usr`, the native application Meson project installs:

- `/usr/bin/goreecloud-terminal`
- `/usr/share/applications/<application-id>.desktop`
- `/usr/share/metainfo/<application-id>.metainfo.xml`
- `/usr/share/icons/hicolor/scalable/apps/<application-id>.svg`

Development and production layouts are staged and validated independently. The Development build is the Meson default. A production-identity build requires the explicit `-Dproduct_identity=production` option.

The desktop entry, AppStream metadata, installed icon name, and runtime application ID all derive from the same selected product identity. A production staging root must not contain Development identity files, and a Development staging root must not contain production identity files.

## Version authority

The native Meson `project()` version is the single Development source for version identity. The current source value is `0.1.0-dev`.

That value is propagated into:

- the generated `GOREECLOUD_TERMINAL_VERSION` product identity header;
- the rendered About-dialog version;
- the native product-identity test contract; and
- the installed AppStream `<release>` version for both staged application identities.

The AppStream release remains explicitly `type="development"`. Selecting `-Dproduct_identity=production` changes the application identity being staged; it does not promote the source version or lifecycle to Stable. The `Native Version Contract` workflow must prove Meson introspection, generated runtime identity, rendered About source, and installed AppStream metadata all agree for both identities.

## Host-session component

The host-session agent remains a separate native Meson project because it runs outside a sandboxed application boundary and has different installation and security responsibilities.

With `--prefix=/usr`, it installs:

- `/usr/libexec/goreecloud-terminal-host-agent`
- `/usr/lib/systemd/user/goreecloud-terminal-host-agent.service`

The systemd user service is intentionally lifecycle-only because it launches the user's interactive host shell. Service-level execution sandboxing, namespace/network-family restrictions, privilege-state restrictions, forced umasks, environment rewriting, and similar process policy would be inherited by descendant shells and would change normal host behavior. The security boundary therefore remains in the private runtime directory/socket, same-user `SO_PEERCRED` checks, bounded versioned protocol, approved-shell and launch-context validation, and the absence of arbitrary-command or network-listener authority. `native/host-agent/test-service-contract.py` locks this service contract in CI.

Staged install validation may place the application and host-agent files into the same temporary DESTDIR to verify a coherent filesystem layout. That does not constitute host-agent enablement, workstation deployment, or physical PTY/job-control acceptance.

## Flatpak Development boundary

The current Development Flatpak identity is `com.goreecloud.Terminal.Devel`.

The manifest:

- keeps the narrow `--filesystem=xdg-run/goreecloud-terminal` runtime-socket permission;
- does not grant `--filesystem=host`;
- does not grant `--talk-name=org.freedesktop.Flatpak`;
- uses `goreecloud-terminal` as the application command;
- selects `-Dproduct_identity=development` rather than rewriting source code;
- uses the same Meson install path as the native staged layout.

The host-agent itself is not packaged inside the Flatpak. Packaging and deployment must preserve the trust boundary between the sandboxed application and the separately installed same-user host service.

## Coexistence and migration boundary

The current Development and production application IDs are distinct, but both native builds use the canonical executable name `goreecloud-terminal`. Therefore this staged-install work does not claim that two native system packages can be installed side by side into the same prefix without a packaging-level coexistence decision.

The transitional Ptyxis-derived GoreeCloud Terminal line also has existing settings and release history. Native replacement still requires explicit, reversible migration/coexistence/rollback design and validation. No staged-install test may be interpreted as permission to overwrite or remove that line on a supported workstation.

## Required validation

The repository staging contract must, for both Development and production identities:

1. bind validation to the exact checked-out source revision;
2. configure and compile the native application with fatal warnings;
3. run the native unit-test suite;
4. install into an isolated DESTDIR;
5. verify the canonical executable, desktop entry, AppStream metadata, and identity-specific icon;
6. validate desktop and AppStream metadata with platform tools;
7. prove Development and production identities do not cross-contaminate each other's staging roots;
8. reject `.Native` application identity artifacts;
9. build and stage the host-agent into the same temporary filesystem root;
10. verify the systemd user service points to the staged layout's canonical `/usr/libexec/goreecloud-terminal-host-agent` runtime path and remains lifecycle-only so spawned host shells retain normal host semantics;
11. prove the staged AppStream release version equals the Meson/runtime version for both application identities.

## Remaining production blockers

This staged layout is only one build/package-validation layer. Native production readiness still requires, at minimum:

- a governed native package/artifact format and exact artifact identity;
- governed production/Stable version assignment, package-version policy, release tags, and release notes beyond the current `0.1.0-dev` Development authority;
- supported-workstation installation and removal behavior;
- migration, coexistence/replacement, and rollback validation against the transitional line;
- settings/profile data compatibility and Everkeep recovery treatment;
- production host-session installation, enablement, and physical PTY/job-control validation;
- real Privacy Shield-authorized remote/SSH acceptance;
- all applicable Integral Platform System integrations and evidence;
- Glaze UI and accessibility acceptance;
- independent exact-artifact verification and governed release promotion.

Passing source or staged-install CI is not Stable or production evidence.
