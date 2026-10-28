# AIHEROES / Codex / Искра SoT30 — verified local installation receipt

Date: 2026-10-28
Project: C:\github\AIHEROES
Scope: Windows / VS Code Codex extension; user asked to bind full SoT30 v5.5.7.
Status: LOCAL_CODEX_READ_PASS, STATIC_20_20_PASS; FULL_LIVE_RUNTIME_NOT_CLAIMED.

## Source and integrity
Source archive: C:\github\iskra-1\dist\SoT30_v5.5.7.zip
Archive bytes: 1,135,594
Archive SHA256: 193879cadf3b61f61dbbb8b4ca9c360a98c09c5006cc8d08e7f4ecd98508779d
Installed source: C:\github\AIHEROES\.iskra\sot30\
Manifest SHA256: 0decd118a6300324b757d2587ec315a569aaf5800299d2b33bf046feaf56aeff
Knowledge: 30/30, 4,054,425 bytes; SHA256SUMS 32/32; total 33 ZIP members.
Both source ZIP and installed directory passed Scripts/verify_sot30.py.
Original content is Git-ignored; no pinned canon bytes were changed.

## Project files: bytes, SHA256 (observed read-back)
- AGENTS.md | 6553 | 8f208c72d34dd332a184aee164b12a3d75b127164b1377bb15470a5781fea3ce
- .codex/config.toml | 416 | 7a6c38ac3efa79183f781b37327725192cc3981dbe1e7c61b06b83d19f409d85
- .agents/skills/iskra-sot30/SKILL.md | 4275 | bbae69ae8504e9919b5adfe863148d4d6650d6a36940df8231e5cc24c12d36a1
- .agents/skills/iskra-sot30/agents/openai.yaml | 179 | 7958938c173d9e1df1fe45bab7d44bd82cefd3eedbd1f00bea1460fdabaf9b9c
- .agents/skills/iskra-sot30/references/aiheroes-workflow.md | 1238 | ba4519bf0b6560907604cb9305d1e03d0d1b7c688bf4f844e1c2dc81b0c20ac6
- Scripts/verify_sot30.py | 5072 | 6d045e18d9272637c3af666121dfa094558f2468c85fbab5ae3f884f3a7ca1f1
- Scripts/install_sot30.py | 2038 | a076de6a01c3192f0278c52ca7c4285e1da6d3e51a49d9faa17f180e3f717739
- Scripts/grant_codex_canon_read.py | 2829 | ed898799bfabe286a23591db4ac13b2499822e5f03b7b1b53905f60bf48a9bf5
- Scripts/validate_codex_setup.py | 4384 | 3ab86108d7acedc432d4c7750b02f136daff4d13d87a63c5dfecef1ad2ceef35
- Docs/ISKRA_CODEX_SETUP.md | 7042 | faad26b762a732a5064855d7cc7d8648e2ab8c8fe5d9223c683d875108e511e5
- Docs/ADR/ADR-20261028-01_CODEX_SOT30_PROJECT_RUNTIME.md | 4410 | 4999a0139cbbcca539ba406f3c82a6c4c6ca9a36515d43967f3b59683734fc09
- .gitignore | 366 | f190140ee00913158dd8e471b366a3b110f0899ae294c001c87b6e79a378ccbd
- .vscode/tasks.json | 1899 | 562ed343c505ea8b8be0d98aac32a64952bb4c2e4e18bd07519a0d184a892686

## Local security and rollback
Global Codex default previously used sandbox_mode=danger-full-access. It
was not changed. Only the trusted-project entry for C:\github\AIHEROES was
appended to the user config; .codex/config.toml overrides to workspace-write,
approval_policy=on-request, network_access=false and project model gpt-6-sol.
Codex model_instructions_file points into verified SoT30 support instructions.
The global gpt-6.1-sol was rejected by the current ChatGPT Codex client;
gpt-6-sol was actually used in the successful extension CLI smoke.

User config rollback file:
C:\Users\gabra\.codex\config.toml.pre-aiheroes-sot30-20261028.bak
Bytes: 13040
SHA256: f645f0e45231bb52489717316455e1b71a084004c3eddca7f8cfd9496309160b

Windows ACL anomaly: extracted SoT30 was not readable from Codex sandbox
because the atomic directory rename preserved a protected DACL. Scoped
read+execute only permission was granted to CodexSandboxUsers on SoT30
and propagated to child files. No global Windows ACLs were altered.
ACL rollback file (Git-ignored):
C:\github\AIHEROES\.iskra\acl_backup\sot30-acl-pre-codex-read-20261028.txt
Bytes: 6342
SHA256: f5d9c4756c235141e9e495c51c5ea844c176a83be9a25751a126547c021eddee

## Independent observed verification
1. Windows CLI outside Codex verified the original ZIP then the installed corpus.
2. Scripts/validate_codex_setup.py completed CODEX_SOT30_STATIC_PASS
   checks=20/20, exit code 0.
3. Actual executable embedded in the VS Code openai.chatgpt extension,
   codex-cli 0.164.0-alpha.11.1, invoked with --sandbox read-only
   and --ephemeral, ran the canonical SHA verifier inside its sandbox:
   SOT30_INTEGRITY_PASS knowledge=30/30 sha256sums=32/32 exit code 0.
   It read the original PROJECT_INSTRUCTIONS_SOT30.md and
   29_INDEX_UPLOAD_MANIFEST.md, cited actual sections and correctly
   extracted all five Guard decisions. Process exited 0.
4. A second independent Codex read-only invocation explicitly used
   .agents/skills/iskra-sot30/SKILL.md and references/aiheroes-workflow.md,
   ran validation: CODEX_SOT30_STATIC_PASS checks=20/20, exit code 0,
   and reported an accurate Unreal AI/game authority boundary.
5. VS Code tasks.json parses; three tasks are present (build, battle
   tests and Codex SoT verification).
6. git check-ignore matched the local SoT and the ACL backup.

## Remaining non-claims
This verifies the runtime *instructions, retrieval and a read-only test*.
It does not prove external Supabase/memory writes, full Iskra SoT30 services,
model checkpoint evolution in game, all Guard transitions, deployment, or a
fresh VS Code UI chat having reloaded every instruction. Some user-wide
MCP servers report AuthRequired and global plugin hooks report warnings;
the actual Codex model request succeeded over HTTPS after WebSocket 403.
Codex skill catalog exceeded context budget, but explicit project-skill
file loading was confirmed. Global CLI 0.140.0 had earlier startup/memory
issues; the extension's newer binary was used for acceptance.

No Git commit or push was performed by this configuration task. Parallel
work in the same AIHEROES repository was observed; modified game source
files were not overwritten by this task.

Verification boundary: LOCAL_INTEGRATION_PASS / FULL_RUNTIME_PARTIAL.
ΔDΩΛ: Δ = verified Codex SoT30 project overlay; D = source hashes,
config, ACL receipt, independent Codex reads; Ω = 95% for bounded
acceptance; Λ = retest after Codex/SoT upgrade, permission changes,
new VS Code chat, or live connector activation.
