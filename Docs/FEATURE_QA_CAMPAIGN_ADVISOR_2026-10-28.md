# AI HEROES · Campaign persistence + strategic advisor · Local QA

Date: 2026-10-28
Status: **IMPLEMENTED / PC-TESTED-LOCALLY**
Surface: Windows machine DESKTOP-ITUAMNF, C:\github\AIHEROES
Engine: Unreal Engine 5.8.4, AIHeroesEditor Win64 Development
Git: local worktree, including concurrent edits; no commit, push, release, merge or deployment performed by this session.

## Scope

- Implemented a versioned Unreal SaveGame for campaign progress: Day, Gold, Wood, Ore, MovesLeft, ProtocolTier, ModelSlotTier, DataFragments, hero's node ID and selected node ID, army HP/stats, node ownership/defenders, bounded battle journal and last tactic.
- Added fail-closed validation for schema, bounds, known node/unit IDs, units stats, duplicates and invalid hero location before applying saved state. Actor references are preflighted before game-state writes.
- Added deterministic typed read-only adviser: target-specific forecasts (rounds, survivors, HP losses, movement), optional resource-based suggestions and alternative, with player confirmation still required for attacks.
- Added keyboard F5 SAVE and L LOAD; HUD also provides SAVE/LOAD buttons. F9 was deliberately removed because Unreal bound it to shader-complexity diagnostics.
- Added four automation tests (2 save/restore, 2 adviser) alongside seven existing ones. Tested remote Windows PC in game, not Android.

## Quality gates

| Gate | Result | Read-back evidence |
|---|---|---|
| Pinned Iskra SoT30 integrity | PASS | v5.5.7, knowledge 30/30, sha256sums 32/32, corpus 4054425 bytes |
| Unreal C++ Editor build after F9-to-L fix | PASS | UnrealBuildTool: Result: Succeeded |
| Unreal Automation final group | **PASS 11/11** | Last run: 11 tests, each Result={Success}; process exit code 0 |
| Advisor global strategic suggestion | PASS | AIHeroes.Advisor.GlobalStrategicChoice |
| Advisor progression/forecast | PASS | AIHeroes.Advisor.ProgressionAndForecast |
| SaveGame disk round-trip | PASS | AIHeroes.Campaign.DiskRoundTrip |
| Save snapshot invalid-data rejection | PASS | AIHeroes.Campaign.Validation |
| Other existing combat/turn/unit tests | PASS 7/7 | AIHeroes.Core.* and AIHeroes.Simulation.CombatPolicy |
| Manual in-game F5 SAVE | PASS | Log: GAME SAVED: day 2, 5 nodes, protocol tier 1 |
| Manual F9 LOAD before hotkey correction | PASS for persistence, UI conflict found | Log: GAME LOADED; shader complexity debug overlay was unintentionally activated |
| Manual **L LOAD after hotkey correction** | **PASS** | New game process, Log: GAME LOADED: day 2, 5 nodes, protocol tier 1 |
| Normal 3D rendering after L | PASS visual smoke | Docs/CAMPAIGN_LOAD_VERIFIED_2026-10-28.png |
| Android APK | BLOCKED | SDK API 35 platform absent and NDK r27c absent |
| Real LLM 1B/3B/8B inference | NOT IMPLEMENTED | The slots are explicitly themed upgrade tiers, no checkpoint loaded |

### Exact automatic-test names (11)

1. AIHeroes.Advisor.GlobalStrategicChoice
2. AIHeroes.Advisor.ProgressionAndForecast
3. AIHeroes.Campaign.DiskRoundTrip
4. AIHeroes.Campaign.Validation
5. AIHeroes.Core.AtomicResourceSpend
6. AIHeroes.Core.DeterministicBattle
7. AIHeroes.Core.ModelSlotPurchase
8. AIHeroes.Core.NoArmyCannotWin
9. AIHeroes.Core.ProtocolChangesTactics
10. AIHeroes.Core.TravelBudget
11. AIHeroes.Simulation.CombatPolicy

## Confirmed local artifacts

- User campaign: `Saved/SaveGames/AIHEROES_Campaign_v1.sav`, **5915 bytes**, SHA256 `B8F625FB811C83CBA8F45DB97DDF2628FEEF0AF7245CFC2EBB460FF00048000B` (snapshot created during runtime QA).
- Screenshot **after safe L load**: `Docs/CAMPAIGN_LOAD_VERIFIED_2026-10-28.png`, **377058 bytes**, SHA256 `364948EABDD1A651B8D2902848909C9F212B63EC951844B7432B94BDD2D1FB9F`.
- Read-only Android SDK audit: `Tools/check_android_readiness.ps1` and `Docs/ANDROID_READINESS_2026-10-28.md`.
- Real Unreal runtime log: `Saved/Logs/AIHeroes.log`. Game screenshot verifies DAY 2, owned Forest and selected Ore after L restore.

## Known boundaries and follow-ups

- Local SaveGame is a gameplay save, **not Iskra memory**, model fine-tuning or an evidence-backed SoT change. No DB/memory writes were performed.
- A single fixed SaveGame slot means manual saves can overwrite their previous save in normal play. No automatic backup/rollback or multi-slot UI yet. Automation uses only unique temporary test slots and cleans them up.
- Full 3D tactical animations, buildings/terrain assets from Blender, enemy factions, long campaigns and phone optimization are later milestones.
- Unreal/Windows successful builds do **not** prove Android build. NDK r27c and SDK Platform 35 must be validated before any APK claim.
- A true model inference adapter must be separately implemented with model provenance, memory/latency benchmarks, constrained typed actions and fallback. It is not present here.
- Concurrent code changes were observed from another local process. The worktree was not force-cleaned, committed or pushed.
- The test runner displayed generic Unreal startup test warnings, but all eleven named AIHeroes tests individually reported Success and runner exit was 0.
- Next reversible step: add unit action trace/replay and save-slot backup, then confirm Android SDK preflight and AI adviser UX in a fresh manual playthrough.

## Verification reproduction

In VS Code open `C:\github\AIHEROES`.
- Editor build: `C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat` target `AIHeroesEditor Win64 Development` with project argument pointing to `AIHeroes.uproject`.
- Tests: `UnrealEditor-Cmd.exe` with `-ExecCmds=Automation RunTests AIHeroes` and `-TestExit=Automation Test Queue Empty`.
- Launch `AIHeroes.uproject` in game mode and use F5 to SAVE / L to LOAD. Avoid F9.
- Android preflight: `Tools/check_android_readiness.ps1` (read-only).

Final QA boundary: **PASS PC feature implementation and automated testing. PARTIAL complete PC/Android strategy game.**
