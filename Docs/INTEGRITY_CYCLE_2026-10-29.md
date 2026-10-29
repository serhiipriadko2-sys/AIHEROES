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

Previous checkpoint: PARTIAL / implemented, runtime-unverified. Nine new
Unreal automation tests are present but were not run against a fresh binary.
The full AIHeroes group, source-to-DLL build verification and fresh visual smoke
remain pending a free native-work window. No historical test count is used as
current proof. All process closures and user-save changes remain zero.

After the QA game is closed: run the Build task, then the full automation task;
inspect the fresh report/provenance. Only then run the visual live-grid task
and perform manual input/save-load acceptance separately.

## Resumed verification after user confirmation

The user reported the QA game closed and parallel writes paused. Elevated
process inspection found no Unreal/Live Coding process. Git had advanced
externally from `429ad91` to `87d9012`; this agent made no Git/index writes.
The complete current working source, including preserved parallel work, was
checked rather than assuming the earlier source baseline.

- Build: PASS, UBT target up to date (0 actions). Stable 178-input manifest:
  `BuildRuns/20261029T112650766Z/compiled.json`. UBT cautioned about installed
  compiler 14.51 being newer than its preferred 14.50; no compile failure.
- Full strict automation: **173/173 Success, warnings/errors/skips 0** in
  `AutomationRuns/20261029T112705252Z/index.json`. All nine new integrity
  regressions passed. `provenance.json` matches source, DLL and report hashes.
- Independent read-only reviewer rechecked all 178 actual inputs, DLL, report
  counters, unique test paths and the nine named regressions: PASS.
- Graphic attempt 1: the game logged PASS and requested status 0, but the
  observer exposed a null native ExitCode. The wrapper refused PASS. Logs,
  frames and `observer_failure.json` remain in `VisualRuns/20261029T112905018Z`.
  No gameplay/source fix or logical retry was introduced.
- Graphic attempt 2: explicit `Start-Process -Wait -PassThru` observed exit 0.
  `VisualRuns/20261029T113106551Z/evidence.json` records six fresh frames and
  source/DLL checks before/after. All six PNGs were visually inspected. Pause,
  highlighted path, moving units/attack, 2x, 4x fast finish, single result,
  exact strategic camera return and read-only dossier passed the script.
- Four existing save files were compared by count/hash before and after the
  successful smoke and were unchanged. The smoke did not execute save/load.
- Final rechecks: tooling fixtures 23/23, PowerShell parser 4/4, Codex static
  setup 20/20, source/DLL manifest read-back and `git diff --check`: PASS.

Scope status: **PASS for implemented integrity fixes, build, full automation
and scripted PC smoke**. **PARTIAL for complete human PC acceptance**: manual
keyboard/mouse and visual save/load remain unverified. The Windows control
connection still returns `missing field sandboxPolicy`; no alternate foreground
input injection was used. Real neural weights and Android are not claimed.

Current evidence: `INTEGRITY_VERIFICATION_2026-10-29.json`. The earlier
`INTEGRITY_RECEIPT_2026-10-29.json` remains a historical blocked checkpoint,
not current runtime status. No commits/pushes, SDK installs, private-canon
changes, user-save edits or foreign process closures by this agent.
