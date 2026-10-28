# AI HEROES — evolving AI commander

Original 3D turn-based strategy for Windows and future Android builds.
The PLAYER manages recruitment, resources, movement and attack authorization.
The HERO AI fights each battle autonomously through a deterministic multi-round
initiative and target-selection policy. This is not a running language model.

## Run in VS Code

Open C:\github\AIHEROES in VS Code, then use Terminal > Run Task:
- AI HEROES: Build Editor (UE 5.8)
- AI HEROES: Run 4 battle tests (the name predates two extra strategy tests)
- AI HEROES: Launch PC prototype

Or open AIHeroes.uproject in Unreal Engine 5.8 and press Play.

## PC controls — active AIHeroesGameMode

- W/A/S/D = camera pan, Q/E = rotate perspective camera, R/F = camera elevation.
- Click a node = SELECT only; selection never begins a battle.
- H = ask the hero for advice on the selected node.
- Enter = CONFIRM movement and any battle at that node.
- 1 = recruit 5 Militia, 2 = recruit 2 Scouts.
- P = unlock next cognitive protocol (resource costs).
- Space = next strategic day: restores movement and pays owned-mine income.

The bottom HUD includes clickable buttons for the same six strategic actions.
Touch handling is implemented for taps, but Android build/device QA is pending.
Each day grants 4 movement points; distant targets cost more moves.
An attack cannot start without a living army or enough movement.

## Hero evolution — independent tracks

1. Army strength and economic progression: militia, scouts, resources, nodes.
2. Cognitive protocols: seed -> threat analysis -> memory ledger -> tactical planner.
3. Model slots shown as 1B/3B/8B themes: NO REAL MODEL WEIGHTS LOADED.

The battle simulation is deterministic and saves a short session-only
combat journal and tactical trace. Actual file memory, on-device inference
and model checkpoint replacement are NOT implemented yet.

## Evidence and boundaries

- Active GameMode: Source/AIHeroes/Private/AIHeroesGameMode.cpp.
- Active combat AI: Source/AIHeroes/Private/AIHeroesBattleEngine.cpp.
- Active UI: Source/AIHeroes/Private/AIHeroesHUD.cpp.
- Old AIH* prototype classes remain for comparison, NOT the active mode.
- Design: Docs/AI_EVOLUTION_DESIGN.md.
- Local build/test receipts: Docs/BUILD_RECEIPT_*.md.

The 3D world still uses placeholder cubes/spheres. Battles are simulated
with logged rounds; animated tactical 3D battlefields are a later milestone.
Android SDK is not validated, no Android APK or live 1B/3B/8B checkpoint
has been tested. No assets or game concepts from Heroes III are copied.
