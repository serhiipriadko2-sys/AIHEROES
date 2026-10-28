# Headquarters development - verified Windows development slice

Status: IMPLEMENTED / COMPILED / AUTOMATION-PASS / WINDOWS-INPUT-PASS.
This is not an Android release, polished art pack, model integration or deployment.

## Play
Select the owned HQ and bring the hero there. Press U or click HQ + U, beside
the model-slot button, to purchase one level. Selection and H show the cost.
The hero starts at HQ but HQ must still be selected explicitly.

| Current level | HQ gold per day | Next upgrade: gold / wood / ore |
|---|---:|---|
| 0 | 40 | 100 / 10 / 10 |
| 1 | 60 | 180 / 20 / 15 |
| 2 | 80 | 300 / 35 / 25 |
| 3 | 100 | Maximum level |

Income starts on NEXT DAY, not on purchase. An owned outpost still adds 30 gold;
forest and ore-mine payouts are unchanged. These prices are provisional balance
choices, not a player-study result. Three simple mesh additions show progression.

## Authority and persistence
- The server-side GameMode method preflights selection, ownership, actual hero
  location, level and all resources. Rejected purchases leave state unchanged.
- Live battle, historical replay and the open dossier block construction.
- BaseTier participates in the battle campaign fingerprint and dossier freshness.
  Unsupported mid-battle changes therefore cannot silently commit a result.
- Protocol and model-slot progression are independent; construction loads no LLM.
- BaseTier is an optional tagged property in schema 1; absent legacy fields default
  to zero. Save validation and A/B read-back compare the new level explicitly.
- End-turn arithmetic now preflights day/resource limits with wide intermediate
  totals before changing day, movement or income; partial updates are refused.

## Verification on 2026-10-28
Baseline HEAD: d6ee2059ffc2daa50b9882754c2c854d0dedf387.
The working tree was clean initially. Other sessions subsequently added objectives,
AI proposal checks and grid replay; those features are not attributed to this slice.

1. Baseline all-gameplay suite: 58/58, zero warnings.
   Docs/AutomationRuns/20261028T195815013Z/index.json
2. Build verification: Result Succeeded; source hashes unchanged over that check.
   Saved/QA/BaseDevelopment/build3.log
   Saved/QA/BaseDevelopment/source-before-build3.json
   Saved/QA/BaseDevelopment/source-after-build3.json
3. All seven new headquarters test groups: 7/7, zero warnings.
   Docs/AutomationRuns/20261028T200707446Z/index.json
4. Integrated gameplay suite on the compiled snapshot: 77/77, zero warnings,
   no skipped tests; the loaded DLL did not change during this run.
   Docs/AutomationRuns/20261028T200750062Z/index.json
   One parallel grid-replay TEST source changed during the run, so this report
   attests the compiled snapshot, not every later edit in the shared working tree.
5. Real dedicated 1280x720 Windows game: clicked HQ, pressed U for level 1,
   advanced day (+60 gold), clicked action 201 for level 2, rejected an unaffordable
   level-3 purchase, advanced day (+80 gold). Final state: day 3, 100 gold,
   5 wood, 5 ore, HQ2; protocol/model slots remained zero.
   Saved/QA/BaseDevelopment/visual-qa.log
   Saved/QA/BaseDevelopment/HQ_before.png
   Saved/QA/BaseDevelopment/HQ_level1.png
   Saved/QA/BaseDevelopment/HQ_level2.png
6. Four pre-existing player save files retained identical names, bytes and SHA-256.
   Visual QA sent no save/load commands. Disk tests use only their own GUID slots.
   Saved/QA/BaseDevelopment/save-preservation-result.json

Seven groups cover pricing, exact budget/failure atomicity, location and income,
modal locks and battle conflicts, disk validation/recovery, day/resource bounds,
visual/button isolation, and legacy tagged payloads with BaseTier actually omitted.
The legacy-payload test is not a claim that every historical save file was tested.

## Reproduce / rollback
Use the existing VS Code Build and Test all gameplay automation tasks.
Tools/qa_headquarters.ps1 targets an explicitly supplied QA PID and exact log tag;
review the initial screenshot before coordinate-based actions. It rejects an
already-upgraded scenario rather than accepting old log entries as fresh evidence.

Scoped pre-edit backups and before/after hashes are in
Saved/QA/BaseDevelopment/baseline and integration-receipt.json. Roll back only the
headquarters changes after reviewing current diffs; do not overwrite concurrent
work. No commits, pushes, branch switches, model downloads or global changes were
performed by this development slice. Only its own QA game was closed afterward.

## Remaining boundaries
Android device/touch and APK QA not run. Model weights/inference not connected.
World art remains placeholder geometry. Level 1 and 2 were checked in the actual
window; level 3 and recovery paths were checked by automation. Balance playtesting
and a dedicated base-management screen are later tasks, not completed features.
