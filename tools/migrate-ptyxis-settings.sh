#!/usr/bin/env bash
set -euo pipefail

umask 077

source_path="/org/gnome/Ptyxis/"
target_kind="production"
target_path="/com/goreecloud/Terminal/"
apply=false
state_dir_arg=""
dconf_bin="${GORECLOUD_TERMINAL_DCONF_BIN:-dconf}"
state_root="${GORECLOUD_TERMINAL_MIGRATION_STATE_ROOT:-${XDG_STATE_HOME:-${HOME}/.local/state}/goreecloud-terminal/settings-migrations}"

usage() {
  cat <<'EOF'
Usage:
  migrate-ptyxis-settings.sh status [--target production|development]
  migrate-ptyxis-settings.sh migrate [--target production|development] [--apply]
  migrate-ptyxis-settings.sh rollback --state DIR [--target production|development] [--apply]

Commands:
  status    Inspect whether the source and target subtrees contain custom values.
  migrate   Copy the complete Ptyxis settings subtree into the selected GoreeCloud
            Terminal namespace. Without --apply, this is a dry run.
  rollback  Restore the target subtree from a migration state's target-before.dconf
            snapshot. Without --apply, this is a dry run.

Options:
  --target production   Use /com/goreecloud/Terminal/ (default).
  --target development  Use /com/goreecloud/Terminal/Devel/.
  --state DIR           Migration state directory to use for rollback.
  --apply               Permit writes. Required for migration or rollback changes.
  -h, --help            Show this help.

Safety model:
  * The source /org/gnome/Ptyxis/ subtree is never reset, loaded, or modified.
  * Migration refuses to write when the target already contains custom values.
  * A target snapshot is created before every write.
  * Failed migration validation automatically restores the pre-migration target.
  * State files are created with private permissions and may contain user-specific
    terminal preferences, profile commands, paths, or custom-link patterns.
EOF
}

fail() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

info() {
  printf '%s\n' "$*"
}

set_target_path() {
  case "$target_kind" in
    production)
      target_path="/com/goreecloud/Terminal/"
      ;;
    development)
      target_path="/com/goreecloud/Terminal/Devel/"
      ;;
    *)
      fail "unsupported target '$target_kind'; use production or development"
      ;;
  esac
}

require_dconf() {
  if [[ "$dconf_bin" == */* ]]; then
    [[ -x "$dconf_bin" ]] || fail "dconf command is not executable: $dconf_bin"
  else
    command -v "$dconf_bin" >/dev/null 2>&1 || fail "dconf is required but was not found in PATH"
  fi
}

dump_path() {
  local path="$1"
  local output="$2"
  "$dconf_bin" dump "$path" >"$output"
  chmod 600 "$output"
}

has_values() {
  local file="$1"
  [[ -s "$file" ]]
}

make_state_dir() {
  local label="$1"
  local timestamp
  timestamp="$(date -u +%Y%m%dT%H%M%SZ)"
  mkdir -p "$state_root"
  chmod 700 "$state_root"
  mktemp -d "${state_root}/${label}-${timestamp}-XXXXXX"
}

write_manifest() {
  local dir="$1"
  local operation="$2"
  cat >"${dir}/manifest.txt" <<EOF
format=1
operation=${operation}
source_path=${source_path}
target_kind=${target_kind}
target_path=${target_path}
created_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)
EOF
  chmod 600 "${dir}/manifest.txt"
}

manifest_has_line() {
  local manifest="$1"
  local expected="$2"
  grep -Fqx -- "$expected" "$manifest"
}

restore_target_from_snapshot() {
  local snapshot="$1"
  "$dconf_bin" reset -f "$target_path"
  if [[ -s "$snapshot" ]]; then
    "$dconf_bin" load "$target_path" <"$snapshot"
  fi
}

verify_snapshot_matches_target() {
  local expected="$1"
  local actual="$2"
  dump_path "$target_path" "$actual"
  cmp -s "$expected" "$actual"
}

command_name="${1:-status}"
if [[ $# -gt 0 ]]; then
  shift
fi

while [[ $# -gt 0 ]]; do
  case "$1" in
    --target)
      [[ $# -ge 2 ]] || fail "--target requires production or development"
      target_kind="$2"
      shift 2
      ;;
    --state)
      [[ $# -ge 2 ]] || fail "--state requires a directory"
      state_dir_arg="$2"
      shift 2
      ;;
    --apply)
      apply=true
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      fail "unknown argument '$1'"
      ;;
  esac
done

set_target_path
require_dconf

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

source_before="${tmp_dir}/source-before.dconf"
target_before="${tmp_dir}/target-before.dconf"
dump_path "$source_path" "$source_before"
dump_path "$target_path" "$target_before"

case "$command_name" in
  status)
    info "GoreeCloud Terminal settings migration status"
    info "Source: $source_path"
    if has_values "$source_before"; then
      info "Source custom values: present"
    else
      info "Source custom values: none"
    fi
    info "Target: $target_path"
    if has_values "$target_before"; then
      info "Target custom values: present"
    else
      info "Target custom values: none"
    fi
    ;;

  migrate)
    info "GoreeCloud Terminal settings migration"
    info "Source: $source_path"
    info "Target: $target_path"

    if ! has_values "$source_before"; then
      info "No custom Ptyxis values were found. Nothing to migrate."
      exit 0
    fi

    if has_values "$target_before"; then
      fail "target already contains custom values; migration is intentionally fail-closed"
    fi

    if [[ "$apply" != true ]]; then
      info "Dry run: source values are present and the target is empty."
      info "No settings were changed. Re-run with --apply to perform the migration."
      exit 0
    fi

    migration_state="$(make_state_dir migrate)"
    write_manifest "$migration_state" migrate
    cp "$source_before" "${migration_state}/source.dconf"
    cp "$target_before" "${migration_state}/target-before.dconf"
    chmod 600 "${migration_state}/source.dconf" "${migration_state}/target-before.dconf"

    migration_failed=false
    if ! "$dconf_bin" load "$target_path" <"${migration_state}/source.dconf"; then
      migration_failed=true
    else
      target_after="${migration_state}/target-after.dconf"
      source_after="${tmp_dir}/source-after.dconf"
      dump_path "$target_path" "$target_after"
      dump_path "$source_path" "$source_after"

      if ! cmp -s "${migration_state}/source.dconf" "$target_after"; then
        migration_failed=true
      fi

      if ! cmp -s "$source_before" "$source_after"; then
        migration_failed=true
      fi
    fi

    if [[ "$migration_failed" == true ]]; then
      info "Migration validation failed; restoring the target snapshot." >&2
      restore_target_from_snapshot "${migration_state}/target-before.dconf"
      restored="${tmp_dir}/target-restored.dconf"
      if ! verify_snapshot_matches_target "${migration_state}/target-before.dconf" "$restored"; then
        fail "migration failed and automatic target restoration could not be verified; state: $migration_state"
      fi
      printf 'result=restored-after-failure\n' >>"${migration_state}/manifest.txt"
      fail "migration failed; the pre-migration target was restored; state: $migration_state"
    fi

    printf 'result=success\n' >>"${migration_state}/manifest.txt"
    info "Migration completed and validated."
    info "State: $migration_state"
    info "The source Ptyxis subtree was verified unchanged."
    ;;

  rollback)
    [[ -n "$state_dir_arg" ]] || fail "rollback requires --state DIR"
    [[ -d "$state_dir_arg" ]] || fail "rollback state directory does not exist: $state_dir_arg"
    manifest="${state_dir_arg}/manifest.txt"
    snapshot="${state_dir_arg}/target-before.dconf"
    [[ -f "$manifest" ]] || fail "missing rollback manifest: $manifest"
    [[ -f "$snapshot" ]] || fail "missing rollback snapshot: $snapshot"

    manifest_has_line "$manifest" "format=1" || fail "unsupported or invalid rollback manifest"
    manifest_has_line "$manifest" "operation=migrate" || fail "rollback state is not a migration state"
    manifest_has_line "$manifest" "source_path=$source_path" || fail "rollback source path does not match the supported Ptyxis source"
    manifest_has_line "$manifest" "target_path=$target_path" || fail "rollback state target does not match selected target '$target_kind'"

    info "GoreeCloud Terminal settings rollback"
    info "Target: $target_path"
    info "Migration state: $state_dir_arg"

    if [[ "$apply" != true ]]; then
      info "Dry run: rollback state is valid."
      info "No settings were changed. Re-run with --apply to restore the target snapshot."
      exit 0
    fi

    rollback_state="$(make_state_dir rollback)"
    write_manifest "$rollback_state" rollback
    cp "$target_before" "${rollback_state}/target-before-rollback.dconf"
    cp "$snapshot" "${rollback_state}/restored-target.dconf"
    chmod 600 "${rollback_state}/target-before-rollback.dconf" "${rollback_state}/restored-target.dconf"

    source_pre_rollback="${tmp_dir}/source-pre-rollback.dconf"
    cp "$source_before" "$source_pre_rollback"

    restore_target_from_snapshot "$snapshot"
    target_after_rollback="${rollback_state}/target-after-rollback.dconf"
    if ! verify_snapshot_matches_target "$snapshot" "$target_after_rollback"; then
      info "Rollback validation failed; restoring the target state captured immediately before rollback." >&2
      restore_target_from_snapshot "${rollback_state}/target-before-rollback.dconf"
      fail "rollback validation failed; pre-rollback target was restored; state: $rollback_state"
    fi

    source_post_rollback="${tmp_dir}/source-post-rollback.dconf"
    dump_path "$source_path" "$source_post_rollback"
    if ! cmp -s "$source_pre_rollback" "$source_post_rollback"; then
      fail "source subtree changed unexpectedly during rollback validation"
    fi

    printf 'result=success\n' >>"${rollback_state}/manifest.txt"
    info "Rollback completed and validated."
    info "Recovery state: $rollback_state"
    info "The source Ptyxis subtree was verified unchanged."
    ;;

  help)
    usage
    ;;

  *)
    fail "unknown command '$command_name'; use status, migrate, rollback, or help"
    ;;
esac
