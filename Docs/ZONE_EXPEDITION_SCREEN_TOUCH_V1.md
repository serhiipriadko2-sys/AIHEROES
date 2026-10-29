# AI HEROES — Zone Expedition screen controls and touch bridge v1

Date: 2026-10-29
Target: opt-in Unreal Engine 5.8 ZoneExpedition, NOT default 5-site campaign.
Lifecycle: Windows prototype implementation; exact build/QA receipts are
tracked separately. This is NOT a physical Android APK/device validation.

## Intent and external sources

The active 7–11-zone expedition has human-authorized turn-based movement,
AI-resolved Grid V2 combat and a read-only known-frontier adviser.
A keyboard-only control layer makes the mode unusable on a touch-only
device, regardless of the combat design.

Unreal 5.8 GetInputTouchState:
https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/APlayerController/GetInputTouchState
Unreal 5.8 screen-coordinate world trace:
https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/APlayerController/GetHitResultAtScreenPosition
Unreal 5.8 UMG Safe Zones and per-device visual preview:
https://dev.epicgames.com/documentation/unreal-engine/umg-safe-zones-in-unreal-engine
Unreal 5.8 DPI Scaling:
https://dev.epicgames.com/documentation/en-us/unreal-engine/dpi-scaling-in-unreal-engine

The HoTA template editor's roads and guarded zone connections
(https://h3hota.com/en/template-format) are general design references.
No HoTA source, art, map, name, factions or UI was copied.

## Implemented prototype

- Source/AIHeroes/Public/AIHeroesZoneScreenLayout.h
- Source/AIHeroes/Private/AIHeroesZoneScreenLayout.cpp
- Source/AIHeroes/Private/Tests/AIHeroesZoneScreenLayoutTests.cpp
- Source/AIHeroes/Public/AIHeroesZoneExpeditionGameMode.h
- Source/AIHeroes/Private/AIHeroesZoneExpeditionGameMode.cpp
- Source/AIHeroes/Private/AIHeroesZoneExpeditionHUD.cpp
- Tools/qa_zone_touch_buttons.ps1

All source is local Win64 editor code; no UMG asset dependencies,
network services, camera control rework or SaveGame schema migration.

The screen layout uses the SAME rectangle computation for drawing
the Canvas interface and for input hit-testing. Two rows, five
columns; the supported prototype viewport is at least 640x520.
Below this minimum the touch layout fails closed instead of
overlapping or dispatching an unintended order.

Top row: NEXT selection, HERO advice, INSPECT known selection,
ROUTES comparison, MOVE/FIGHT player confirmation.
Second row: NEXT DAY, RECRUIT, PROTOCOL, SAVE and LOAD.
The two high-impact Save/Load controls are visually distinguished.
The MOVE/FIGHT button is explicit and visually emphasized.

The read-only ROUTES overlay replaces all actions with exactly
one CLOSE ROUTES button. All other pointer positions are swallowed,
including positions formerly occupied by MOVE/FIGHT, next day,
Save/Load, or the map. Keyboard D/Esc closure remains intact.

Touch input checks EKeys::Touch1 and GetInputTouchState; mouse uses
LeftMouseButton and GetMousePosition. Both are routed to
HandleScreenPress; an outside-map pointer is projected with
GetHitResultAtScreenPosition and can only SELECT a discovered 3D node.
The touch branch is evaluated before the mouse branch to mitigate a
duplicate synthetic mouse click on the same frame. No touch can auto
select and fight in one press: selecting only changes SelectedSite,
then MOVE/FIGHT calls the original ConfirmPlayerDestination.

The authoritative game mode continues to validate movement, resource
spending and battle outcomes. Screen rectangles do not grant orders.
QA prints [ZONE INPUT] POINTER BUTTON <action> for diagnostics.

## Test and validation boundaries

- Automation tests check deterministic 10-button layout for
  1280x720, 960x540, 720x1280, 640x520, and 1920x1080.
- Minimum button height=42 viewport pixels; no overlaps.
- Modal contains only CLOSE; no hidden high-impact hit targets.
- Tiny/oversized and offscreen taps fail closed.
- Native Windows window-targeted mouse click scripts check actual
  3D engine logs, HUD actions, GPU screenshots and user save hashes.
- 1280x720 and 960x540 desktop-window QA are visualization/input
  proxies, NOT proof of native touch delivery on Android.
- No physical display-cutout, DPI/orientation/overscan or gesture
  QA is claimed. Manual margin padding is NOT a real UMG Safe Zone.
- Touch1 availability, system gestures, virtual joysticks and
  multi-finger handling on hardware are unverified.
- Current screen labels and controls are prototype English,
  not an accessibility or localization completion claim.
- Smaller screen sizes below 640x520 show an explicit resize/rotate
  notice. Keyboard remains usable there on the PC.
- The control panel may overlap some visual map space; a proper
  UMG layout with Safe Zone widgets is a future product/UI step.

## Provenance / parallel agent constraints

At task start git HEAD was cae9f7b and the shared working tree had
many uncommitted edits from other agents. No changes were made here
to the default game HUD, art, battle textures, or shared packaging
script. Pre-edit copies of the three altered GameMode/HUD C++ files
and the HUD header are held at:
Saved/QA/ZoneTouchBefore_20261029/

Build workflow: Tools/build_aiheroes_editor.ps1 -> exact input hashes
-> Tools/run_aiheroes_automation.ps1 -> native game QA -> visual
image hashes -> release-style read-back receipt.

Initial whole-project test run after touch UI was 222/224,
with three ZoneTouch tests PASS. Two unrelated tests failed while
the separate battle-art work was being actively edited:
AIHeroes.LiveBattle.StagePauseAndFinish (initiative label) and
AIHeroes.Visual.Battle.SixModelsAndPortraits (portrait dimensions).
Do not misrepresent the entire suite as green until a new source-
pinned report is available.

No Git push/merge, production slot write, APK, package or
real neural model inference is implied.

Rollback: reverse only the ZoneScreenLayout, ZoneTouch tests,
dedicated QA script and surgical GameMode/HUD integration.
Leave all other agent changes alone; re-read current state before
applying rollback. The game state and save version are unchanged.

Delta: portable mouse/touch-ready HUD control layer.
D: official UE API -> input/layout -> build -> tests -> GPU QA.
Omega: <=95% for Windows-observed effects.
Lambda: Android touch device and safe-zone/DPI QA before launch.
