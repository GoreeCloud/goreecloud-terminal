#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: scripts/testing/native-workstation-acceptance.sh [--automated-only]

Builds and tests the GoreeCloud Terminal native implementation from the current
checked-out revision. In interactive mode it then launches the application twice
with an isolated XDG configuration directory so physical-device Glaze UI, Theme
Engine persistence, context-menu, Clear, keyboard, focus, high-contrast, VTE,
and assistive-technology behavior can be reviewed without changing the user's
normal GoreeCloud Terminal preferences.

Options:
  --automated-only  Run dependency, build, contract, and unit-test checks only.
  -h, --help        Show this help text.
EOF
}

mode="interactive"
case "${1:-}" in
  --automated-only) mode="automated" ;;
  -h|--help) usage; exit 0 ;;
  "") ;;
  *) usage >&2; exit 2 ;;
esac

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
path_candidate="$(cd -- "${script_dir}/../.." && pwd)"

if [[ -n "${GITHUB_WORKSPACE:-}" ]] && git -C "${GITHUB_WORKSPACE}" rev-parse --show-toplevel >/dev/null 2>&1; then
  repo_root="$(git -C "${GITHUB_WORKSPACE}" rev-parse --show-toplevel)"
elif git -C "${path_candidate}" rev-parse --show-toplevel >/dev/null 2>&1; then
  repo_root="$(git -C "${path_candidate}" rev-parse --show-toplevel)"
else
  printf 'Acceptance runner could not resolve a GoreeCloud Terminal Git checkout.\n' >&2
  printf 'Script path candidate: %s\n' "${path_candidate}" >&2
  exit 1
fi

native_dir="${repo_root}/native"

require_command() {
  if ! command -v "$1" >/dev/null 2>&1; then
    printf 'Missing required command: %s\n' "$1" >&2
    return 1
  fi
}

missing=0
for command_name in git gcc meson ninja pkg-config python3; do
  require_command "${command_name}" || missing=1
done

if (( missing != 0 )); then
  cat >&2 <<'EOF'
Install the missing development tools for your Linux distribution and run this
script again. The CI reference environment is Fedora 44.
EOF
  exit 1
fi

check_pkg() {
  local requirement="$1"
  if ! pkg-config --exists "${requirement}"; then
    printf 'Missing or too-old development dependency: %s\n' "${requirement}" >&2
    return 1
  fi
}

check_pkg 'glib-2.0 >= 2.76' || missing=1
check_pkg 'gtk4 >= 4.14' || missing=1
check_pkg 'vte-2.91-gtk4 >= 0.76' || missing=1

if (( missing != 0 )); then
  cat >&2 <<'EOF'
Install your distribution's GLib, GTK 4, and VTE GTK4 development packages and
run this script again.

Fedora 44 reference packages:
  gcc git glib2-devel gtk4-devel meson ninja-build pkgconf-pkg-config vte291-gtk4-devel
EOF
  exit 1
fi

source_revision="$(git -C "${repo_root}" rev-parse HEAD)"
printf 'GoreeCloud Terminal native acceptance source: %s\n' "${source_revision}"

if [[ "${mode}" == "interactive" ]]; then
  if ! git -C "${repo_root}" diff --quiet -- || ! git -C "${repo_root}" diff --cached --quiet --; then
    cat >&2 <<'EOF'
The working tree has uncommitted changes. Physical-device acceptance must be tied
to an exact source revision. Commit, stash, or discard those changes, then retry.
EOF
    exit 1
  fi
else
  printf 'Automated mode: exact revision is provided by the checked-out CI source.\n'
fi

work_root="$(mktemp -d "${TMPDIR:-/tmp}/goreecloud-terminal-native-acceptance.XXXXXX")"
build_dir="${work_root}/build"
config_dir="${work_root}/config"
mkdir -p "${config_dir}"

cleanup() {
  rm -rf -- "${work_root}"
}
trap cleanup EXIT

printf '\n[1/4] Validating Glaze adoption contract...\n'
python3 "${native_dir}/tools/validate-glaze-contract.py"

printf '\n[2/4] Configuring native build...\n'
meson setup "${build_dir}" "${native_dir}" --buildtype=debugoptimized

printf '\n[3/4] Compiling native build...\n'
meson compile -C "${build_dir}"

printf '\n[4/4] Running native unit tests...\n'
meson test -C "${build_dir}" --print-errorlogs

if [[ "${mode}" == "automated" ]]; then
  printf '\nAutomated workstation-acceptance prerequisites passed for %s.\n' "${source_revision}"
  exit 0
fi

binary="${build_dir}/goreecloud-terminal-native"
settings_file="${config_dir}/goreecloud/terminal/theme.ini"

cat <<'EOF'

Automated checks passed. The remaining steps require physical-device and human
visual/accessibility judgment and therefore are not auto-approved by this script.

FIRST LAUNCH CHECKLIST
  1. Verify the window opens and terminal text is readable.
  2. Cycle Theme through Follow System, Light, Dark, and Deep Dark.
  3. End the first launch on Deep Dark so persistence can be verified.
  4. Right-click the terminal and confirm exactly these product actions appear:
       Copy, Paste, Select All, Clear, New Session, Close Session
     Confirm there is no sudo apt update or other package-management shortcut.
  5. Enter visible non-sensitive text, use Select All / Copy / Paste, then Clear.
     Clear must clear the visible display without placing a clear command in
     shell history.
  6. Verify New Session and Close Session, including Ctrl+Shift+T and Ctrl+Shift+W.
  7. Check visible focus treatment and keyboard operability of application chrome.
  8. If your desktop provides High Contrast, enable it and verify usable contrast.
  9. Check representative Unicode, selection, cursor, shell input, and scrolling.
 10. If an assistive technology such as Orca is available, verify controls and
     local-session labels are understandable and not misleading.

Close GoreeCloud Terminal after completing the first-launch checks.
EOF

printf '\nPress Enter to launch the isolated first pass... '
read -r _
XDG_CONFIG_HOME="${config_dir}" "${binary}"

if [[ ! -f "${settings_file}" ]]; then
  cat >&2 <<EOF

Theme persistence evidence was not created at the isolated expected path:
  ${settings_file}

Set the Theme to Deep Dark before closing the first launch, then rerun this test.
EOF
  exit 1
fi

if ! grep -Fxq 'mode=deep-dark' "${settings_file}"; then
  cat >&2 <<EOF

The isolated Theme Engine preference is not Deep Dark:
  ${settings_file}

The persistence check deliberately requires Deep Dark at the end of the first
launch so restoration can be verified deterministically. Rerun and finish the
first launch on Deep Dark.
EOF
  exit 1
fi

cat <<'EOF'

PERSISTENCE RELAUNCH CHECKLIST
  1. The second launch must restore Deep Dark without reselecting it.
  2. Recheck terminal readability, cursor, selection, and context-menu placement.
  3. Recheck Clear and one New Session / Close Session cycle.
  4. Confirm no unexpected settings from your normal profile appear; this run is
     isolated under a temporary XDG configuration directory.

Close GoreeCloud Terminal after the persistence check.
EOF

printf '\nPress Enter to relaunch with the same isolated preferences... '
read -r _
XDG_CONFIG_HOME="${config_dir}" "${binary}"

cat <<EOF

Laptop acceptance run finished for source:
  ${source_revision}

This script proves the automated build/test prerequisites and exercises the
isolated persistence path, but it cannot decide visual quality, accessibility,
or physical-device correctness for you. Record PASS/FAIL for the checklist above.
Do not classify the native Terminal as Stable if any required item failed or was
not evaluated.
EOF
