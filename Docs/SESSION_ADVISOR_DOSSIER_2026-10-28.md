# AIHEROES - Advisor dossier development

Status: IN PROGRESS. No release, APK or real model inference claimed.
Surface: local project C:\github\AIHEROES through Desktop Commander.

## Confirmed baseline
- Git HEAD: 2e098e2e1e1b1fd113fe288f1ae074b50a2bd174; user/concurrent changes retained.
- Previous isolated recruitment build: Result: Succeeded; its test report is missing.
- Newer root-project report: Docs/AutomationRuns/20261028T193408158Z/index.json.
  51 Success, 0 failures, 0 warnings, 0 not-run. This is baseline evidence, not new-feature QA.
- SoT30 local read-only verification: PASS, 30/30 Knowledge, 32/32 checksums.

## Current implementation scope
Add an expandable read-only advisor dossier: full advice, recruitment alternatives,
resource costs and protocol restrictions. Re-evaluate freshness after changes to
army, target, day or resources. A stale report must never look like a fresh forecast.
No automatic orders, model downloads, save-format changes, Git commits or pushes.
Preserve existing battle controls and strategic hotkeys.

## Verification gates
1. Back up each changed file; refuse an edit when its preimage has changed.
2. Add deterministic document/layout tests and GameMode read-only integration tests.
3. Build sequentially (-NoUBA -MaxParallelActions=1) only when no competing build/game locks the DLL.
4. Run the full AIHeroes suite and inspect the fresh JSON report.
5. Run an isolated owned game smoke test and capture only that game window.
6. Read back source hashes and record remaining limitations.

## Progress
Baseline read-back completed. Implementation and new-feature tests pending.
