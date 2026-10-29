# Measured HUD text and readability recovery

Status: IMPLEMENTED / verification pending. This document is not a release claim.
Surface: C:\github\AIHEROES, existing AAIHeroesGameMode / Canvas HUD.
Baseline HEAD observed: 429ad91e33defebb618543f3ae58c2fd7ab67662.

## User-visible change

The strategic header measures each rendered line with AHUD::GetTextSize.
It reserves actual SAVE/LOAD/SLOT/REPLAY rectangles on intersecting rows.
Small windows use a compact title, resource summary and command hint; no font is
shrunk further by the fitter. Text that still cannot fit ends with an explicit
ellipsis. Event, adviser, tactical-summary, threat and mission lines are measured
including their prefixes and ellipsis, not estimated from character count.
The underlying messages and adviser dossier are unchanged. No purchase, move,
combat, model, fog, save schema or input hitbox changes belong to this slice.

The helper rejects nonfinite dimensions/measurements and input over 8192 TCHARs.
Single-line rendering normalizes control line breaks to spaces. Truncation does
not split a UTF-16 surrogate pair. This is NOT a full localization, grapheme
shaping, screen-reader, DPI, text enlargement or mobile safe-area implementation.

## New automated checks

Four mandatory AIHeroes.HUDText groups:
- FitAndDisclosure: variable glyph widths, exact fit, ellipsis and input immutability.
- InvalidInputs: invalid dimensions/measurements and bounded input handling.
- UtilitySeparation: 54 width/row combinations; utility hitbox geometry unchanged.
- BoundedWidthSweep: bounded font-measurement work and UTF-16 pair preservation.

Existing six AIHeroes.Readability groups remain required, including the 60-case
viewport/crowding sweep and repeated-chevron geometry. A written test is not a pass.

## Verification tool repair

Tools/verify_map_readability.ps1 now checks queued/running UnrealBuildTool commands
in addition to active engines/compiler processes. This is a preflight check, not
a system-wide lock and not protection against a later non-cooperating process.
Snapshot source enumeration uses the original project file even after the build
project path changes. PNG verification reads IHDR and rejects a screenshot whose
actual dimensions differ from the requested 1280x720, 960x720 or 640x720 window.
The receipt records actual dimensions and the four new required test groups.

Preferred incremental verification when the worktree and engine are idle:

```powershell
powershell -NoProfile -File Tools/verify_map_readability.ps1 -Visual
```

`-Snapshot` remains available for an isolated source copy, but forces a separate
build and is more expensive. No global build configuration or pagefile is changed.

## Recovered historical evidence

Saved/QA/MapReadability_20261029T061206815Z/build.log ended in failure, with
C3859/C1076/C1060 memory diagnostics. It produced no final test report or receipt.
A fresh attempt at MapReadability_20261029T063643680Z exposed a unity-build name
collision and an incorrect camera include. Another session corrected the save
constant name and camera include before this slice patched the HUD. Those fixes
are read-back observations, not authorship claims for this work.

## Sources consulted on 2026-10-29

- Epic AHUD::GetTextSize: https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/AHUD
- Epic automation JSON reports: https://dev.epicgames.com/documentation/unreal-engine/run-automation-tests-in-unreal-engine
- Microsoft XAG 101, text readability: https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/101
- Microsoft PCH memory diagnostics: https://learn.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/fatal-error-c1076

Rollback: reverse only this slice after comparing current files with the pre-edit
copies under Saved/QA/HUDTextWork_20261029T063804968Z. Never restore a whole old
worktree or overwrite concurrent gameplay changes. No commit or push performed.
