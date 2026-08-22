#!/usr/bin/env bash
set -euo pipefail

root="${GORECLOUD_TERMINAL_ROOT:-/}"
app_id="${GORECLOUD_TERMINAL_APP_ID:-com.goreecloud.Terminal}"
live=0

usage() {
  cat <<'EOF'
Usage: tools/validate-runtime-identity.sh [OPTIONS]

Validate GoreeCloud Terminal product-identity artifacts in a staged or installed tree.

Options:
  --root PATH       Filesystem root to inspect. Defaults to /.
  --app-id ID       Expected application ID. Defaults to com.goreecloud.Terminal.
  --live            Also run non-graphical live checks. Requires --root /.
  -h, --help        Show this help.

Examples:
  tools/validate-runtime-identity.sh --root "$PWD/_install" --app-id com.goreecloud.Terminal.Devel
  tools/validate-runtime-identity.sh --live
EOF
}

fail() {
  printf 'FAIL: %s\n' "$*" >&2
  exit 1
}

pass() {
  printf 'PASS: %s\n' "$*"
}

while (($#)); do
  case "$1" in
    --root)
      (($# >= 2)) || fail "--root requires a value"
      root="$2"
      shift 2
      ;;
    --app-id)
      (($# >= 2)) || fail "--app-id requires a value"
      app_id="$2"
      shift 2
      ;;
    --live)
      live=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      fail "unknown option: $1"
      ;;
  esac
done

[[ -n "$root" ]] || fail "root must not be empty"
[[ -n "$app_id" ]] || fail "app ID must not be empty"

if [[ "$root" != "/" ]]; then
  root="${root%/}"
fi

rooted() {
  local absolute="$1"
  [[ "$absolute" == /* ]] || fail "internal error: expected absolute path: $absolute"
  if [[ "$root" == "/" ]]; then
    printf '%s\n' "$absolute"
  else
    printf '%s%s\n' "$root" "$absolute"
  fi
}

require_file() {
  local path="$1"
  [[ -f "$path" ]] || fail "missing file: $path"
  pass "file exists: $path"
}

require_executable() {
  local path="$1"
  [[ -x "$path" ]] || fail "missing executable: $path"
  pass "executable exists: $path"
}

require_contains() {
  local path="$1"
  local text="$2"
  grep -Fq -- "$text" "$path" || fail "expected text not found in $path: $text"
  pass "validated content: $path"
}

launcher="$(rooted /usr/bin/goreecloud-terminal)"
compat_binary="$(rooted /usr/bin/ptyxis)"
agent_binary="$(rooted /usr/libexec/ptyxis-agent)"
desktop_file="$(rooted "/usr/share/applications/${app_id}.desktop")"
metainfo_file="$(rooted "/usr/share/metainfo/${app_id}.metainfo.xml")"
schema_file="$(rooted "/usr/share/glib-2.0/schemas/${app_id}.gschema.xml")"
dbus_file="$(rooted "/usr/share/dbus-1/services/${app_id}.service")"
product_man="$(rooted /usr/share/man/man1/goreecloud-terminal.1)"
compat_man="$(rooted /usr/share/man/man1/ptyxis.1)"
icons_root="$(rooted /usr/share/icons)"

require_executable "$launcher"
require_executable "$compat_binary"
require_executable "$agent_binary"
require_file "$desktop_file"
require_file "$metainfo_file"
require_file "$schema_file"
require_file "$dbus_file"

if [[ -f "$product_man" ]]; then
  pass "file exists: $product_man"
elif [[ -f "${product_man}.gz" ]]; then
  pass "compressed file exists: ${product_man}.gz"
else
  fail "missing GoreeCloud Terminal man page"
fi

if [[ -f "$compat_man" ]]; then
  pass "file exists: $compat_man"
elif [[ -f "${compat_man}.gz" ]]; then
  pass "compressed file exists: ${compat_man}.gz"
else
  fail "missing Ptyxis compatibility man page"
fi

require_contains "$launcher" 'ptyxis_bin="${GORECLOUD_TERMINAL_PTYXIS_BIN:-ptyxis}"'
require_contains "$launcher" 'exec "$ptyxis_bin" "$@"'
require_contains "$launcher" 'exec "$ptyxis_bin" --new-window -- ssh "$@"'
require_contains "$launcher" 'exec "$ptyxis_bin" --tab -- ssh "$@"'
require_contains "$desktop_file" 'Name=GoreeCloud Terminal'
require_contains "$desktop_file" 'Exec=goreecloud-terminal'
require_contains "$metainfo_file" "<id>${app_id}</id>"
require_contains "$dbus_file" "Name=${app_id}"
require_contains "$dbus_file" 'Exec=/usr/bin/goreecloud-terminal --gapplication-service'

find "$icons_root" -type f -name "${app_id}*" -print -quit | grep -q . \
  || fail "no installed icon found for ${app_id} under ${icons_root}"
pass "icon exists for ${app_id}"

if ((live)); then
  [[ "$root" == "/" ]] || fail "--live requires --root /"

  command -v goreecloud-terminal >/dev/null 2>&1 \
    || fail "goreecloud-terminal is not discoverable through PATH"
  pass "goreecloud-terminal is discoverable through PATH"

  command -v ptyxis >/dev/null 2>&1 \
    || fail "ptyxis compatibility command is not discoverable through PATH"
  pass "ptyxis compatibility command is discoverable through PATH"

  version_output="$(goreecloud-terminal --version 2>&1)" \
    || fail "goreecloud-terminal --version failed"
  [[ -n "$version_output" ]] || fail "goreecloud-terminal --version returned no output"
  pass "goreecloud-terminal --version completed"

  if command -v gsettings >/dev/null 2>&1; then
    gsettings list-schemas | grep -Fxq "$app_id" \
      || fail "GSettings schema is not registered: $app_id"
    pass "GSettings schema is registered: $app_id"
  fi
fi

cat <<EOF

GoreeCloud Terminal identity acceptance completed.
Root: $root
Application ID: $app_id
Live checks: $([[ "$live" == 1 ]] && printf 'enabled' || printf 'disabled')

Manual graphical acceptance is still required for desktop launch, D-Bus single-instance
behavior, icon presentation, Glaze UI light/dark appearance, Wardveil context transitions,
and representative terminal/clipboard/SSH/container/sudo workflows.
EOF
