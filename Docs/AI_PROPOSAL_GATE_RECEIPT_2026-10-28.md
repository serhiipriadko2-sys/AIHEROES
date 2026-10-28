# AI HEROES — AI Proposal Gate v1: integration receipt

Date: 2026-10-28
Project: C:\github\AIHEROES (Windows DESKTOP-ITUAMNF)
Frozen claim boundary: LOCAL WINDOWS BUILD + AUTOMATED TESTS PASS,
PLUS SCRIPTED EXACT-WINDOW GAMEPLAY QA PASS (SEED and ANALYST).
Status labels: SOURCE WRITTEN / BUILT / TESTED / VERIFIED-WINDOW-QA.
No claim of manual human-play QA or Android readiness.
No GitHub commit/push/merge or external deployment by this operator.

## Source-of-truth gate

At start of work, command:
  python Scripts/verify_sot30.py --directory .iskra/sot30

Read-back result:
  SOT30_INTEGRITY_PASS version=v5.5.7
  knowledge=30/30 sha256sums=32/32 corpus_bytes=4054425 members=33/33
  pinned_manifest_sha256=0decd118a6300324b757d2587ec315a569aaf5800299d2b33bf046feaf56aeff

Repository HEAD at start: d6ee2059ffc2daa50b9882754c2c854d0dedf387.
Concurrent modules for base upgrades, campaign objectives and Grid V2 replay
changed in the same worktree. These were not deleted, overwritten, committed
or attributed to this operator. Original GameMode.h, advisor dossier and
README snapshots are stored and hash-checked in
Saved\IskraBackups\AIProposalGate_20261028_2300.

## Delivered behavior

The new FAIHeroesAIProposalGate accepts a filtered, engine-owned immutable
game snapshot and a typed candidate. Types are:
ADVICE | PROPOSE_ORDER | NO_ACTION;
NONE | MOVE_TO_NODE | ATTACK_NODE;
OFFLINE_RULES | LOCAL_MODEL | REMOTE_MODEL.

Checks include stale day/revision, node visibility/identity, owned/hostile
guards, living army, travel budget using the same TurnRules as GameMode,
protocol lock, game/battle lock, snapshot bounds, text controls/length,
and claims of model readiness. Offline rules cannot impersonate a model.
Remote inference needs separately confirmed connection AND opt-in.
Actual PC/Android weights or HTTP adapters are NOT connected.

An accepted order remains a proposal ONLY. There is NO execution API in the
gate; it never captures a node, changes army HP, deducts resources or moves
the player. All suggested orders explicitly require human confirmation.

Integrated into the existing read-only strategic adviser dossier only.
The recruitment and campaign-objective reports are not modified by this
integration. No saves or on-disk memory are written by advice. The in-game
report says whether the proposed order passed checks or why it was rejected.

Design and limits: Docs\AI_PROPOSAL_GATE_V1.md.
The revision CRC32 is a per-session staleness check, not a cryptographic
authentication token. It must not be described as security authorization.

## Automated verification

The toolchain compiled AIHeroesEditor Win64 Development with the new code;
UnrealBuildTool reported Result: Succeeded.
Compiled DLL was last modified at 23:07:43 Windows local time.

Fresh full report:
 Docs\AutomationRuns\20261028T200750062Z\index.json
 RESULTS: 77 / 77 SUCCESS, failed=0, notRun=0, warnings=0.

New automated test cases, all SUCCESS:
 AIHeroes.Proposal.OfflinePreview
 AIHeroes.Proposal.AuthorityAndStaleness
 AIHeroes.Proposal.ModelAndConsentBoundaries
 AIHeroes.Proposal.InputAndSnapshotBounds
 AIHeroes.Proposal.DeterministicStaleFuzz

The fuzz test uses exactly 10000 fixed-seed malformed proposals and asserts
they are ALL rejected. It also verifies that original valid proposals
remain valid and the game-state fixture never mutates.

One previous full run (76/77) failed the separately authored
GridV2RejectTamperAndLegacy test because its purported V1 fixture used
two enemy stacks while V1 permits one. A parallel developer independently
fixed that fixture; the full post-fix 77/77 report is the current evidence.
Do not use the older failed report as a passing gate.

The PowerShell QA script Tools\qa_ai_proposal_gate.ps1 parsed successfully
(656 tokens) and was executed against its OWN independently created
1280x720 game window (exact verified process PID 5908) while leaving other
QA sessions untouched. Exit code 0: AI_PROPOSAL_WINDOW_QA_PASS.
Game log: Saved\Logs\AIHeroesProposalQA.log.
Evidence: selected FOREST; starting SEED proposed only ADVICE with
requires_player=0; dossier opened fresh and consumed a map click;
player explicitly unlocked THREAT ANALYSIS, spending 80 GOLD, 20 WOOD
and 15 ORE via ordinary UI; ANALYST then produced an accepted typed
ATTACK FOREST proposal with requires_player=1; no battle, save or
inference call was executed. Dossier closed, map remained playable.
The first QA attempt expected ANALYST at the starting SEED tier and
failed correctly; the revised scenario checks both stages.
All screenshots were produced via PrintWindow of the exact game window,
not by reading or capturing the user's desktop. Both strategic dossier
screenshots were manually inspected for visible text and readability.
The assistant's own QA process was closed gracefully after capture;
other QA game/editor windows were left untouched.
This is scripted real-window QA, NOT a manual hardware input test.

## Artifact byte/hash manifest

| Project-relative file | Bytes | SHA256 |
|---|---:|---|
| Source/AIHeroes/Public/AIHeroesAIProposalGate.h | 2243 | 70e4990b086daa909d457df9e6e7bd4cc958f780602ad54a6f014ebcf041d432 |
| Source/AIHeroes/Private/AIHeroesAIProposalGate.cpp | 9973 | 6220b874ba87b5fa87d41e890d173487ee80dd080dc379c48db67c5e7067f159 |
| Source/AIHeroes/Private/AIHeroesAIProposalGameMode.cpp | 1550 | 67bfd4c87513befce06609e7ac9c7a85cb9cbeaa4795e6c0fecc87666014cba5 |
| Source/AIHeroes/Private/Tests/AIHeroesAIProposalTests.cpp | 12497 | fbe34eecd9d3cab0a4b2eb411666bf330b9d69def178389ed77f278f2275e233 |
| Source/AIHeroes/Private/AIHeroesAdvisorDossierGameMode.cpp | 3488 | e7054fb84432d5f18fa7799ba8ac00fc5953a02f0460ec879505a20322c9b453 |
| Source/AIHeroes/Public/AIHeroesGameMode.h | 7391 | 123eae01bb48488bb8cf17bc82babad60d1f56bc0ea09a73f85f66dfe60b9ae1 |
| Tools/qa_ai_proposal_gate.ps1 | 5104 | 0893a7bedbc5a1387335e37d3cc84fce686f9f46c121e081590f8c41312e209a |
| Docs/AI_PROPOSAL_GATE_V1.md | 4574 | 00da66fe58a3bf6b490a55d7271e394cb40dd7f4d43f43b9cb4b0e43bb9c02de |
| Docs/AIHEROES_PROPOSAL_SEED_2026-10-28.png | 72836 | 069e90704a5d8e6ed3259182c7e54dbb606561de6512ba6b3928b1124362edc1 |
| Docs/AIHEROES_PROPOSAL_ANALYST_2026-10-28.png | 74391 | 2ab136352b5dfc890d8380d51cd8aa586fceb78f3a9b7888c04bd870e036012b |
| Docs/AIHEROES_PROPOSAL_MAP_2026-10-28.png | 357544 | 7b7a6b78e0d2d5bcd1a11cf82ac517f0dff88077fbca8a26dc557f9671ba8701 |
| Binaries/Win64/UnrealEditor-AIHeroes.dll | 1094656 | 3b26336a6a93c585924cfa8a6519ac476d0ed24c42971d151146bd5aa1d55082 |
| Docs/AutomationRuns/20261028T200750062Z/index.json | 30482 | 1ca37ab38b338d582e1013fdb29d24c7edd3ed873b8b54bb4a8831775c38c15b |

These hashes evidence byte identity at read-back, not ongoing sync or
deployment. If parallel activity changes any file, rerun the hash manifest
and compilation gates rather than silently claiming all results still apply.

## Required next gate and nonclaims

- Automated and 1280x720 window-based QA both passed for the current PC
  snapshot. The next gate is to integrate a real licensed local inference
  adapter and measure actual latency, RAM/VRAM, deterministic fallback and
  explicit opt-in, not to pretend the gameplay slots are real models.
- Recheck source/binary/report hashes and rerun full Unreal automation
  after any subsequent gameplay code changes.
- Do not claim Android, real 1B/3B/8B model, filesystem knowledge,
  network inference, actual Iskra agent protocols, deployment or user saves.

Stage: PASS (local compiler, 77/77 Unreal automation, targeted live-window
QA, screenshot read-back). Manual inputs and Android remain unverified.
metric computation unavailable for mobile FPS, battery, real inference.

∆DΩΛ:
∆: typed, bounded, validated AI suggestions with explicit human authority.
D: SoT verified -> source/readback -> Unreal build -> 77/77 tests -> SHA.
Ω: 95% for compiled/tests+scripted window behavior; manual touch and
real inference remain unverified.
Λ: revise after future integration changes or a verified model adapter.
