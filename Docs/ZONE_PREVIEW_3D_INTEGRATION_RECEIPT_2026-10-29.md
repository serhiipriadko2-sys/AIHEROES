# AI HEROES — Isolated 3D Zone Preview: verified development receipt

Observed: 2026-10-29 00:30 Windows device time (+03), 2026-10-28 21:30 UTC.
Device: DESKTOP-ITUAMNF.
Project: C:\github\AIHEROES, Unreal Engine 5.8, local main HEAD cc062ba at readback.
Status: IMPLEMENTED / EDITOR BUILD PASS / UNREAL AUTOMATION PASS /
        TWO REAL GAME-VIEWPORT VISUAL QA RUNS PASS / FILE QC PASS.
Final file QC: 11/11 manifest rows matched byte length and SHA256;
no case-fold duplicates, symlinks, obvious credential patterns, invalid
PowerShell syntax or git whitespace errors were observed. Both PNGs
were read back as 1280x720; two independent window QA receipts were
PASS, and the full Unreal automation reported 138/138 successful tests.
No Git commit, merge, push, deployment, mobile package or player-save
migration was executed by this operator.

## Intake, SIFT and research

The owner requested continued comprehensive development with current
internet research. The previous AI HEROES milestone delivered a
deterministic C++17 generator for 7..11 zones and a validated AdventurePath
oracle, but did NOT create 3D content or replace the active five-node
campaign. That boundary remains intact in this milestone.

Official HotA Random Map Template Editor:
https://h3hota.com/en/template-format
Official HotA random templates:
https://h3hota.com/en/templates
The original HotA generator uses map zones, types of connections,
guarded chokepoints, and road policies. The implementation here is
original AI HEROES graph visualization, NOT an import of HotA data,
assets, art, maps, code, strings, music, heroes or faction names.

Epic Unreal Engine 5.8 documentation:
https://dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-overview
https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FScreenshotRequest/RequestScreenshot
https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UTextRenderComponent

Unreal PCG can eventually render richer procedurally generated terrain,
but it does not replace topological validation and is NOT used in this
milestone. Unreal's FScreenshotRequest was verified via installed UE 5.8
headers before implementing the game-viewport screenshot gate.

## Implemented behavior

A separate developer-only AAIHeroesZonePreviewGameMode (directly derived
from AGameModeBase, not AAIHeroesGameMode) renders the actual
FAIHeroesZoneTemplate output. It is selected only by the explicit
/Engine/Maps/Entry?game=/Script/AIHeroes.AIHeroesZonePreviewGameMode
URL. DefaultEngine.ini, existing 5-site campaign and save slots remain
unchanged.

- 7..11 stable role-colored zones placed on a rectangular preview board;
- two guarded starting resource choices, a central guarded checkpoint,
  and two late guarded objectives;
- 0..4 optional, seed-driven side routes;
- exactly one visible road per pair of stored directed links;
- read-only guard labels, HQ label, and reproducible coordinates;
- a dedicated, fixed orthographic CameraActor viewing the entire map,
  independent of PlayerController possession;
- built-in Unreal cube, sphere, text, sun and skylight only;
- ordinary PlayerController, no AI HEROES strategic-action bindings;
- no SaveGame API calls, hero movement, combat or data persistence;
- no real 1B/3B/8B neural model or remote inference call.

The developer view deliberately reveals the FULL validation graph.
It is not the fog-of-war-restricted player intelligence surface.
No generated map is currently executable by the five-site campaign;
before that step a versioned map-save schema and rollback are required.

## Failure investigation and repair evidence

First Windows screenshot attempt returned a black/white frame through
PrintWindow/BitBlt, although the world-spawn log showed 10 actors and
14 roads. Both capture methods were honestly marked FAIL. A viewport-
native Unreal FScreenshotRequest produced a true 1280x720 GPU frame,
which was also black before camera repair. The defect was in camera
orientation/possession, not merely screenshot retrieval.

After introducing the independent orthographic CameraActor, the
actual GPU capture showed the map. Follow-up work reduced light wash,
expanded the map's on-screen footprint, enlarged labels, corrected
their upside-down rotation, and moved labels away from site markers.
Read-back reviewed the actual final PNG images, not a render promise.

One integrated build initially failed because concurrently developed
Fog/FieldDwelling/TravelPresentation modules changed during Unreal UHT,
and one parallel QA game held the Live Coding mutex. Those were not
fixed by destructive resets. An old completed Fog QA game window was
closed gracefully after its final screenshots existed and its log was
idle over 120 seconds. Other developer sources were not reset and
foreign live windows were not force-killed.

## Deterministic automated evidence

UnrealBuildTool:
 AIHeroesEditor Win64 Development, Result: Succeeded (final source).
Full automation script:
 Tools\run_aiheroes_automation.ps1
Full report:
 Docs\AutomationRuns\20261028T212905823Z\index.json

Result: 138 tests, 138 Success, 0 Failed, 0 Warnings, 0 NotRun.
Including both new test cases:
 AIHeroes.ZonePreview.SeparateGameModeAndNoCampaignController
 AIHeroes.ZonePreview.GeneratedDraftCannotChangeCampaignTopology

At the start and end of that full 138-test run the editor DLL's
SHA256 was identical:
 418c8b760524ca21faa24da3d45c7d90e90d04e9e481b831a4501d3f4dc3e37a
Four owned sources (preview header, implementation, tests, QA script)
were also hash-checked before and after the run without drift.
A byte-identical frozen tested DLL is stored in Saved/QA:
 Saved\QA\ZonePreviewVerified_20261028T212905823Z\UnrealEditor-AIHeroes.dll

## Two window-targeted 3D GPU QA runs

QA command:
 powershell -NoProfile -File Tools\qa_zone_preview.ps1 -Seed 20261028 -Branches 3
 and a second run with -Seed 20261029 -Branches 4

1. Seed 20261028: zones=10, roads=14, labels=10, revision=2411826805.
   1280x720 PNG, 438917 bytes, GPU-sampled colors=131.
   Result: PASS_ISOLATED_3D_ZONE_PREVIEW, process exit 0.
   Source: Saved\QA\ZonePreview_20261028T212745974Z\receipt.json

2. Seed 20261029: zones=11, roads=16, labels=11, revision=2700400296.
   1280x720 PNG, 439568 bytes, GPU-sampled colors=116.
   Result: PASS_ISOLATED_3D_ZONE_PREVIEW, process exit 0.
   Source: Saved\QA\ZonePreview_20261028T212819101Z\receipt.json

Both screenshots were visually inspected. The graph composition differs
with the seed; roads and node markers are visible, and text is no longer
upside down. There is room for more visual polish; this is a validation
prototype rather than production-ready game artwork.

The QA script checks the precise process command line, explicit
GameMode and seed/branches, 1280x720 window, Unreal log, safe screenshot
path under Saved/QA/ZonePreviewFrames, real non-uniform image and source
SHA256. It copies the frame to a separate unique run folder, verifies
the copied hash, checks absence of campaign save and battle-commit
events, and gracefully closes ONLY its own game window in a finally
block even on failure. No global mouse/keyboard or desktop capture.

## Concrete byte/sha256 manifest (frozen read-back snapshot)

| Project-relative file | Bytes | SHA256 |
|---|---:|---|
| Source/AIHeroes/Public/AIHeroesZonePreviewGameMode.h | 893 | 6cb69a97309dd3611c577383c80335d4066f79d0288237744e3756db688b28ce |
| Source/AIHeroes/Private/AIHeroesZonePreviewGameMode.cpp | 13013 | 489d903c77c3a5790c79906a6ac6096cd36f15a386434e7566934e1189acc2b0 |
| Source/AIHeroes/Private/Tests/AIHeroesZonePreviewTests.cpp | 3188 | 434f3850899b5e88c58423a65c914b374f84478d003cb42891cd2611485f75e4 |
| Tools/qa_zone_preview.ps1 | 7817 | 283d5801fdeadb269464f346ca30e1372401e7986bb76fe2f5d5583d26c3a820 |
| Docs/ZONE_PREVIEW_3D_V1.md | 5946 | 5cfbfadc3def298545ce6d7b598f5227fbcc00cf1fec4c840fa4b3554f65cdfe |
| Docs/AIHEROES_ZONE_PREVIEW_10_ZONES_2026-10-29.png | 438917 | bfde374cc133a84ff4c902981dba61e4b4309065cff9faabf5555fb97e8622e2 |
| Docs/AIHEROES_ZONE_PREVIEW_11_ZONES_2026-10-29.png | 439568 | 3386a9886e49922ce43fc98b053233cfc4eb0ab4a48c8b4d314978ac6ca6c1c0 |
| Docs/ZONE_PREVIEW_QA_SEED20261028_2026-10-29.json | 819 | 776e4de404073d3bde6e08a6885683fb32ce37765357fe676f7564b0cbe036ca |
| Docs/ZONE_PREVIEW_QA_SEED20261029_2026-10-29.json | 819 | d9f47385bef3320b2b32c5720f6c1166098362c9859b047953dd297f7af98887 |
| Saved/QA/ZonePreviewVerified_20261028T212905823Z/UnrealEditor-AIHeroes.dll | 1860096 | 418c8b760524ca21faa24da3d45c7d90e90d04e9e481b831a4501d3f4dc3e37a |
| Docs/AutomationRuns/20261028T212905823Z/index.json | 54581 | 93ff0042c018e99d02b0b39880b30501848bcb4e4a6da05e0b5efb226c9b50fa |

This manifest verifies immutable bytes at specific project paths on the
owner's Windows machine. It is NOT a sandbox URL, a GitHub push or a
claim that other concurrent edits cannot later change shared files.

## Next safety and delivery gates

1. New world schema v2: explicit topology seed, zone IDs, discovery,
   positions, ownership, surviving guards and action revisions.
2. Migration design + isolated SaveGame roundtrip for old v1 slots.
   Never overwrite a real human save for migration tests.
3. Separate human-authorized preview -> move/attack executor that uses
   identical FAIHeroesAdventurePath graph and resources.
4. Eventually replace primitive meshes with original PCG terrain,
   roads, biome budgets, player-side fog, large-font HUD, readable
   minimap, controller/touch verification and performance budgets.
5. Separate Android SDK/NDK and physical-device QA, and an actual
   licensed local model adapter before making real 1B/3B/8B claims.

Status boundaries:
 PASS: standalone Windows preview mode, compiler, 138-test automation,
       two distinct GPU snapshots and game-window logs.
 PARTIAL: artwork remains prototype quality; no playable procedural
          campaign/save-v2/PCG/Android/real LLM/multiplayer.
 NOT RUN: mobile hardware performance, human UX sessions and deployment.

∆DΩΛ:
∆: safe orthographic full-map preview of seeded strategy-zone graphs.
D: HotA/Epic evidence → isolated GameMode → native GPU capture repair
   → compile → 138/138 tests → two distinct QA windows → hashes.
Ω: 95% for read-backed Windows development evidence.
Λ: revise after real playable migration and manual/mobile QA.
