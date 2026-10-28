# AI HEROES — evolving AI commander

Original 3D turn-based strategy for Windows and future Android builds.
The PLAYER manages recruitment, resources, movement and attack authorization.
The HERO AI fights each battle autonomously through a deterministic multi-round
initiative and target-selection policy. This is not a running language model.

## Run in VS Code

Open C:\github\AIHEROES in VS Code, then use Terminal > Run Task:
- AIHEROES: Build Unreal Editor
- AIHEROES: Test autonomous battles (runs the full AIHeroes test suite)
- AIHEROES: Launch playable Windows game
Close the game window before rebuilding so the Unreal DLL can be replaced.

Or open AIHeroes.uproject in Unreal Engine 5.8 and press Play.

## PC controls — active AIHeroesGameMode

- W/A/S/D = camera pan, Q/E = rotate perspective camera, R/F = camera elevation.
- Click a node = SELECT only; selection never begins a battle.
- H = ask the hero for advice about the selected node, or get a safe-target suggestion without a selection.
- Enter = CONFIRM movement and any battle at that node.
- 1 = recruit 5 Militia, 2 = recruit 2 Scouts.
- P = unlock next cognitive protocol (resource costs).
- M = unlock the next 3B/8B model SLOT after earning DATA in battles.
- F5 = save the local campaign; L = load it. F9 is intentionally unused (it switches Unreal viewport diagnostics).
- Space = next strategic day: restores movement and pays owned-mine income.

The bottom HUD has seven strategic-action buttons plus SAVE/LOAD in the top right.
Touch taps work through the same controller path, but Android device QA is pending.
Each day grants 4 movement points; distant targets cost more moves.
An attack cannot start without a living army or enough movement.

## Hero evolution — independent tracks

1. Army strength and economic progression: militia, scouts, resources, nodes.
2. Cognitive protocols: seed -> threat analysis -> memory ledger -> tactical planner.
3. Model slots shown as 1B/3B/8B themes: NO REAL MODEL WEIGHTS LOADED.

The multi-round battle simulator is deterministic. A bounded battle journal,
the army, owned nodes, movement, data fragments, protocol and model-slot tier
can now be saved to a local Unreal SaveGame slot (F5) and restored (F9).
This is GAME PROGRESS persistence, not AI memory, training, file ingestion
or actual neural-network checkpoint replacement.

## Evidence and boundaries

- Active GameMode: Source/AIHeroes/Private/AIHeroesGameMode.cpp.
- Active combat AI: Source/AIHeroes/Private/AIHeroesBattleEngine.cpp.
- Active UI: Source/AIHeroes/Private/AIHeroesHUD.cpp.
- Old AIH* prototype classes remain for comparison, NOT the active mode.
- Design: Docs/AI_EVOLUTION_DESIGN.md.
- Local build/test receipts: Docs/BUILD_RECEIPT_*.md.
- Window-targeted playback QA: Tools/qa_playthrough_game.ps1.
- 3D playthrough evidence: Docs/AIHEROES_PLAYTHROUGH_PASS_2026-10-28.png.

The 3D world still uses placeholder cubes/spheres. Battles are simulated
with logged rounds; animated tactical 3D battlefields are a later milestone.
Android SDK is not validated, no Android APK or live 1B/3B/8B checkpoint
has been tested. No assets or game concepts from Heroes III are copied.
