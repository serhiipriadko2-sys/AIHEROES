# AIHEROES — reliability cycle, 2026-10-29

Visible work log. This file is a development ledger, not gameplay acceptance.

## A Intake / B SIFT

- User requested continued development and improvements.
- SoT30 integrity rechecked: PASS, 30 knowledge files, pinned manifest intact.
- Active config: `/Script/AIHeroes.AIHeroesGameMode`.
- Baseline at fresh read: HEAD `429ad91`; worktree clean. No Git writes by us.
- Historical automation is not promoted to proof for new source changes.

## C Frame

Bounded slice: load-map identity, atomic battle rewards, source/binary QA
provenance. See `ADR/ADR-20261029-02_TRANSACTION_AND_QA_INTEGRITY.md`.
Three independent implementers have exclusive files. Root owns integration,
VS Code tasks and serial Unreal verification. No parallel Editor-Cmd runs.

Ruling: preserve existing workspace/branch rather than create a worktree or
commit: user explicitly requires changes in this open workspace and forbids
commits. No core protocol, model inference or procedural schema changes.

## D Step

- Load mapping helper and four regression tests: implemented, Unreal run pending.
- Reward preflight, three rules tests and two transaction tests: implemented,
  Unreal run pending.
- Build manifest and test evidence checks: implemented. PowerShell 5.1 fixture
  checks 23/23 PASS; parser 4/4 PASS; static Codex setup checks 20/20 PASS.
- Common interfaces: no runtime overlap. Both game fixes preserve save schema;
  tooling consumes fixed source/DLL paths and does not modify gameplay state.

## E Verify / F Close

Independent static review found no P1/P2 defect in the scoped changes.
No runtime RED was executed before the game fixes. The duplicate-map regression
includes the former insufficient check and its counterexample; this is not
described as a baseline Unreal run.

Build preflight correctly refused to start while a foreign QA game was active.
Read-back of PID 33940 identified `-game` and `AIHeroesZoneExpeditionGameMode`,
not an unsaved ordinary Editor workspace. Its LiveCodingConsole was a child.
No process was terminated. Other writers also changed HUD/zone/save files;
their changes are preserved, not attributed to this slice.

Windows window-control connection failed (`missing field sandboxPolicy`).
Opening this log via the existing VS Code CLI was attempted; CLI emitted a
crash-reporting error, so visible editor/task execution is not verified.
Native build/test commands and generated evidence are the fallback, not a
claim of manual input QA. Two build attempts stopped at the live-process
preflight; neither ran UBT. Elevated process inspection confirmed the same
foreign QA game was still active. A limited coordination request was sent.

Current scoped status: PARTIAL / implemented, runtime-unverified. Nine new
Unreal automation tests are present but were not run against a fresh binary.
The full AIHeroes group, source-to-DLL build verification and fresh visual smoke
remain pending a free native-work window. No historical test count is used as
current proof. All process closures and user-save changes remain zero.

After the QA game is closed: run the Build task, then the full automation task;
inspect the fresh report/provenance. Only then run the visual live-grid task
and perform manual input/save-load acceptance separately.
