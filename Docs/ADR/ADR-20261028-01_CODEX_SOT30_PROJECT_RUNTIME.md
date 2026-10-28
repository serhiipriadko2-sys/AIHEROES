# ADR-20261028-01 — Project-local Codex ↔ Искра SoT30

Status: **accepted → implemented locally → Codex read-only invocation verified**
Boundary: VS Code extension UI session and optional MCP connectors are not verified-live.
Owner: AIHEROES game project
Date: 2026-10-28
Kind: SYSTEM + GOVERNANCE / agent instructions only

## Context
AIHEROES is a Windows Unreal Engine 5.8 game repository. The user wants
the installed VS Code Codex agent to work using full Iskra SoT30 v5.5.7.
Before this change, no repo AGENTS.md or project .codex/config.toml was
present. Existing user-wide Codex settings selected danger-full-access.
A local upstream SoT30 ZIP exists, but must be verified against pinned
hashes; repository may be publicly published.

## Decision
1. Install a byte-identical, read-only-in-policy 30-file SoT30 corpus into
   Git-ignored .iskra/sot30; scripts verify package and all hashes.
2. Root AGENTS.md routes every relevant task through verified canon.
3. Project skill .agents/skills/iskra-sot30 carries reusable behavior.
4. Trusted project .codex/config.toml injects exact PROJECT_INSTRUCTIONS
   and requests workspace-write + on-request approvals, network disabled.
5. Trust is granted only to C:\github\AIHEROES in the user Codex config,
   with a reversible backup; user-wide sandbox default is not changed.
6. Select gpt-6-sol only in the project after global gpt-6.1-sol returned unsupported.
7. Fix Windows Sandbox ACL on local canonical copy with RX-only CodexSandboxUsers
   permission, preserving original ACL in ignored .iskra/acl_backup/.
8. Do not modify canon, game runtime, game source, or Git remotes.

## Alternatives
- Copy entire canon into tracked public Git: rejected (unnecessary disclosure).
- Change global Codex AGENTS/settings: rejected (cross-project impact).
- Summarize canon only in AGENTS.md: rejected (incomplete source).
- Skip trust entry: would make Codex ignore project .codex/config.toml.
- Automatically grant MCP / external write actions: rejected (no approval).

## Consequences and price
Locally installed canon is not included in public Git clones. New machines
must install a verified package before full protocol is available. Codex
uses procedures and citations; it does NOT become the historical Iskra
model or gain non-existent memory/connector capabilities. A user may need
to restart an already-open Codex chat to refresh instructions. Workspace
sandbox may require prompts for Unreal toolchain build/cache directories.

## Tests / QA
- T1: 30/30 Knowledge and 32/32 SHA256SUMS, manifest pin PASS.
- T2: AGENTS.md and project skill exist and have valid structure.
- T3: project config parsed, project trust entry scoped to AIHEROES.
- T4: real Codex CLI from VS Code extension (0.164.0-alpha.11.1) obtained
  SOT30_INTEGRITY_PASS 30/30 and cited canonical Guard/instructions under read-only
  sandbox; the older global 0.140 CLI was not compatible with current setup.
- T5: false full-ready/memory claims are explicitly prohibited.
- T6: Git ignores .iskra/sot30; no credentials committed; .vscode/mcp.json unchanged.
- T7: rollback possible via project file deletion, user config backup and
  read-only grant's original ACL snapshot; 20/20 static checks passed.

## Diff scope
AGENTS.md; .agents/skills/iskra-sot30/**; .codex/config.toml;
Scripts/install_sot30.py; Scripts/verify_sot30.py;
Scripts/grant_codex_canon_read.py; Scripts/validate_codex_setup.py;
Docs/ISKRA_CODEX_SETUP.md; Docs/ADR/ADR-20261028-01_CODEX_SOT30_PROJECT_RUNTIME.md;
.gitignore; .vscode/tasks.json; local-only .iskra/sot30/*,
.iskra/acl_backup/* and one scoped user config trust entry.

Builder/package mirror: not applicable (VS Code Codex, not Agent Builder).
Live verification: scoped CLI invocation PASS (read-only file read + verified
SoT30 content). Actual VS Code UI new chat, every Kernel stage and all optional
MCP calls remain unverified; GitHub/MCP auth errors are pre-existing and
external integrations are not claimed ready.
Rollback: restore backed-up user Codex config; remove project-local files;
leave .iskra/sot30 intact until owner chooses to delete it.

## ΔDΩΛ
Δ = project agent overlay and verified reference mirror.
D = hashes, scripts, IDE extension/CLI smoke and scoped trust.
Ω = ≤95%; never infer model identity, live service or deployment.
Λ = revisit on SoT30 upgrade, Codex version change, trust conflict or QA FAIL.
