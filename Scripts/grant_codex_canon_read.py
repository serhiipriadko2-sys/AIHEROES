#!/usr/bin/env python3
"""Grant *read-only* access to the verified local SoT30 for CodexSandboxUsers.

Windows-only, project-bounded and idempotent. Preserves an ACL rollback file
before changing permissions; does not change global Codex security settings.
"""
import hashlib
import os
import pathlib
import subprocess
import sys

from verify_sot30 import verify


ROOT = pathlib.Path(__file__).resolve().parents[1]
CANON = ROOT / ".iskra" / "sot30"
MANIFEST = CANON / "support" / "MANIFEST.json"
BACKUP = ROOT / ".iskra" / "acl_backup" / "sot30-acl-pre-codex-read-20261028.txt"


def shell_icacls(*args: str) -> tuple[int, str]:
    result = subprocess.run(
        ["icacls", *map(str, args)], capture_output=True, check=False)
    msg = (result.stdout + b"\n" + result.stderr).decode(errors="replace")
    return result.returncode, msg


def main() -> int:
    if os.name != "nt":
        print("NOT_APPLICABLE: native Windows Codex sandbox only")
        return 2
    if (not CANON.is_dir() or CANON.is_symlink()
            or getattr(CANON, "is_junction", lambda: False)()):
        print("BLOCKED: canonical directory missing or reparse point")
        return 1
    if not verify(CANON, False):
        print("BLOCKED: canonical SHA-256 mismatch")
        return 1

    computer = os.environ.get("COMPUTERNAME")
    if not computer:
        print("BLOCKED: cannot determine local group owner")
        return 1
    group = computer + r"\CodexSandboxUsers"
    rc, file_acl = shell_icacls(str(MANIFEST))
    if rc != 0:
        print("BLOCKED: cannot inspect existing manifest ACL")
        return 1
    if "CodexSandboxUsers:(I)(RX)" in file_acl:
        print("CODEX_CANON_READ_PASS (already granted, no ACL writes)")
        return 0

    BACKUP.parent.mkdir(parents=True, exist_ok=True)
    if not BACKUP.exists():
        rc, message = shell_icacls(str(CANON), "/save", str(BACKUP), "/T")
        if rc != 0 or not BACKUP.is_file() or BACKUP.stat().st_size == 0:
            print("BLOCKED: cannot save original ACL", message[-500:])
            return 1

    rc, message = shell_icacls(str(CANON), "/grant",
                                group + ":(OI)(CI)RX")
    if rc != 0:
        print("BLOCKED: read-only sandbox ACL grant failed", message[-500:])
        return 1
    rc, file_acl = shell_icacls(str(MANIFEST))
    if rc or "CodexSandboxUsers:(I)(RX)" not in file_acl:
        print("BLOCKED: read-back failed; rollback from", BACKUP)
        return 1
    if not verify(CANON, False):
        print("BLOCKED: canon changed after ACL operation!")
        return 1

    print("CODEX_CANON_READ_PASS (RX only, scoped to .iskra/sot30)")
    print("ACL_BACKUP", BACKUP)
    print("ACL_BACKUP_SHA256", hashlib.sha256(BACKUP.read_bytes()).hexdigest())
    return 0


if __name__ == "__main__":
    sys.exit(main())
