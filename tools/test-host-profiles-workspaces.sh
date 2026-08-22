#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
launcher="${repo_root}/data/goreecloud-terminal"

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

capture="${tmp_dir}/argv.txt"
profiles_file="${tmp_dir}/profiles.tsv"

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

cat >"$profiles_file" <<'EOF'
# Workspace<TAB>Profile<TAB>OpenSSH Host alias
Infrastructure	primary-vps	vps-admin
Virtualization	primary-hypervisor	hypervisor-admin
Storage	primary-storage	storage-admin
Infrastructure	secondary-vps	vps-secondary
EOF

run_launcher() {
  GORECLOUD_TERMINAL_PTYXIS_BIN="${tmp_dir}/ptyxis" \
  GORECLOUD_TERMINAL_TEST_CAPTURE="$capture" \
  GORECLOUD_TERMINAL_PROFILES_FILE="$profiles_file" \
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

# Workspaces are listed once in first-seen order and do not start the runtime.
rm -f "$capture"
run_launcher workspaces >"${tmp_dir}/workspaces.txt"
[[ ! -e "$capture" ]]
printf 'Infrastructure\nVirtualization\nStorage\n' >"${tmp_dir}/expected-workspaces.txt"
cmp -s "${tmp_dir}/expected-workspaces.txt" "${tmp_dir}/workspaces.txt"

# Profiles are non-secret metadata and can be filtered by workspace.
run_launcher profiles >"${tmp_dir}/profiles.txt"
printf 'primary-vps\tInfrastructure\tvps-admin\nprimary-hypervisor\tVirtualization\thypervisor-admin\nprimary-storage\tStorage\tstorage-admin\nsecondary-vps\tInfrastructure\tvps-secondary\n' >"${tmp_dir}/expected-profiles.txt"
cmp -s "${tmp_dir}/expected-profiles.txt" "${tmp_dir}/profiles.txt"

run_launcher profiles Infrastructure >"${tmp_dir}/infrastructure.txt"
printf 'primary-vps\tInfrastructure\tvps-admin\nsecondary-vps\tInfrastructure\tvps-secondary\n' >"${tmp_dir}/expected-infrastructure.txt"
cmp -s "${tmp_dir}/expected-infrastructure.txt" "${tmp_dir}/infrastructure.txt"

# A profile opens standard OpenSSH using only the stored Host alias.
run_launcher profile primary-vps
assert_args $'--new-window\n--\nssh\nvps-admin\n'

# Tab mode preserves the same authority boundary.
run_launcher profile-tab primary-hypervisor
assert_args $'--tab\n--\nssh\nhypervisor-admin\n'

# Remaining arguments are remote-command arguments after the resolved alias.
run_launcher profile primary-storage uname -a
assert_args $'--new-window\n--\nssh\nstorage-admin\nuname\n-a\n'

# Unknown workspaces and profiles fail before the terminal runtime starts.
rm -f "$capture"
if run_launcher profiles Missing >"${tmp_dir}/missing-workspace.out" 2>"${tmp_dir}/missing-workspace.err"; then
  printf 'expected unknown workspace to fail\n' >&2
  exit 1
fi
[[ ! -e "$capture" ]]
grep -Fq 'unknown workspace: Missing' "${tmp_dir}/missing-workspace.err"

if run_launcher profile missing-profile >"${tmp_dir}/missing-profile.out" 2>"${tmp_dir}/missing-profile.err"; then
  printf 'expected unknown profile to fail\n' >&2
  exit 1
fi
[[ ! -e "$capture" ]]
grep -Fq 'unknown profile: missing-profile' "${tmp_dir}/missing-profile.err"

# Duplicate profile IDs fail closed rather than selecting an ambiguous destination.
cat >"$profiles_file" <<'EOF'
Infrastructure	duplicate	first-alias
Storage	duplicate	second-alias
EOF
if run_launcher profile duplicate >"${tmp_dir}/duplicate.out" 2>"${tmp_dir}/duplicate.err"; then
  printf 'expected duplicate profile IDs to fail\n' >&2
  exit 1
fi
[[ ! -e "$capture" ]]
grep -Fq 'duplicate profile ID' "${tmp_dir}/duplicate.err"

# Aliases that could be interpreted as OpenSSH options are rejected.
cat >"$profiles_file" <<'EOF'
Infrastructure	unsafe	-oProxyCommand=unexpected
EOF
if run_launcher profile unsafe >"${tmp_dir}/unsafe.out" 2>"${tmp_dir}/unsafe.err"; then
  printf 'expected unsafe SSH alias to fail\n' >&2
  exit 1
fi
[[ ! -e "$capture" ]]
grep -Fq 'SSH Host alias must be one non-option token' "${tmp_dir}/unsafe.err"

# Missing profile configuration fails before the runtime starts.
rm -f "$profiles_file" "$capture"
if run_launcher workspaces >"${tmp_dir}/missing-config.out" 2>"${tmp_dir}/missing-config.err"; then
  printf 'expected missing profile configuration to fail\n' >&2
  exit 1
fi
[[ ! -e "$capture" ]]
grep -Fq 'profile configuration not found' "${tmp_dir}/missing-config.err"

printf 'Host profile and workspace tests passed\n'
