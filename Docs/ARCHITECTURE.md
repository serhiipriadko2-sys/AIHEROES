# AIHEROES architecture v0.1

## Current active game mode (verified 2026-10-28)
Config/DefaultEngine.ini selects /Script/AIHeroes.AIHeroesGameMode.
Its native game logic lives under Source/AIHeroes/Public and Private.
The AIH* classes described below are a second, experimental implementation:
they compile but are NOT selected at startup. A future integration pass
must consolidate both implementations without dropping working features.
The active PC game currently uses keyboard controls, not the alternative
Slate button panel. Android input has not been acceptance-tested.

## Experimental AIH responsibilities
- AIHGameMode: authoritative turn economy, resource ownership, recruitment,
  AI progression, node attacks and player decisions.
- AIHWorldNode: visible interactive 3D nodes (base, mines, data, neutral, rival).
- AIHCameraPawn: perspective PC camera, pan, rotate, zoom.
- AIHPlayerController: native Slate interface and mouse/touch node selection.
- AIHSimulation: deterministic hero combat policy, independently testable.
- AIHSimulationTests: Unreal automation test for battle safety invariants.

## The future *real model* boundary
HeroAdviceRequest (versioned game snapshot, permitted choices)
 -> optional IHeroInferenceAdapter (1B/3B/8B actual model checkpoint)
 -> JSON tactical proposal with schema and strict action whitelist
 -> validate permissions, resources, fog-of-war and AP
 -> deterministic game engine executes a validated order
 -> result + battle log are displayed to player.

The model does not get raw file access or arbitrary UE console commands.
TRACE/SIFT/PLANNING/GUARD/COUNCIL are in-game unlocked mechanisms,
not proof of running the original Iskra SoT30 service.
No AI model is currently loaded and no network call is made.

## Platform strategy
Shared Unreal C++ game core targets Windows and Android.
PC can later select an Ollama or llama.cpp-backed adapter.
Android can use a small quantized on-device model on supported devices,
or a consented remote inference service; offline rule fallback is mandatory.
Performance budgets and packaging are NOT verified yet.

## Milestones
1. Compile, launch Editor, run AIHeroes.Simulation.CombatPolicy.
2. Make camera, buttons and resource capture work in Play In Editor.
3. Implement route planning, turn ending and opponent turns.
4. Implement real schema-constrained inference (initial 1B checkpoint).
5. Optimize UI, Android touch/power/memory and package native builds.
6. Add original Blender assets, VFX, larger 3D world and save/load.

## Non-goals of first slice
No multiplayer, no fog-of-war, no persistent saves or real 1B->8B model swap.
Windows Editor compilation and four automated battle tests PASSED on 2026-10-28;
the PC game process reached GameMode BeginPlay. A manual visual interaction
playthrough and native Android APK are still NOT verified. See BUILD_RECEIPT_ISKRA_2026-10-28.md.
