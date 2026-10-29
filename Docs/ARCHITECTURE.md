# AIHEROES architecture — active surfaces and prototype history

## Current source read-back (2026-10-29)
Config/DefaultEngine.ini selects /Script/AIHeroes.AIHeroesGameMode.
Its native game logic lives under Source/AIHeroes/Public and Private.
The AIH* classes described below are a second, experimental implementation:
they are NOT selected at startup. Do not remove either implementation or
silently replace the default mode to make an optional feature appear integrated.

The default five-site campaign and the optional procedural ZoneExpedition are
distinct surfaces. Source presence does not prove that one surface has inherited
the other surface's saves, UI, adviser, model adapter or visual battle lifecycle.

| Surface | Implementation read-back | Boundary |
|---|---|---|
| Campaign | AIHeroesGameMode, BaseRules, RecruitmentStock, WorldWeek, RivalStrategy | Player owns strategic decisions; current source has base, stock and rival systems |
| Exploration and logistics | FogRules, TravelRules, RoutePlanner, FieldDwelling, SupplyLines | Known-site filtering and confirmed movement, not physical landscape LOS |
| Live combat | GridBattle, LiveBattleGameMode, LiveBattleStage | V2 Begin/Step/RunToEnd, atomic completion; presentation does not choose attacks |
| Historical battles | BattleRecord, BattleEngine, GridReplay, BattleReplayGameMode | V1 and V2 retain different rules; verified playback does not award campaign rewards |
| Adviser and comparison | Advisor, AdvisorDossier, RecruitmentAdvisor, ProtocolLab | Read-only recommendations and counterfactuals, not automatic strategic execution |
| Campaign persistence | CampaignSave, SaveSystem, LoadPreflight | Existing validated slots/read-back; no unfinished-live-battle persistence |
| Procedural expedition | ZoneTemplate, ZoneExpeditionRules/GameMode/Store, ZoneJourney | Separate generated world, state and known-only planning; not the configured default |
| Presentation | HUD/PlayerController, BattleHUD, ExplorerHUD, BattleArt, SceneryV3 | Multiple evolving views; PC captures do not establish physical Android usability |

Filenames in the table are relative to `Source/AIHeroes/Public` and `Private`.
These are source observations, not blanket runtime acceptance. Use the matching
compiled manifest, full automation provenance and scenario-specific visual proof
for a frozen build. See [reproducible QA](../README.md#reproducible-local-qa).

## Experimental AIH responsibilities
- AIHGameMode: authoritative turn economy, resource ownership, recruitment,
  AI progression, node attacks and player decisions.
- AIHWorldNode: visible interactive 3D nodes (base, mines, data, neutral, rival).
- AIHCameraPawn: perspective PC camera, pan, rotate, zoom.
- AIHPlayerController: native Slate interface and mouse/touch node selection.
- AIHSimulation: deterministic hero combat policy, independently testable.
- AIHSimulationTests: Unreal automation test for battle safety invariants.

## Implemented contract versus future real inference

The main campaign now has `AIHeroesModelRequest`, `AIHeroesModelReply`,
`AIHeroesAIProposalGate` and `AIHeroesLocalExchange`. Read their individual
[request](MODEL_REQUEST_EXPORT_V1.md), [reply](MODEL_REPLY_GATE_V1.md) and
[loopback exchange](LOCAL_MODEL_ASYNC_EXCHANGE_V1.md) contracts before changes.

Current contract: bounded fog-filtered snapshot and legal candidate menu
 -> optional explicitly opted-in nonblocking loopback exchange
 -> bounded reply and correlation checks
 -> current-state/readiness validation.

The mock-tested transport is not checkpoint attestation, model loading or proof
of useful advice. Production model readiness remains false. Normal gameplay does
not start that transport; an explicit opt-in is required. Thus the old blanket
claim "no network code exists" would be incorrect, while "real 1B/3B/8B inference
is verified" would also be incorrect.

Future adapter acceptance additionally needs a real identified checkpoint,
server compatibility, useful safe output, measured device budgets and explicit
UI consent. Advice and a legal candidate are not authorization to spend campaign
resources; player confirmation and engine validation remain separate.

The contract does not grant raw file access or arbitrary UE console commands.
Protocol tiers are game mechanics, not evidence of model weight growth or of
invoking a live external Iskra service. The private canonical snapshot is not
game content and must not be bundled with a public build.

## Platform strategy
Shared Unreal C++ game core targets Windows and Android.
PC can later select an Ollama or llama.cpp-backed adapter.
Android can use a small quantized on-device model on supported devices,
or a consented remote inference service; offline rule fallback is mandatory.
Physical Android, real inference and final packaging remain separate gates.
Editor DLL, Game executable, cooking/staging, signed distribution and device
acceptance must not be collapsed into one "build passed" status. See
[Game parity gate](WIN64_GAME_BUILD_PARITY_GATE_2026-10-29.md); its dated status
must itself be checked against the newest build artifacts before reuse.

## Historical v0.1 milestones — not the current backlog
1. Compile, launch Editor, run AIHeroes.Simulation.CombatPolicy.
2. Make camera, buttons and resource capture work in Play In Editor.
3. Implement route planning, turn ending and opponent turns.
4. Implement real schema-constrained inference (initial 1B checkpoint).
5. Optimize UI, Android touch/power/memory and package native builds.
6. Add original Blender assets, VFX, larger 3D world and save/load.

## Historical first-slice evidence and current research

The earliest slice omitted multiplayer, fog and persistence. It reported four
battle tests and BeginPlay on 2026-10-28 in BUILD_RECEIPT_ISKRA_2026-10-28.md.
That is historical evidence, not today's test count or a claim that saves and
scouting are still absent. A genuine 1B->8B checkpoint transition is not proven.

For the current reference study and proposed priorities, see
[HotA deep research](HOTA_DEEP_RESEARCH_2026-10-29.md). Its proposals do not
modify V1/V2 rules, save schemas or the configured GameMode.

Scope of this documentation correction: capability map and stale-status labels
only. Rollback its own text hunks, not source, user saves or concurrent art work.
