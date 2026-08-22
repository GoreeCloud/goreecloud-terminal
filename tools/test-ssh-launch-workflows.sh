#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
launcher="${repo_root}/data/goreecloud-terminal"

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

capture="${tmp_dir}/argv.txt"
cat >"${tmp_dir}/ptyxis" <<'EOF'
#!/bin/sh
set -eu
: "${GORECLOUD_TERMINAL_TEST_CAPTURE:?capture path required}"
: >"$GORECLOUD_TERMINAL_TEST_CAPTURE"
for arg in "$@"; do
  printf '%s\n' "$arg" >>"$GORECLOUD_TERMINAL_TEST_CAPTURE"
done
EOF
chmod 700 "${tmp_dir}/ptyxis"

run_launcher() {
  GORECLOUD_TERMINAL_PTYXIS_BIN="${tmp_dir}/ptyxis" \
  GORECLOUD_TERMINAL_TEST_CAPTURE="$capture" \
  sh "$launcher" "$@"
}

assert_args() {
  local expected="$1"
  printf '%s' "$expected" >"${tmp_dir}/expected.txt"
  if ! cmp -s "${tmp_dir}/expected.txt" "$capture"; then
    printf 'unexpected launcher arguments\nexpected:\n' >&2
    cat "${tmp_dir}/expected.txt" >&2
    printf 'actual:\n' >&2
    cat "$capture" >&2
    exit 1
  fi
}

# Existing Ptyxis/GoreeCloud CLI behavior remains an argument-transparent pass-through.
run_launcher --version
assert_args $'--version\n'

# The default SSH convenience opens a new window and preserves all OpenSSH args.
run_launcher ssh server-alias -p 2222 -o StrictHostKeyChecking=yes
assert_args $'--new-window\n--\nssh\nserver-alias\n-p\n2222\n-o\nStrictHostKeyChecking=yes\n'

# user@host targets are passed directly to OpenSSH without interpretation.
run_launcher ssh admin@example.internal
assert_args $'--new-window\n--\nssh\nadmin@example.internal\n'

# Tab mode reuses Ptyxis tab routing while still executing standard OpenSSH.
run_launcher ssh-tab goreecloud-vps-netbird
assert_args $'--tab\n--\nssh\ngoreecloud-vps-netbird\n'

# SSH subcommand help is local and must not start the runtime.
rm -f "$capture"
run_launcher ssh --help >"${tmp_dir}/help.txt"
[[ ! -e "$capture" ]]
grep -Fq 'goreecloud-terminal ssh TARGET' "${tmp_dir}/help.txt"
grep -Fq 'does not store SSH passwords' "${tmp_dir}/help.txt"

# A missing target is a usage error and must not start Ptyxis.
rm -f "$capture"
if run_launcher ssh >"${tmp_dir}/missing.out" 2>"${tmp_dir}/missing.err"; then
  printf 'expected missing SSH target to fail\n' >&2
  exit 1
fi
[[ ! -e "$capture" ]]
grep -Fq 'Usage:' "${tmp_dir}/missing.err"

printf 'SSH launch workflow tests passed\n'
