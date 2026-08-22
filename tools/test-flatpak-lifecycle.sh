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
app_id='com.goreecloud.Terminal.Devel'

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
        -y|--noninteractive|--reinstall) shift ;;
        *) bundle="$1"; shift ;;
      esac
    done
    [ -n "$bundle" ] || exit 2
    sha256sum "$bundle" | awk '{print $1}' > "$commit_file"
    ;;

  uninstall)
    while [ "$#" -gt 0 ]; do
      case "$1" in
        -y|--noninteractive) shift ;;
        *) [ "$1" = "$app_id" ] || exit 2; shift ;;
      esac
    done
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

# Exact-artifact reinstall must preserve the installed commit and remove the app.
run_validator reinstall --bundle "$baseline" --sha256 "$baseline_sha"
test ! -f "$fake_state/commit"

# Distinct artifacts must support candidate transition and exact baseline rollback.
run_validator transition \
  --baseline "$baseline" --baseline-sha256 "$baseline_sha" \
  --candidate "$candidate" --candidate-sha256 "$candidate_sha"
test ! -f "$fake_state/commit"

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
expect_failure run_validator reinstall --bundle "$baseline" --sha256 "$baseline_sha"
test -f "$fake_state/commit"
rm -f "$fake_state/commit"

# A smoke-check failure must trigger cleanup of an installation created by the harness.
if FAKE_FLATPAK_FAIL_SMOKE=1 run_validator reinstall \
  --bundle "$baseline" --sha256 "$baseline_sha" >/dev/null 2>&1; then
  printf 'Expected smoke-check failure but lifecycle validation succeeded\n' >&2
  exit 1
fi
test ! -f "$fake_state/commit"

# Ordinary lifecycle removal must preserve user data by never using --delete-data.
if grep -Fq -- '--delete-data' "$validator"; then
  # The usage statement may document the prohibition; reject executable invocations only.
  if grep -Eq '^[[:space:]]*flatpak_user[[:space:]]+uninstall.*--delete-data' "$validator"; then
    printf 'Lifecycle harness must not delete Flatpak user data\n' >&2
    exit 1
  fi
fi

echo 'Flatpak lifecycle safety model validated'
