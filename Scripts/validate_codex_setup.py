#!/usr/bin/env python3
"""Deterministic, read-only static audit of AIHEROES Codex + pinned SoT30."""
import hashlib
import pathlib
import subprocess
import sys
import tomllib

from verify_sot30 import verify, INSTRUCTIONS_SHA

ROOT = pathlib.Path(__file__).resolve().parents[1]
failures = []
checks = []


def assert_check(label: str, ok: bool) -> None:
    checks.append(label)
    print(("PASS " if ok else "FAIL ") + label)
    if not ok:
        failures.append(label)


def sha(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    canon = ROOT / ".iskra" / "sot30"
    assert_check("SoT30 integrity", canon.is_dir() and verify(canon, False))
    instructions = canon / "support" / "PROJECT_INSTRUCTIONS_SOT30.md"
    assert_check("pinned model instructions", instructions.exists()
                 and sha(instructions) == INSTRUCTIONS_SHA)

    agents = ROOT / "AGENTS.md"
    text = agents.read_text(encoding="utf-8") if agents.is_file() else ""
    assert_check("AGENTS.md present and scoped", "AIHEROES" in text
                 and "SOT30_UNAVAILABLE" in text and "ΔDΩΛ" in text)
    for value in ("PROCEED", "FORCE_ISKRIV_1", "FORCE_SHADOW",
                  "FORCE_CRISIS", "CLOSE_HONESTLY"):
        assert_check("AGENTS Guard " + value, value in text)

    skill_path = ROOT / ".agents" / "skills" / "iskra-sot30" / "SKILL.md"
    skill_text = skill_path.read_text(encoding="utf-8") if skill_path.exists() else ""
    assert_check("Codex local skill", skill_text.startswith("---\nname: iskra-sot30\n")
                 and "description:" in skill_text)
    assert_check("Skill companion reference", (skill_path.parent
                 / "references" / "aiheroes-workflow.md").is_file())

    config_path = ROOT / ".codex" / "config.toml"
    cfg = tomllib.loads(config_path.read_text(encoding="utf-8")) if config_path.exists() else {}
    assert_check("supported project model override", cfg.get("model") == "gpt-6-sol")
    assert_check("project workspace sandbox", cfg.get("sandbox_mode") == "workspace-write")
    assert_check("project approval prompt", cfg.get("approval_policy") == "on-request")
    assert_check("project network disabled", cfg.get("sandbox_workspace_write", {})
                 .get("network_access") is False)
    raw_model_instructions = cfg.get("model_instructions_file", "")
    resolved_instructions = (config_path.parent / raw_model_instructions).resolve()
    assert_check("Codex model instructions resolve to verified canon",
                 resolved_instructions == instructions.resolve())

    user_config = pathlib.Path.home() / ".codex" / "config.toml"
    home_cfg = tomllib.loads(user_config.read_text(encoding="utf-8-sig"))
    path = str(ROOT).lower()
    trust = {key.lower(): value for key, value in home_cfg.get("projects", {}).items()}
    assert_check("AIHEROES-specific trust (required for project config)",
                 trust.get(path, {}).get("trust_level") == "trusted")

    if sys.platform == "win32":
        acl = subprocess.run(["icacls", str(canon)], capture_output=True)
        file_acl = subprocess.run(
            ["icacls", str(canon / "support" / "MANIFEST.json")],
            capture_output=True)
        assert_check("Windows sandbox root has inherited RX for Codex",
                     acl.returncode == 0 and
                     b"CodexSandboxUsers:(OI)(CI)(RX)" in acl.stdout)
        assert_check("Windows sandbox can read pinned manifest",
                     file_acl.returncode == 0 and
                     b"CodexSandboxUsers:(I)(RX)" in file_acl.stdout)
        assert_check("Windows ACL rollback snapshot preserved",
                     (ROOT / ".iskra" / "acl_backup" /
                      "sot30-acl-pre-codex-read-20261028.txt").is_file())

    try:
        git_ignore = subprocess.run(
            ["git", "-C", str(ROOT), "check-ignore", "-q", "--",
             ".iskra/sot30/knowledge/00_PROJECT_ROUTER.md"],
            timeout=15, check=False).returncode == 0
    except (OSError, subprocess.TimeoutExpired):
        git_ignore = False
    assert_check("private canon ignored by Git", git_ignore)

    print("CODEX_SOT30_STATIC_" + ("FAIL" if failures else "PASS")
          + f" checks={len(checks)-len(failures)}/{len(checks)}")
    return int(bool(failures))


if __name__ == "__main__":
    sys.exit(main())
