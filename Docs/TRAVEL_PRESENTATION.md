# Strategic travel presentation

Status: PASS_SCOPED_WINDOWS for the tested presentation slice.
Verification: Saved/QA/TravelViewValidation_20261028T211414863Z.
The complete working tree also contains concurrent features; this document does
not claim authorship or current verification of later edits outside this slice.

## Play

Select a visible destination. A known affordable route has a cyan connecting
line and chevron; an over-budget route has an amber chevron without the
connecting highlight. The text also states the exact movement shortfall.
Unknown, blocked and invalid routes have no invented path. Collision is disabled
on the marks so they do not swallow map selections or mouse/touch traces.

After a confirmed non-combat move, the hero marker travels along the approved
waypoints. Logical position and the one movement debit are already committed.
The target strip explicitly identifies the animation and committed move.
Another strategic command finishes the old presentation before proceeding.
Combat entry and post-victory placement remain immediate; this is not a character
locomotion system or a new battle rule. The marker still uses prototype geometry.

## Contract

TravelRules remains the sole path and price authority. The presenter receives
copied waypoint positions and status, never a GameMode pointer or command API.
It cannot change resources, ownership, scouting, recruitment, battle rewards,
save schema or model capabilities. Load and save-management commands finish the
presentation; durable position remains the logical destination, not the marker.
Missing presentation actors/markers cannot undo a move or leave a gameplay lock.

FAIHeroesTravelMotion uses distance along each segment, not a diagonal shortcut.
The duration is bounded to 0.35..5 seconds; at most 129 copied points are accepted.
Negative/nonfinite elapsed time and malformed paths are rejected. Repeated
geometry uses one instanced mesh component, with at most three instances per
edge. Unchanged preview geometry is cached instead of rebuilt each frame.

## Verified evidence

- Unreal Editor target build succeeded; installed MSVC still has the existing
  newer-than-preferred toolchain warning.
- Fresh full automation report: 137 tests, 0 failures, 0 skipped, 0 test warnings.
- Eight TravelView groups passed: segment/frame parity; invalid inputs/bounds;
  read-only preview/cache/collision; one commit/next command; visual loss;
  modal/teardown; in-memory durable destination; blocked action.
- Real-window scripted run exited 0. It selected FOREST, ran the real battle,
  returned to HQ with an intermediate moving marker, then showed an over-budget
  route. No save/load commands were sent by this graphical smoke.
- All 143 source files and the DLL were unchanged during that verification.
  The original four player saves also matched by name, bytes and SHA-256.
- Three 1280x720 screenshots were inspected: VIEW_READY.png, VIEW_MOVING.png and
  VIEW_OVER_BUDGET.png under Saved/QA/TravelPresentation.

The initial build found an auto-pointer deduction error in the smoke test's
TObjectPtr loop. A concurrent correction was read back before the successful
build. No failure is counted as a passed test.

## Visual limitation and pending proposal

The existing lower HUD hides some map area; the small chevron is not readable
everywhere. Larger repeated chevrons were prepared, but a further rebuild was
blocked by other active QA windows. The active source was restored BYTE-FOR-BYTE
to its tested SHA-256, rather than retaining an untested cosmetic edit.
Docs/TRAVEL_CHEVRON_POLISH_PROPOSED.patch preserves that optional follow-up.
It is PROPOSED, NOT APPLIED and NOT VERIFIED. No other agent's window was closed.
Other existing HUD label overlaps and finished art are outside this slice.

## Reproduce

VS Code task: AIHEROES: Verify travel visuals and motion
Command: powershell -NoProfile -File Tools/verify_travel_presentation.ps1 -Visual
The verifier refuses to replace a DLL used by another running game/editor.
It checks a fresh JSON report, eight required groups, source/DLL drift and saves.

## References checked for implementation

- Actor ticking: https://dev.epicgames.com/documentation/unreal-engine/actor-ticking-in-unreal-engine
- Instanced mesh: https://dev.epicgames.com/documentation/unreal-engine/instanced-static-mesh-component-in-unreal-engine
- Actor transforms: https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/AActor
- Material parameter discovery: https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UMaterialInterface
- Automation JSON report: https://dev.epicgames.com/documentation/unreal-engine/run-automation-tests-in-unreal-engine

These establish API behavior, not measured Android performance. No APK, actual
LLM, publication, commit/push or global settings change is claimed here.
Rollback: inspect scoped pre-edit copies in Saved/QA/TravelPresentation/
baseline_20261028T210743209355Z and reverse only this slice. Never reset or
replace concurrent fog, field-dwelling, zone-generator or other project work.
