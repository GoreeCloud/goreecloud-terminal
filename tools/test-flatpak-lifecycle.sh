#!/usr/bin/env bash
set -euo pipefail

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
validator="$repo_root/tools/validate-flatpak-lifecycle.sh"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT INT TERM

fake_bin="$tmp/bin"
fake_state="$tmp/state"
mkdir -p "$fake_bin" "$fake_state"

cat > "$fake_bin/flatpak" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
state_dir="${FAKE_FLATPAK_STATE:?}"
mkdir -p "$state_dir"
commit_file="$state_dir/commit"
log_file="$state_dir/commands.log"
data_file="$state_dir/user-data"
app_id='com.goreecloud.Terminal.Devel'

printf '%s\n' "$*" >> "$log_file"

[ "${1:-}" = '--user' ] || { printf 'missing --user\n' >&2; exit 2; }
shift
cmd="${1:-}"
shift || true

case "$cmd" in
  info)
    show=''
    case "${1:-}" in
      --show-commit|--show-runtime|--show-ref) show="$1"; shift ;;
    esac
    [ "${1:-}" = "$app_id" ] || exit 2
    [ -f "$commit_file" ] || exit 1
    case "$show" in
      --show-commit) cat "$commit_file" ;;
      --show-runtime) printf 'org.gnome.Platform/x86_64/50\n' ;;
      --show-ref) printf 'app/com.goreecloud.Terminal.Devel/x86_64/master\n' ;;
      '') printf 'Name: GoreeCloud Terminal\n' ;;
    esac
    ;;

  install)
    bundle=''
    while [ "$#" -gt 0 ]; do
      case "$1" in
        -y|--noninteractive) shift ;;
        --reinstall)
          printf 'fake backend intentionally rejects local-bundle --reinstall\n' >&2
          exit 23
          ;;
        *) bundle="$1"; shift ;;
      esac
    done
    [ -n "$bundle" ] || exit 2
    if [ -f "$commit_file" ]; then
      printf 'already installed\n' >&2
      exit 24
    fi
    sha256sum "$bundle" | awk '{print $1}' > "$commit_file"
    ;;

  uninstall)
    while [ "$#" -gt 0 ]; do
      case "$1" in
        -y|--noninteractive) shift ;;
        --delete-data)
          printf 'fake backend refuses destructive data deletion\n' >&2
          exit 25
          ;;
        *) [ "$1" = "$app_id" ] || exit 2; shift ;;
      esac
    done
    [ -f "$commit_file" ] || exit 1
    rm -f "$commit_file"
    ;;

  run)
    [ "${FAKE_FLATPAK_FAIL_SMOKE:-0}" != 1 ] || exit 9
    command_name=''
    case "${1:-}" in
      --command=*) command_name="${1#--command=}"; shift ;;
    esac
    [ "$command_name" = 'goreecloud-terminal' ] || exit 2
    [ "${1:-}" = "$app_id" ] || exit 2
    [ -f "$commit_file" ] || exit 1
    shift
    case "${1:-}" in
      --version)
        printf 'GoreeCloud Terminal 50.2\n'
        ;;
      ssh)
        [ "${2:-}" = '--help' ] || exit 2
        printf 'Standard OpenSSH authentication and configuration remain authoritative.\n'
        ;;
      profile)
        [ "${2:-}" = '--help' ] || exit 2
        printf 'WORKSPACE<TAB>PROFILE<TAB>SSH_HOST_ALIAS\n'
        ;;
      *) exit 2 ;;
    esac
    ;;

  *)
    printf 'unsupported fake flatpak command: %s\n' "$cmd" >&2
    exit 2
    ;;
esac

# The fake backend models normal Flatpak uninstall semantics: user data is
# independent of the installed app ref unless --delete-data is explicitly used.
[ -e "$data_file" ] || :
EOF
chmod +x "$fake_bin/flatpak"

baseline="$tmp/baseline.flatpak"
candidate="$tmp/candidate.flatpak"
printf 'baseline GoreeCloud Terminal development package\n' > "$baseline"
printf 'candidate GoreeCloud Terminal development package\n' > "$candidate"
baseline_sha="$(sha256sum "$baseline" | awk '{print $1}')"
candidate_sha="$(sha256sum "$candidate" | awk '{print $1}')"

run_validator() {
  FAKE_FLATPAK_STATE="$fake_state" \
  GORECLOUD_TERMINAL_FLATPAK_BIN="$fake_bin/flatpak" \
    "$validator" "$@"
}

expect_failure() {
  if "$@" >/dev/null 2>&1; then
    printf 'Expected lifecycle validation failure but command succeeded: %s\n' "$*" >&2
    exit 1
  fi
}

reset_log() {
  : > "$fake_state/commands.log"
}

assert_no_unsupported_local_reinstall() {
  if grep -Fq -- '--reinstall' "$fake_state/commands.log"; then
    printf 'Lifecycle harness attempted unsupported local-bundle --reinstall\n' >&2
    exit 1
  fi
}

# Exact-artifact reinstall must preserve the installed commit, preserve user
# data across explicit app-ref replacement, and remove the app at the end.
printf 'synthetic-user-data\n' > "$fake_state/user-data"
reset_log
run_validator reinstall --bundle "$baseline" --sha256 "$baseline_sha"
test ! -f "$fake_state/commit"
test "$(cat "$fake_state/user-data")" = 'synthetic-user-data'
assert_no_unsupported_local_reinstall
install_count="$(grep -c -- '--user install ' "$fake_state/commands.log")"
uninstall_count="$(grep -c -- '--user uninstall ' "$fake_state/commands.log")"
[ "$install_count" -eq 2 ]
[ "$uninstall_count" -eq 2 ]

# Distinct artifacts must support data-preserving candidate replacement and
# exact baseline rollback without relying on local-bundle --reinstall.
printf 'transition-user-data\n' > "$fake_state/user-data"
reset_log
run_validator transition \
  --baseline "$baseline" --baseline-sha256 "$baseline_sha" \
  --candidate "$candidate" --candidate-sha256 "$candidate_sha"
test ! -f "$fake_state/commit"
test "$(cat "$fake_state/user-data")" = 'transition-user-data'
assert_no_unsupported_local_reinstall
install_count="$(grep -c -- '--user install ' "$fake_state/commands.log")"
uninstall_count="$(grep -c -- '--user uninstall ' "$fake_state/commands.log")"
[ "$install_count" -eq 3 ]
[ "$uninstall_count" -eq 3 ]

# Production identity is never accepted by this development-only lifecycle harness.
expect_failure run_validator reinstall \
  --bundle "$baseline" --sha256 "$baseline_sha" \
  --app-id com.goreecloud.Terminal

# Hash mismatch must fail before installation.
wrong_sha='0000000000000000000000000000000000000000000000000000000000000000'
expect_failure run_validator reinstall --bundle "$baseline" --sha256 "$wrong_sha"
test ! -f "$fake_state/commit"

# A 64-character value containing non-hexadecimal data must also fail before installation.
malformed_sha="${baseline_sha%?}z"
expect_failure run_validator reinstall --bundle "$baseline" --sha256 "$malformed_sha"
test ! -f "$fake_state/commit"

# Transition mode must reject cryptographically identical artifacts.
expect_failure run_validator transition \
  --baseline "$baseline" --baseline-sha256 "$baseline_sha" \
  --candidate "$baseline" --candidate-sha256 "$baseline_sha"
test ! -f "$fake_state/commit"

# A pre-existing installation must not be modified.
printf '%064d\n' 1 > "$fake_state/commit"
reset_log
expect_failure run_validator reinstall --bundle "$baseline" --sha256 "$baseline_sha"
test -f "$fake_state/commit"
if grep -Eq -- '--user (install|uninstall) ' "$fake_state/commands.log"; then
  printf 'Pre-existing installation was modified by lifecycle harness\n' >&2
  exit 1
fi
rm -f "$fake_state/commit"

# A smoke-check failure must trigger cleanup of an installation created by the harness.
reset_log
if FAKE_FLATPAK_FAIL_SMOKE=1 run_validator reinstall \
  --bundle "$baseline" --sha256 "$baseline_sha" >/dev/null 2>&1; then
  printf 'Expected smoke-check failure but lifecycle validation succeeded\n' >&2
  exit 1
fi
test ! -f "$fake_state/commit"
test -f "$fake_state/user-data"
assert_no_unsupported_local_reinstall

# Ordinary lifecycle removal must preserve user data by never using --delete-data.
if grep -Eq '^[[:space:]]*flatpak_user[[:space:]]+uninstall.*--delete-data' "$validator"; then
  printf 'Lifecycle harness must not delete Flatpak user data\n' >&2
  exit 1
fi

echo 'Flatpak lifecycle safety model validated'
