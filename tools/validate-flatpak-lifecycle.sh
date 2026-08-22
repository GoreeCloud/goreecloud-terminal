#!/usr/bin/env bash
set -euo pipefail

app_id="com.goreecloud.Terminal.Devel"
flatpak_bin="${GORECLOUD_TERMINAL_FLATPAK_BIN:-flatpak}"
installed_by_script=false

usage() {
  cat <<'EOF'
Usage:
  validate-flatpak-lifecycle.sh reinstall \
    --bundle PATH --sha256 SHA256 \
    [--app-id com.goreecloud.Terminal.Devel]

  validate-flatpak-lifecycle.sh transition \
    --baseline PATH --baseline-sha256 SHA256 \
    --candidate PATH --candidate-sha256 SHA256 \
    [--app-id com.goreecloud.Terminal.Devel]

The lifecycle harness is intentionally restricted to the isolated GoreeCloud
Terminal development identity. It never uses flatpak uninstall --delete-data.
EOF
}

die() {
  printf 'GoreeCloud Terminal lifecycle validation failed: %s\n' "$*" >&2
  exit 1
}

flatpak_user() {
  "$flatpak_bin" --user "$@"
}

is_installed() {
  flatpak_user info "$app_id" >/dev/null 2>&1
}

cleanup() {
  if [ "$installed_by_script" = true ] && is_installed; then
    flatpak_user uninstall -y --noninteractive "$app_id" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT INT TERM

require_development_app_id() {
  [ "$app_id" = "com.goreecloud.Terminal.Devel" ] || \
    die "only com.goreecloud.Terminal.Devel is accepted by this development lifecycle harness"
}

is_sha256() {
  printf '%s\n' "$1" | grep -Eq '^[0-9a-fA-F]{64}$'
}

verify_sha256() {
  local file="$1"
  local expected="$2"
  local actual

  [ -f "$file" ] || die "bundle not found: $file"
  is_sha256 "$expected" || die "SHA-256 must contain exactly 64 hexadecimal characters for $file"

  expected="$(printf '%s' "$expected" | tr 'A-F' 'a-f')"
  actual="$(sha256sum "$file" | awk '{print $1}')"
  [ "$actual" = "$expected" ] || die "SHA-256 mismatch for $file"
}

installed_commit() {
  flatpak_user info --show-commit "$app_id"
}

validate_installed_identity() {
  local runtime ref commit version_output ssh_help profile_help

  runtime="$(flatpak_user info --show-runtime "$app_id")"
  case "$runtime" in
    org.gnome.Platform/*/50) ;;
    *) die "unexpected installed runtime: $runtime" ;;
  esac

  ref="$(flatpak_user info --show-ref "$app_id")"
  case "$ref" in
    app/com.goreecloud.Terminal.Devel/*) ;;
    *) die "unexpected installed Flatpak ref: $ref" ;;
  esac

  commit="$(installed_commit)"
  printf '%s\n' "$commit" | grep -Eq '^[0-9a-f]{64}$' || \
    die "installed commit is not a 64-character lowercase hexadecimal OSTree commit"

  version_output="$(flatpak_user run --command=goreecloud-terminal "$app_id" --version 2>&1)"
  printf '%s\n' "$version_output" | grep -Fq 'GoreeCloud Terminal' || \
    die "canonical launcher version smoke check failed"

  ssh_help="$(flatpak_user run --command=goreecloud-terminal "$app_id" ssh --help 2>&1)"
  printf '%s\n' "$ssh_help" | grep -Fq 'Standard OpenSSH authentication and configuration remain' || \
    die "SSH help smoke check failed"

  profile_help="$(flatpak_user run --command=goreecloud-terminal "$app_id" profile --help 2>&1)"
  printf '%s\n' "$profile_help" | grep -Fq 'WORKSPACE<TAB>PROFILE<TAB>SSH_HOST_ALIAS' || \
    die "profile help smoke check failed"
}

ensure_clean_start() {
  require_development_app_id
  if is_installed; then
    die "$app_id is already installed; refusing to modify a pre-existing installation"
  fi
}

install_initial() {
  local bundle="$1"
  flatpak_user install -y --noninteractive "$bundle"
  installed_by_script=true
  validate_installed_identity
}

remove_current_preserve_data() {
  is_installed || die "$app_id is not installed before lifecycle replacement"
  flatpak_user uninstall -y --noninteractive "$app_id"
  if is_installed; then
    die "$app_id remained installed after lifecycle replacement removal"
  fi
}

replace_with_bundle() {
  local bundle="$1"

  # Flatpak 1.14.6 on the supported CI baseline does not replace an already
  # installed app when a local single-file bundle is passed with --reinstall.
  # Remove only the application ref, preserve ~/.var/app data, then install
  # the already hash-verified exact local bundle.
  remove_current_preserve_data
  flatpak_user install -y --noninteractive "$bundle"
  validate_installed_identity
}

uninstall_and_verify() {
  remove_current_preserve_data
  installed_by_script=false
}

mode="${1:-}"
case "$mode" in
  reinstall|transition) shift ;;
  -h|--help|'') usage; [ -n "$mode" ] && [ "$mode" != "-h" ] && [ "$mode" != "--help" ] && exit 64 || exit 0 ;;
  *) usage >&2; die "unknown lifecycle mode: $mode" ;;
esac

bundle=""
bundle_sha=""
baseline=""
baseline_sha=""
candidate=""
candidate_sha=""

while [ "$#" -gt 0 ]; do
  case "$1" in
    --bundle)
      [ "$#" -ge 2 ] || die "--bundle requires a path"
      bundle="$2"; shift 2 ;;
    --sha256)
      [ "$#" -ge 2 ] || die "--sha256 requires a value"
      bundle_sha="$2"; shift 2 ;;
    --baseline)
      [ "$#" -ge 2 ] || die "--baseline requires a path"
      baseline="$2"; shift 2 ;;
    --baseline-sha256)
      [ "$#" -ge 2 ] || die "--baseline-sha256 requires a value"
      baseline_sha="$2"; shift 2 ;;
    --candidate)
      [ "$#" -ge 2 ] || die "--candidate requires a path"
      candidate="$2"; shift 2 ;;
    --candidate-sha256)
      [ "$#" -ge 2 ] || die "--candidate-sha256 requires a value"
      candidate_sha="$2"; shift 2 ;;
    --app-id)
      [ "$#" -ge 2 ] || die "--app-id requires a value"
      app_id="$2"; shift 2 ;;
    -h|--help)
      usage; exit 0 ;;
    *) die "unknown argument: $1" ;;
  esac
done

ensure_clean_start

case "$mode" in
  reinstall)
    [ -n "$bundle" ] || die "reinstall mode requires --bundle"
    [ -n "$bundle_sha" ] || die "reinstall mode requires --sha256"
    verify_sha256 "$bundle" "$bundle_sha"

    install_initial "$bundle"
    first_commit="$(installed_commit)"
    replace_with_bundle "$bundle"
    second_commit="$(installed_commit)"
    [ "$second_commit" = "$first_commit" ] || \
      die "exact-artifact replacement changed the installed OSTree commit"

    uninstall_and_verify
    printf 'Exact-artifact reinstall lifecycle validated\n'
    printf 'Application ID: %s\n' "$app_id"
    printf 'Installed commit: %s\n' "$first_commit"
    ;;

  transition)
    [ -n "$baseline" ] || die "transition mode requires --baseline"
    [ -n "$baseline_sha" ] || die "transition mode requires --baseline-sha256"
    [ -n "$candidate" ] || die "transition mode requires --candidate"
    [ -n "$candidate_sha" ] || die "transition mode requires --candidate-sha256"

    verify_sha256 "$baseline" "$baseline_sha"
    verify_sha256 "$candidate" "$candidate_sha"

    baseline_sha="$(printf '%s' "$baseline_sha" | tr 'A-F' 'a-f')"
    candidate_sha="$(printf '%s' "$candidate_sha" | tr 'A-F' 'a-f')"
    [ "$baseline_sha" != "$candidate_sha" ] || \
      die "transition mode requires two cryptographically distinct bundle artifacts"

    install_initial "$baseline"
    baseline_commit="$(installed_commit)"

    replace_with_bundle "$candidate"
    candidate_commit="$(installed_commit)"
    [ "$candidate_commit" != "$baseline_commit" ] || \
      die "candidate did not produce a distinct installed OSTree commit; refusing to claim upgrade acceptance"

    replace_with_bundle "$baseline"
    rollback_commit="$(installed_commit)"
    [ "$rollback_commit" = "$baseline_commit" ] || \
      die "rollback did not restore the exact baseline OSTree commit"

    uninstall_and_verify
    printf 'Distinct-artifact replacement transition and exact rollback validated\n'
    printf 'Application ID: %s\n' "$app_id"
    printf 'Baseline commit: %s\n' "$baseline_commit"
    printf 'Candidate commit: %s\n' "$candidate_commit"
    printf 'Rollback commit: %s\n' "$rollback_commit"
    ;;
esac
