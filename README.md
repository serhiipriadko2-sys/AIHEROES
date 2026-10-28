# AI HEROES — evolving AI commander

Original 3D turn-based strategy for Windows and future Android builds.
The PLAYER manages recruitment, resources, movement and attack authorization.
The HERO AI fights each battle autonomously through a deterministic multi-round
initiative and target-selection policy. This is not a running language model.

## Run in VS Code

Open C:\github\AIHEROES in VS Code, then use Terminal > Run Task:
- AIHEROES: Build Unreal Editor
- AIHEROES: Test all gameplay automation (checks the fresh JSON report, not exit code alone)
- AIHEROES: Launch playable Windows game
- AIHEROES: Visual live grid V2 smoke (scripted real-window QA; no save writes)
Close the game window before rebuilding so the Unreal DLL can be replaced.

Or open AIHeroes.uproject in Unreal Engine 5.8 and press Play.

## PC controls — active AIHeroesGameMode

Confirmed attacks now enter **live Grid V2**, not precomputed replay.
In battle: Q/E orbit, R/F zoom, Space/Esc pause, T speed 1x/2x/4x,
B fast finish, Enter return after the result. Strategic actions and save/load
are blocked until return. See [live battle rules and evidence](Docs/LIVE_GRID_V2.md).
I opens/closes the full read-only adviser dossier; PgUp/PgDn browse pages,
Esc closes it. Close the report before giving strategic orders.

- W/A/S/D = camera pan, Q/E = rotate perspective camera, R/F = camera elevation.
- Click a node = SELECT only; selection never begins a battle.
- H = ask the hero for advice about the selected node, or get a safe-target suggestion without a selection.
- Enter = CONFIRM movement and any battle at that node.
- 1 = recruit 5 Militia, 2 = recruit 2 Scouts.
- U / HQ + = upgrade the selected owned headquarters while the hero is there; the displayed price is charged once.
- P = unlock next cognitive protocol (resource costs).
- M = unlock the next 3B/8B model SLOT after earning DATA in battles.
- C = choose one of three independent local campaign slots (1 -> 2 -> 3 -> 1).
- K = save the selected slot; L = load it. Each slot rotates A/B verified saves. F5 is reserved by Unreal for a shader-complexity view mode.
- J = step through the last battle's validated tactical transcript.
- V = show the last verified battle as a 3D tactical replay; Esc or Enter skips it.
- F9 is intentionally unused (Unreal viewport diagnostics).
- Space = next strategic day: restores movement and pays owned-mine income.

The bottom HUD has seven strategic-action buttons; SAVE, LOAD, SLOT, text REPLAY and 3D REPLAY controls appear at the top right.
Touch taps work through the same controller path, but Android device QA is pending.
Each day grants 4 movement points; distant targets cost more moves.
An attack cannot start without a living army or enough movement.

## Campaign objectives and victory

The prototype now has a real **campaign completion condition**, not just an
endless resource loop. **O** or the touch **GOALS** button opens a read-only
mission dossier; a progress strip remains visible on the map. Complete all
four goals: capture both supply mines, liberate the Outpost and Ruins, unlock
all 3 offline AI protocols, and upgrade to the **8B-themed game slot**.
The slot is not a real LLM; no checkpoint has been installed.

When all goals are satisfied, the HUD says **CAMPAIGN VICTORY**. This does not
automatically spend resources, close the game, or save without permission.
Press K to save the victory, or continue playing. Old SaveGames work because
objective progress is derived from previously saved fields.
See [mission contract](Docs/CAMPAIGN_OBJECTIVES_V1.md).

## Headquarters economy

Select HQ and bring the hero there, then use U or HQ +. Construction is blocked
while a live battle, replay or adviser dossier is open. Levels 0/1/2/3 pay
40/60/80/100 gold on the next day; an owned outpost adds its usual 30.
Upgrade costs (gold/wood/ore): 100/10/10, 180/20/15, 300/35/25.
Prices appear in the HQ selection/advice line. Purchases do not award instant
income or alter combat protocols/model slots. Towers appear as the base grows.
The level is saved and validated; old saves without the field start at level 0.
These initial prices are a balance hypothesis, not playtest-validated tuning.
See Docs/BASE_DEVELOPMENT.md for the exact verification boundary.

## Strategic opposition and HotA research

The original OUTPOST raider faction makes the campaign react to player turns.
Only the human player's SPACE / NEXT DAY command allows the NPC to act.
Uncaptured OUTPOST raises bounded reinforcements on week transitions and
may raid a player-owned Forest or Ore mine on a scheduled day. A readable
**next-rival-turn warning** appears on the adventure map; capturing OUTPOST
ends raiding. NPC actions are deterministic, have preflight bounds and are
applied with the weekly neutral-growth phase, without issuing orders to the
player hero or changing the SaveGame v1 schema.

Read [Rival Outpost rules](Docs/RIVAL_FACTION_V1.md) for the mechanics and
[the system-level study of Heroes III: Horn of the Abyss](Docs/HOTA_GAMEPLAY_RESEARCH_2026-10-28.md)
for the original reference, mapping to AI HEROES and prioritized future
features. AI HEROES does **not** import HotA art, code, campaigns, characters,
unit names or game assets. This is not a clone of the HotA factions.

## Hero evolution — independent tracks

1. Army strength and economic progression: militia, scouts, resources, nodes.
2. Cognitive protocols: seed -> threat analysis -> memory ledger -> tactical planner.
3. Model slots shown as 1B/3B/8B themes: NO REAL MODEL WEIGHTS LOADED.

The multi-round battle simulator is deterministic. Progress includes the army,
owned nodes, movement, data fragments, protocol and model-slot tier. Three
independent human save slots each keep alternating A/B recovery snapshots;
a failed or damaged newest file can fall back to a validated prior snapshot.
Slot 1 also loads the original single-slot save without overwriting it.
A bounded tactical record stores the last battle's starting units, policy tier,
rounds and action trace. The engine re-computes this record before playback.
These are GAME progress and battle logs, not AI memory, training, file
ingestion or actual neural-network checkpoint replacement.

## Evidence and boundaries

- Active GameMode: Source/AIHeroes/Private/AIHeroesGameMode.cpp.
- Active live combat AI: Source/AIHeroes/Private/AIHeroesGridBattle.cpp.
- Legacy V1 record/replay rules: Source/AIHeroes/Private/AIHeroesBattleEngine.cpp.
- Active UI: Source/AIHeroes/Private/AIHeroesHUD.cpp.
- Old AIH* prototype classes remain for comparison, NOT the active mode.
- Design: Docs/AI_EVOLUTION_DESIGN.md.
- Local build/test receipts: Docs/BUILD_RECEIPT_*.md.
- Window-targeted playback QA: Tools/qa_playthrough_game.ps1.
- 3D replay window QA: Tools/qa_3d_replay.ps1 (separate test log, no global input).
- 3D replay implementation: Source/AIHeroes/Private/AIHeroesBattleStage.cpp.
- Deterministic typed actions and integrity gate: AIHeroesBattleEngine.cpp.
- 3D playthrough evidence: Docs/AIHEROES_PLAYTHROUGH_PASS_2026-10-28.png.

The 3D world still uses prototype geometry. Confirmed attacks run a live
8x6-grid session: the hero chooses movement/attacks during play, and the
campaign commits casualties/cost/rewards once after completion. Perspective
camera, HP/initiative, pause/speed/fast-finish are implemented. Historical V1
3D replay stays separate; V2 transcripts are readable with J, but the old V
3D replay viewer remains V1-only. Author-created character assets, animation
polish and manual commands to units are not part of this slice.
Android SDK is not validated, no Android APK or live 1B/3B/8B checkpoint
has been tested. No assets or game concepts from Heroes III are copied.
