# GoreeCloud Terminal Native Install and Packaging Boundary

## Status

Development / staged-install validation only.

This document defines the current native GoreeCloud Terminal install layout and the evidence boundary around it. It does not declare a published package, supported-workstation deployment, production approval, or Stable status.

## Canonical identities

The authoritative Terminal project identity is:

- Production application ID: `com.goreecloud.Terminal`
- Development application ID: `com.goreecloud.Terminal.Devel`
- Canonical executable: `goreecloud-terminal`
- Host migration maintenance command: `goreecloud-terminal-migrate`
- Host-session helper: `goreecloud-terminal-host-agent`

The former native-only `.Native` / `.Native.Devel` application identities are not valid package identities for the current native product line. The Development Flatpak manifest is `native/flatpak/com.goreecloud.Terminal.Devel.json` and must use `com.goreecloud.Terminal.Devel` with command `goreecloud-terminal`.

## Staged application layout

With `--prefix=/usr`, a host-native application build using the default `-Dinstall_migration_tool=true` installs:

- `/usr/bin/goreecloud-terminal`
- `/usr/bin/goreecloud-terminal-migrate`
- `/usr/share/glib-2.0/schemas/com.goreecloud.Terminal.Migration.gschema.xml`
- `/usr/share/applications/<application-id>.desktop`
- `/usr/share/metainfo/<application-id>.metainfo.xml`
- `/usr/share/icons/hicolor/scalable/apps/<application-id>.svg`

Development and production layouts are staged and validated independently. The Development build is the Meson default. A production-identity build requires the explicit `-Dproduct_identity=production` option.

The desktop entry, AppStream metadata, installed icon name, and runtime application ID all derive from the same selected product identity. A production staging root must not contain Development identity files, and a Development staging root must not contain production identity files.

`install_migration_tool` controls only installation of the host migration command and its compatibility schema. The migration unit-test source remains part of ordinary native builds even when that install surface is disabled.

## Version authority

The native Meson `project()` version is the single Development source for version identity. The current source value is `0.1.0-dev`.

That value is propagated into:

- the generated `GOREECLOUD_TERMINAL_VERSION` product identity header;
- the rendered About-dialog version;
- the native product-identity test contract; and
- the installed AppStream `<release>` version for both staged application identities.

The AppStream release remains explicitly `type="development"`. Selecting `-Dproduct_identity=production` changes the application identity being staged; it does not promote the source version or lifecycle to Stable. The `Native Version Contract` workflow must prove Meson introspection, generated runtime identity, rendered About source, and installed AppStream metadata all agree for both identities.

## Format-neutral package payload contract

`native/package-contract.json` is the machine-readable authority for the current host-native staged package payload boundary. It deliberately does not select RPM, DEB, or any other distribution package format. Under issue #97, the package format remains unselected until GoreeCloud establishes the supported distribution/package-manager scope and approves an artifact model from authoritative requirements rather than inferring one from CI or a test workstation.

The contract records:

- the canonical package name, `/usr` install prefix, application identities, and Meson version authority;
- the exact host-native file payload currently produced by the application, migration-maintenance, and host-session components;
- host-side components that must remain outside the Flatpak sandbox;
- user state that package actions must preserve rather than treat as package-owned disposable data;
- fail-closed package-action policy that forbids automatic migration, automatic partial migration, transitional-state deletion, user-state deletion, host-agent enablement, or host-agent start at this Development checkpoint; and
- the evidence boundary requiring a retained staged-payload manifest plus package-manager, physical-workstation, exact-artifact, and Everkeep recovery acceptance before production or Stable claims.

`native/tools/validate-package-contract.py` resolves the identity-specific payload and compares it to the combined staged application plus host-agent filesystem root. Validation fails on missing files, unexpected files, non-regular package-owned paths, executable-mode drift, obsolete `.Native` identity paths, package-policy drift, sandbox-boundary drift, or premature package-format selection. Future distro-specific package definitions must be derived from this governed payload contract rather than silently adding or omitting installed files.

`native/tools/render-package-payload-manifest.py` creates deterministic staged-payload provenance for Development and production identities after the exact payload has passed validation. Each manifest contains only package-owned evidence: the exact Git source revision, Meson-derived version, selected application identity, package-format state, installed path, file mode, byte size, and SHA-256 digest. It does not inspect user configuration, terminal content, shell history, credentials, secrets, SSH data, environment values, or migration-source contents. The `Native Install Layout Contract` retains the two manifests as a 30-day CI evidence artifact named with the exact source SHA.

The retained staged-payload provenance is Development validation evidence only. It is not a signed distro package, an SBOM substitute, package-manager acceptance, a published release artifact, production approval, or Stable evidence.

This contract establishes a format-neutral package payload definition only. It is not a distro package, package-manager transaction, signature, published artifact, workstation installation, migration approval, production approval, or Stable evidence.

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
- explicitly selects `-Dinstall_migration_tool=false`;
- asserts that `goreecloud-terminal-migrate` and the migration compatibility schema are absent from `/app`;
- uses the same Meson application install path as the native staged layout.

The host-agent and migration maintenance command are not packaged inside the Flatpak. The host-agent requires separately installed same-user host authority, while migration operates on host-native user state. Packaging and deployment must preserve both boundaries rather than granting broad sandbox authority.

## Coexistence and migration boundary

The current Development and production application IDs are distinct, but both native builds use the canonical executable name `goreecloud-terminal`. Therefore this staged-install work does not claim that two native system packages can be installed side by side into the same prefix without a packaging-level coexistence decision.

The repository now contains an explicit source-level transitional migration and rollback contract through `goreecloud-terminal-migrate`. It is deliberately a host-native maintenance surface, not an application startup behavior or a Flatpak capability.

The current migration contract:

- requires an explicit transitional source identity: `production` or `development`;
- performs a read-only preflight before a write;
- supports explicit dry-run operation;
- refuses to replace existing native state unless `--replace-native` is supplied;
- maps only the bounded non-secret state that the current native model can represent safely;
- never reads or copies terminal contents/history, custom-command contents, credentials, tokens, private keys, SSH secrets, or environment values;
- maps the transitional default profile to native profile ID `default` so the generated native default workspace remains valid;
- blocks unsupported default-profile semantics rather than silently changing them;
- requires explicit `--allow-partial` consent when unsupported non-default profiles would be omitted from native state;
- leaves the transitional source untouched, including when partial migration is explicitly allowed;
- creates private rollback backups before native writes;
- records native-store existence and SHA-256 checksums for backed-up files;
- verifies checksums before restoration;
- attempts all native-store restores even when one restore fails; and
- rolls a migration back automatically if a native-store write fails or an unexpected partial result appears after preflight.

The `Native Migration and Rollback Contract` validates this source/staged-install behavior, including corruption rejection and the host-versus-sandbox install boundary. This is not yet package-manager or supported-workstation migration acceptance. It does not authorize removal of the transitional line, and it does not establish that every transitional setting has an exact native representation.

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
9. verify the host-native migration command and compatibility schema are installed when `install_migration_tool=true`;
10. prove the sandbox-style/Flatpak configuration excludes the migration command and compatibility schema with `install_migration_tool=false`;
11. build and stage the host-agent into the same temporary filesystem root;
12. verify the systemd user service points to the staged layout's canonical `/usr/libexec/goreecloud-terminal-host-agent` runtime path and remains lifecycle-only so spawned host shells retain normal host semantics;
13. prove the staged AppStream release version equals the Meson/runtime version for both application identities;
14. validate migration preflight, explicit partial-migration consent, private backup/rollback behavior, checksum rejection, and failure rollback in automated tests;
15. validate each combined staged host-native root against `native/package-contract.json`, rejecting missing or unexpected package-owned files and any premature package-format selection; and
16. render and retain deterministic Development and production staged-payload provenance manifests bound to the exact source SHA and Meson-derived version.

## Remaining production blockers

This staged layout is only one build/package-validation layer. Native production readiness still requires, at minimum:

- completion of issue #97 with a governed native package/artifact format, supported distribution/package-manager scope, and exact artifact identity;
- governed production/Stable version assignment, package-version policy, release tags, and release notes beyond the current `0.1.0-dev` Development authority;
- supported-workstation installation and removal behavior;
- package-manager and physical supported-workstation upgrade, coexistence/replacement, migration, rollback, interruption, and recovery validation against the transitional line;
- a governed decision for transitional settings that do not yet have exact native representations, including the user experience for any explicitly partial migration;
- settings/profile data compatibility and Everkeep recovery treatment;
- production host-session installation, enablement, and physical PTY/job-control validation;
- real Privacy Shield-authorized remote/SSH acceptance;
- all applicable Integral Platform System integrations and evidence;
- Glaze UI and accessibility acceptance;
- independent exact-artifact verification, SBOM/signature/provenance acceptance for the eventual approved package, and governed release promotion.

Passing source or staged-install CI is not Stable or production evidence.
