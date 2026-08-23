#!/usr/bin/env bash
set -euo pipefail

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
launcher="$repo_root/data/goreecloud-terminal"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT INT TERM

fake_ptyxis="$tmp/ptyxis"
invocations="$tmp/ptyxis-invocations"
cat > "$fake_ptyxis" <<'EOF'
#!/usr/bin/env sh
printf '%s\n' "$*" >> "${GORECLOUD_TEST_PTYXIS_INVOCATIONS:?}"
exit 77
EOF
chmod +x "$fake_ptyxis"

run_launcher() {
  GORECLOUD_TERMINAL_PTYXIS_BIN="$fake_ptyxis" \
  GORECLOUD_TEST_PTYXIS_INVOCATIONS="$invocations" \
    "$launcher" "$@"
}

status_json="$(run_launcher api status)"
STATUS_JSON="$status_json" python3 - <<'PY'
import json, os
obj = json.loads(os.environ["STATUS_JSON"])
assert obj["schema_version"] == 1
assert obj["product"] == "GoreeCloud Terminal"
assert obj["repository"] == "GoreeCloud/goreecloud-terminal"
assert obj["release_lifecycle"] == "Release Candidate"
assert obj["release_candidate"] == "50.2-rc.2"
assert obj["upstream_foundation"] == "Ptyxis 50.2"
assert obj["identity"]["production_application_id"] == "com.goreecloud.Terminal"
assert obj["identity"]["development_application_id"] == "com.goreecloud.Terminal.Devel"
assert obj["identity"]["canonical_launcher"] == "goreecloud-terminal"
assert obj["api"] == {"transport": "local-cli", "read_only": True}
assert obj["privacy"] == {
    "terminal_content_included": False,
    "commands_included": False,
    "credentials_included": False,
    "host_aliases_included": False,
    "identifiers_included": False,
}
assert obj["privacy_shield_capabilities"] == ["telemetry-minimization", "data-minimization"]
assert obj["wardveil"] == {"presentation_available": True, "authorization_authority": False}
PY

test ! -e "$invocations"
run_launcher api --help | grep -Fq 'goreecloud-terminal api status'
test ! -e "$invocations"

if run_launcher api mutate >/dev/null 2>&1; then
  echo 'Unknown API mutation unexpectedly succeeded' >&2
  exit 1
fi
test ! -e "$invocations"

echo 'GoreeCloud Terminal local API contract validated'
