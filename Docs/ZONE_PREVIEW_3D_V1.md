# AI HEROES — Isolated 3D generated-world preview

Date: 2026-10-29 (local Windows machine).
Status: standalone developer-mode visualization; not a playable dynamic campaign.
Source of truth for actual topology: AIHeroesZoneTemplate (C++17 + UE adapter).

## Why this is separate from the campaign

The existing AI HEROES playable campaign has five fixed strategic sites,
an authoritative GameMode, saved armies and resources, route rules,
mission goals, combat replay and three user save slots. The future
procedural generator produces seven to eleven logical zones. Replacing
the five-node map in the live GameMode before a versioned SaveGame
migration would destroy source-of-truth assumptions and could corrupt
or silently reinterpret a player's saved progression.

The preview has a separate native Unreal GameMode class:
  Source/AIHeroes/Public/AIHeroesZonePreviewGameMode.h
  Source/AIHeroes/Private/AIHeroesZonePreviewGameMode.cpp

This class derives directly from AGameModeBase, NOT the existing
AAIHeroesGameMode, and uses an ordinary APlayerController with no
AI HEROES recruitment, save, load, move, attack, duel, upgrade or
confirmation inputs. It is never selected by DefaultEngine.ini and
never instantiated in a normal campaign session. Shipping builds
short-circuit this mode without rendering a developer preview.

## Run

To launch an independent game window, validate its command-line
identity and capture only the window:

  powershell -NoProfile -File Tools/qa_zone_preview.ps1 -Seed 20261028 -Branches 3

To leave the preview open for a human review:

  powershell -NoProfile -File Tools/qa_zone_preview.ps1 -Seed 20261028 -Branches 3 -KeepOpen

The script launches Unreal Engine 5.8 using the URL-selected alternative
GameMode:
  /Engine/Maps/Entry?game=/Script/AIHeroes.AIHeroesZonePreviewGameMode

Use any nonnegative 32-bit signed seed; branches must be 0..4.
The generated QA log, PNG and receipt stay under a unique Saved/QA
directory. Since Windows PrintWindow captures a blank D3D swapchain,
the game uses Unreal FScreenshotRequest to capture the GPU viewport
when -ZoneCapture is explicitly set. A fixed orthographic CameraActor
frames the entire map, independently of player possession. The QA script
verifies the exact game PID, command line, filename under the local
Saved/QA/ZonePreviewFrames directory, 1280x720 dimensions, >8 sampled
colors and a SHA256-identical copy. No desktop capture, global click,
global keyboard focus, editor shutdown, model installation or user
save writes occur.

## Graphics and rules

Generated sites are colored built-in cube/sphere primitives. A stable
integer zone Grid is rendered with 160 Unreal units per cell on a
separate neutral platform. One mesh shows each undirected road, while
the model stores two directional edges for game authority. Danger
and role drive marker shape/color. Site labels indicate guards.

The game-mode preview checks:
  - deterministic validated zone template from a user-supplied seed;
  - 7..11 original sites;
  - two early guarded destinations are reachable;
  - late objective RUINS is blocked before defeating the checkpoint;
  - every zone marker and road is rendered exactly once;
  - preview window uses a stand-alone, non-campaign controller.

This is a developer visualization of FULL graph knowledge.
It does not implement fog of war and MUST NOT be presented as
knowledge legitimately acquired by a player or AI agent.

The built-in preview does NOT enable the Unreal PCG plugin, generate
a landscape/biome, place navigation meshes, add cities or real assets,
run a model checkpoint, start combat, or alter player resources.
An eventual PCG content pipeline can consume the validated world
metadata, but PCG point spawning is not game authority.

## External design references

HotA random map template description: zones, connections,
guard strengths, mandatory roads, wide and fictitious connections:
https://h3hota.com/en/templates
https://h3hota.com/en/template-format

Unreal Engine 5.8 PCG overview: points/attributes, procedural graphs,
spawners, runtime generation and plugin prerequisites:
https://dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-overview

Only generic game-design ideas are studied; AI HEROES does NOT include
assets, factions, unit types, source code, maps, dialogs or music
from HotA. Colors, nodes and layout here are newly authored.

## Acceptance matrix

1. Unreal Editor Win64 Development compile; no C++/UHT errors.
2. Automated tests under AIHeroes.ZonePreview:
   SeparateGameModeAndNoCampaignController,
   GeneratedDraftCannotChangeCampaignTopology.
3. Entire AIHeroes Automation suite with fresh JSON report.
4. Run the standalone URL GameMode with a non-default seed.
   Verify exact mode + seed + branches from process command line.
5. Log source: [AI HEROES ZONE PREVIEW] PASS and expected counts.
6. Observe a non-uniform real-frame capture from exact 1280x720 HWND.
7. Verify absence of campaign-save and combat-commit lines. If a screenshot
   is black, mark VISUAL_QA=FAIL even if the actor-spawn log says PASS.
8. Copy only verified screenshots to Docs with source SHA256 match.
9. Record file bytes/sha256 and save the tested DLL snapshot
   under Saved/QA; do not conflate parallel build changes.

Status labels must not be collapsed:
source-written != built != automated-test-pass != visual QA pass
!= active dynamic campaign != saved v2 data != deployed Android.

## Next original milestone

Author an explicit versioned map-save contract with stable site IDs,
seed, topology hash, ownership, guard state and exploration states.
Implement a V1->V2 conversion in a scratch/test namespace with readback
before any user-facing migration. Then connect route previews, player
confirmation, state restoration and PCG assets. Keep the old five-site
campaign load path backward compatible. Do not change active player
save slots during migration experiments.
