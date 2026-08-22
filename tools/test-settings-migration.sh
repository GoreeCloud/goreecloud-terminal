#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
migration_tool="${repo_root}/tools/migrate-ptyxis-settings.sh"

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

fake_root="${tmp_dir}/fake-dconf"
state_root="${tmp_dir}/state"
mkdir -p "$fake_root" "$state_root"

cat >"${tmp_dir}/dconf" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

root="${FAKE_DCONF_ROOT:?FAKE_DCONF_ROOT is required}"
command_name="${1:?command required}"
shift

path_to_file() {
  case "$1" in
    /org/gnome/Ptyxis/)
      printf '%s/source.dconf\n' "$root"
      ;;
    /com/goreecloud/Terminal/)
      printf '%s/target-production.dconf\n' "$root"
      ;;
    /com/goreecloud/Terminal/Devel/)
      printf '%s/target-development.dconf\n' "$root"
      ;;
    *)
      printf 'unsupported fake dconf path: %s\n' "$1" >&2
      exit 64
      ;;
  esac
}

case "$command_name" in
  dump)
    file="$(path_to_file "${1:?path required}")"
    [[ -f "$file" ]] && cat "$file"
    ;;
  load)
    file="$(path_to_file "${1:?path required}")"
    cat >"$file"
    ;;
  reset)
    [[ "${1:-}" == "-f" ]] || exit 64
    file="$(path_to_file "${2:?path required}")"
    : >"$file"
    ;;
  *)
    printf 'unsupported fake dconf command: %s\n' "$command_name" >&2
    exit 64
    ;;
esac
EOF
chmod 700 "${tmp_dir}/dconf"

source_fixture='[/]
interface-style='"'"'light'"'"'
profile-uuids=['"'"'profile-a'"'"']
default-profile-uuid='"'"'profile-a'"'"'

[Profiles/profile-a/]
label='"'"'Imported profile'"'"'
scrollback-lines=25000
custom-command='"'"'/bin/bash'"'"'

[Shortcuts/]
new-tab='"'"'<ctrl><shift>t'"'"'
'
printf '%s' "$source_fixture" >"${fake_root}/source.dconf"
: >"${fake_root}/target-production.dconf"
: >"${fake_root}/target-development.dconf"

run_tool() {
  FAKE_DCONF_ROOT="$fake_root" \
  GORECLOUD_TERMINAL_DCONF_BIN="${tmp_dir}/dconf" \
  GORECLOUD_TERMINAL_MIGRATION_STATE_ROOT="$state_root" \
  bash "$migration_tool" "$@"
}

assert_same() {
  cmp -s "$1" "$2" || {
    printf 'files differ: %s %s\n' "$1" "$2" >&2
    exit 1
  }
}

assert_empty() {
  [[ ! -s "$1" ]] || {
    printf 'expected empty file: %s\n' "$1" >&2
    exit 1
  }
}

# Status and dry-run migration must never write either namespace.
run_tool status >/dev/null
run_tool migrate >/dev/null
assert_empty "${fake_root}/target-production.dconf"
printf '%s' "$source_fixture" >"${tmp_dir}/expected-source.dconf"
assert_same "${tmp_dir}/expected-source.dconf" "${fake_root}/source.dconf"

# Apply copies the complete relative subtree and leaves the upstream source intact.
run_tool migrate --apply >"${tmp_dir}/migrate-output.txt"
assert_same "${fake_root}/source.dconf" "${fake_root}/target-production.dconf"
assert_same "${tmp_dir}/expected-source.dconf" "${fake_root}/source.dconf"
migration_state="$(find "$state_root" -mindepth 1 -maxdepth 1 -type d -name 'migrate-*' -print -quit)"
[[ -n "$migration_state" ]]
[[ -f "${migration_state}/manifest.txt" ]]
[[ -f "${migration_state}/source.dconf" ]]
[[ -f "${migration_state}/target-before.dconf" ]]
[[ -f "${migration_state}/target-after.dconf" ]]
grep -Fqx 'result=success' "${migration_state}/manifest.txt"

# A second migration must fail closed instead of replacing custom target data.
if run_tool migrate --apply >"${tmp_dir}/conflict-output.txt" 2>&1; then
  printf 'expected migration conflict to fail\n' >&2
  exit 1
fi
assert_same "${fake_root}/source.dconf" "${fake_root}/target-production.dconf"

# Rollback dry-run must not modify current target data.
printf '%s' 'changed-after-migration' >"${fake_root}/target-production.dconf"
printf '%s' 'changed-after-migration' >"${tmp_dir}/changed-target.dconf"
run_tool rollback --state "$migration_state" >/dev/null
assert_same "${tmp_dir}/changed-target.dconf" "${fake_root}/target-production.dconf"

# Applied rollback restores the exact pre-migration target (empty here) and keeps source intact.
run_tool rollback --state "$migration_state" --apply >"${tmp_dir}/rollback-output.txt"
assert_empty "${fake_root}/target-production.dconf"
assert_same "${tmp_dir}/expected-source.dconf" "${fake_root}/source.dconf"
rollback_state="$(find "$state_root" -mindepth 1 -maxdepth 1 -type d -name 'rollback-*' -print -quit)"
[[ -n "$rollback_state" ]]
grep -Fqx 'result=success' "${rollback_state}/manifest.txt"

# Development migration uses the isolated GoreeCloud development namespace.
run_tool migrate --target development --apply >/dev/null
assert_same "${fake_root}/source.dconf" "${fake_root}/target-development.dconf"
assert_empty "${fake_root}/target-production.dconf"

printf 'settings migration tests passed\n'
