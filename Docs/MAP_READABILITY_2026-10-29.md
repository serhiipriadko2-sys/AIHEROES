# Map labels and route readability

Status: IMPLEMENTED, verification in progress. This file is not a release claim.
Scope: existing Windows Canvas HUD and selected-route scene geometry.
No game balance, travel price, fog discovery, combat, model or save-schema changes are owned by this slice.

## Changes

- Label placement is restricted to a map rectangle below the threat strip and above the mission/action panels.
- The complete two-line site/facility label occupies one rectangle; labels cannot overlap each other.
- The selected site is placed first; the remaining sites use case-normalized stable ID order.
- Unknown, hidden, offscreen and nonfinite projections are not displayed. Duplicate visible IDs fail closed.
- A displaced label has a clipped leader line. Selected labels also carry a `>` character, not only a tint.
- A bounded search uses at most 45 candidate positions per label and accepts at most 64 input labels.
- If a window is too short, labels are omitted rather than painted on action controls. This does not make the entire UI suitable for that resolution.
- Existing route-plan badges do not cover site labels. Mission text stops before FIELD/LAB/GOALS controls.
- Each selected-road edge now has two larger chevrons. Ready = line plus chevrons; over-budget = chevrons only, plus existing textual cost.
- Repeated route geometry still uses one instanced component, no collision, no shadow and an unchanged-input cache.

## Evidence gates

Baseline observed this session: 138 tests passed, full existing travel-view render scenario passed; four human saves unchanged.
New test group `AIHeroes.Readability` has six tests:
SafeMapBounds; LabelsDoNotOverlapPanelsOrEachOther; HiddenAndInvalidInputs;
StableSelectedPriority; ResolutionAndCrowdingSweep; RepeatedChevronGeometry.
The sweep contains 60 viewport dimensions, including layout breakpoints and deliberately undersized windows.
A group PASS is not a device-performance or whole-interface accessibility claim.

Verification command (close/finish existing engine sessions first):

```powershell
powershell -NoProfile -File Tools/verify_map_readability.ps1 -Visual -Snapshot
```

The snapshot mode builds a private, immutable copy under Saved and does not replace a DLL used by another session.
It copies only Source, Config and AIHeroes.uproject, never .iskra, credentials or player saves.
Visual checks run at 1280x720, 960x720 and 640x720 and preserve separate fresh images and logs.
Automation checks every test state and all six required names, not process exit alone.

## Boundaries

This is custom Canvas placement around known HUD panels, NOT Unreal UMG Safe Zone support.
Phone notches, device DPI, portrait mode, touch ergonomics, Android APK and framerate remain separate gates.
The 3D scene still has prototype art; world-space cues may still be occluded by geometry or the HUD.
Top resource/help text at narrow widths is not comprehensively reflowed by this change.
No new click target is added to displaced labels; selection still uses the existing world click path.
The full current worktree is not certified if other sessions change it after the frozen snapshot.
The older TRAVEL_CHEVRON_POLISH_PROPOSED.patch is retained as history; its intended visual change is included here, subject to the new receipt.

## Primary-source grounding (observed 2026-10-29)

1. Epic: Instanced Static Mesh Component. Repeated geometry can share a component; target-device performance still requires profiling.
   https://dev.epicgames.com/documentation/unreal-engine/instanced-static-mesh-component-in-unreal-engine
2. Microsoft XAG 103: visual information should not depend only on color; textual/contextual alternatives help.
   https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/103
3. Microsoft XAG 102: inspect contrast of labels, map elements and backgrounds. Solid label backgrounds used; comprehensive contrast audit not claimed.
   https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/102
4. Epic: UMG Safe Zones. Device safe insets are distinct from this Canvas panel exclusion rectangle.
   https://dev.epicgames.com/documentation/unreal-engine/umg-safe-zones-in-unreal-engine
5. Epic: Run Automation Tests. ReportExportPath produces JSON evidence for complete-run validation.
   https://dev.epicgames.com/documentation/unreal-engine/run-automation-tests-in-unreal-engine

Rollback: review scoped pre-edit bytes under Saved/QA/MapReadabilityWork/baseline_20261029T060919816397Z;
reverse only this slice. Never reset unrelated work or replace the whole shared HUD from an old backup.
