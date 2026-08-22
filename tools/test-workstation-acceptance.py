#!/usr/bin/env python3
from __future__ import annotations

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
COLLECTOR = ROOT / "tools/collect-workstation-acceptance.py"
TEMPLATE = ROOT / "release/workstation-acceptance-manual-status.example.json"
EXPECTED_COMMIT = "b381917f3ba700c3b1b903423f8241fa80b640bdebdfebb438f47212559f8af7"
EXPECTED_BUNDLE = "goreecloud-terminal-50.2-rc.1-ae5d10511a2aed4a504b189165e268a1e9c1040f.flatpak"


def write_fake_flatpak(path: Path) -> None:
    script = r'''#!/usr/bin/env python3
import json
import os
import sys

args = sys.argv[1:]
commit = os.environ.get("FAKE_FLATPAK_COMMIT", "b381917f3ba700c3b1b903423f8241fa80b640bdebdfebb438f47212559f8af7")

if len(args) >= 2 and args[0] in ("--user", "--system") and args[1] == "info":
    scope = args[0][2:]
    if scope == "system" and os.environ.get("FAKE_BOTH_SCOPES") != "1":
        raise SystemExit(1)
    if len(args) == 3:
        print("GoreeCloud Terminal")
        raise SystemExit(0)
    option = args[2]
    if option == "--show-runtime":
        print("org.gnome.Platform/x86_64/50")
    elif option == "--show-commit":
        print(commit)
    elif option == "--show-ref":
        print("app/com.goreecloud.Terminal/x86_64/master")
    elif option == "--show-permissions":
        print("[Context]")
        print("shared=network;ipc;")
        print("sockets=x11;wayland;")
        print("devices=dri;")
        print("features=devel;")
        print("filesystems=host;")
        print("[Session Bus Policy]")
        print("org.freedesktop.Flatpak=talk")
    else:
        raise SystemExit(2)
    raise SystemExit(0)

if args and args[0] == "run":
    if "--version" in args:
        print("GoreeCloud Terminal 50.2")
        raise SystemExit(0)
    if len(args) >= 2 and args[-2:] == ["api", "status"]:
        bad = os.environ.get("FAKE_BAD_API") == "1"
        obj = {
            "release_lifecycle": "Release Candidate",
            "release_candidate": "50.2-rc.1",
            "identity": {"production_application_id": "com.goreecloud.Terminal"},
            "api": {"read_only": True},
            "privacy": {
                "terminal_content_included": bad,
                "credentials_included": False,
            },
        }
        print(json.dumps(obj))
        raise SystemExit(0)

raise SystemExit(2)
'''
    path.write_text(script, encoding="utf-8")
    path.chmod(0o755)


def all_pass_manual(path: Path) -> None:
    obj = json.loads(TEMPLATE.read_text(encoding="utf-8"))
    obj["checks"] = {key: "pass" for key in obj["checks"]}
    path.write_text(json.dumps(obj), encoding="utf-8")


def invoke(work: Path, *args: str, env_extra: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    env = os.environ.copy()
    env["PATH"] = f"{work}:{env.get('PATH', '')}"
    env["XDG_SESSION_TYPE"] = "wayland"
    if env_extra:
        env.update(env_extra)
    return subprocess.run(
        [sys.executable, str(COLLECTOR), *args],
        text=True,
        capture_output=True,
        env=env,
        check=False,
    )


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="goreecloud-terminal-workstation-test-") as tmp:
        work = Path(tmp)
        write_fake_flatpak(work / "flatpak")
        manual = work / "manual.json"
        all_pass_manual(manual)

        evidence_path = work / "evidence.json"
        result = invoke(work, "--manual-status", str(manual), "--output", str(evidence_path))
        assert result.returncode == 0, result.stderr
        evidence = json.loads(evidence_path.read_text(encoding="utf-8"))
        assert evidence["manual_checks_complete"] is True
        assert evidence["published_bundle"]["verified"] is False
        assert evidence["workstation_acceptance_complete"] is False
        assert evidence["production_approved"] is False
        assert evidence["stable_approved"] is False
        assert evidence["machine_checks"]["installed_ostree_commit"] == EXPECTED_COMMIT
        assert evidence["machine_checks"]["local_api"]["terminal_content_included"] is False
        assert evidence["privacy"]["terminal_content_collected"] is False
        assert evidence["privacy"]["credentials_collected"] is False

        result = invoke(work, "--manual-status", str(manual), "--require-complete")
        assert result.returncode == 2, result.stderr

        bad_manual = work / "bad-manual.json"
        obj = json.loads(manual.read_text(encoding="utf-8"))
        obj["notes"] = "free-form notes are forbidden"
        bad_manual.write_text(json.dumps(obj), encoding="utf-8")
        result = invoke(work, "--manual-status", str(bad_manual))
        assert result.returncode == 1

        bad_bundle = work / EXPECTED_BUNDLE
        bad_bundle.write_bytes(b"not-the-published-flatpak")
        result = invoke(work, "--bundle", str(bad_bundle), "--manual-status", str(manual))
        assert result.returncode == 1

        result = invoke(
            work,
            "--manual-status",
            str(manual),
            env_extra={"FAKE_FLATPAK_COMMIT": "0" * 64},
        )
        assert result.returncode == 1

        result = invoke(
            work,
            "--manual-status",
            str(manual),
            env_extra={"FAKE_BAD_API": "1"},
        )
        assert result.returncode == 1

        result = invoke(
            work,
            "--manual-status",
            str(manual),
            env_extra={"FAKE_BOTH_SCOPES": "1"},
        )
        assert result.returncode == 1

    print("GoreeCloud Terminal workstation acceptance contract tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
