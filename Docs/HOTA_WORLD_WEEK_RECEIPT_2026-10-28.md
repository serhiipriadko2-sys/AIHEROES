# AI HEROES — Horn of the Abyss research + World Week integration receipt
Date: 2026-10-28.
Workspace: C:\github\AIHEROES / DESKTOP-ITUAMNF (Windows, Unreal Engine 5.8).
Status: RESEARCH WRITTEN / BUILT / AUTOMATED TESTS PASS / WINDOW QA PASS.
Scope: an original gameplay mechanic inspired by the strategic day/week
rhythm, not a 1:1 reproduction of HotA or a claim to be its playable port.
No commit, push, merge, deployed Android app or real model inference.

## Source and scope

Research file:
Docs/HOTA_181_GAMEPLAY_RESEARCH_2026-10-28.md (318 lines)
Fifteen sections: released version, adventure/world, town economy, seven
resources, creatures, hero RPG, spells/research, tactical combat, siege,
quests/rewards, random map zone templates, PvP/balance, extended events,
comparison to AI HEROES, prioritized independent backlog, IP boundaries,
source references. Facts are marked [FACT]; new design ideas [INTERP] or
[PROPOSAL], and links to primary documentation are included.

Latest version verified via official h3hota.com/en/download and the HotA
crew's announcement at https://www.nexusmods.com/heroesofmightandmagiciii/articles/13:
HotA 1.8.1, 25 Aug 2026. The indexed h3hota.com download page can return
a stale cached 1.8.0 version, so the developer release post is a separate
primary confirmation. Other original sources:
https://h3hota.com/en/documentation
https://h3hota.com/en/template-format
https://h3hota.com/en/templates
https://h3hota.com/en/rules

No HotA installer, protected music, sprites, maps, dialogs, character art,
town logos, textures, unit stats tables or source code were copied into
AI HEROES.

## Implemented original feature

Name: Seven-Day Frontier Pressure, WorldWeek v1.

1) Days 1..7: regular income / movement as before; no weekly guard growth.
2) Day 7 -> 8 (and 14 -> 15, etc): living NEUTRAL guards on uncleared
   sites get small deterministic unit additions:
   FOREST +2 wolves, ORE +2 bandits, OUTPOST +1 raider for each formation,
   RUINS +1 guardian. For the initial world: total +7.
3) Captured sites and already-dead stacks never respawn.
4) Per-stack ceiling 40 living units and MaxUnitHealth 500000; overflowing
   future state rejects the full day advance before any modification.
5) PlanNextDay works only on a verified snapshot with exactly five named
   node types and correct defenders. No file system, AI/LLM or actors inside
   the calculation. The authoritative GameMode owns the final commit.
6) Concurrent NPC rival's day-8 recruitment is computed against the
   *post-week* OUTPOST snapshot, then the entire strategic day is committed
   once. No hidden combat or player action.
7) The event HUD reports [W2 D1/7] and NEW WEEK: neutral guards +7 units;
   FOREST label guard count changes 4 -> 6.
8) SaveGame schema remains unchanged: the original Day and
   NodeStates[].Defenders[].TotalHealth already preserve the growth.

Implementation:
 Source/AIHeroes/Public/AIHeroesWorldWeek.h
 Source/AIHeroes/Private/AIHeroesWorldWeek.cpp
 Source/AIHeroes/Private/AIHeroesGameMode.cpp EndTurn integration
 Source/AIHeroes/Private/Tests/AIHeroesWorldWeekTests.cpp
 Docs/WORLD_WEEK_GAMEPLAY_CONTRACT_2026-10-28.md
 Tools/qa_world_week.ps1

The original GameMode was hash-backed up in
Saved/IskraWeekBackups_20261028/AIHeroesGameMode.before.cpp
(25258 bytes,
SHA256 7739f7abf5f4aad99b8cb8d0914a3889db195a81a9e92921d81c0ad86d6b285c).
Other developers wrote to shared source concurrently: RivalStrategy,
AdventurePath, Base and ProtocolLab belong to their authors. This receipt
claims only the specifically named WorldWeek files and their integration.

## Test receipt

Initial Unreal UBT attempt compiled WorldWeek and WorldWeekTests and
GameMode, but reported three errors in unrelated concurrent modules
AIHeroesHUDLayout and AIHeroesRivalStrategy. A concurrent contributor
fixed those errors in the shared tree. No destructive repair/reset was
performed by this operator.

Subsequent UBT Win64 Development build returned:
 Target is up to date; Result: Succeeded
with output built DLL matching the actual integration snapshot.

Full Unreal Automation Test script:
 & C:\github\AIHEROES\Tools\run_aiheroes_automation.ps1
Result is a fresh JSON report, NOT merely Unreal process exit code:
 Docs/AutomationRuns/20261028T203357532Z/index.json
 98 tests, 98 Success, 0 Failed, 0 Warnings,
 0 notRun, 0 inProcess.

In particular all four new WorldWeek cases PASSED:
 - AIHeroes.WorldWeek.CalendarBoundaries
 - AIHeroes.WorldWeek.GuardGrowthAndNoResurrection
 - AIHeroes.WorldWeek.AtomicFailureAndCap
 - AIHeroes.WorldWeek.SaveSchemaCompatibility

The four Rival tests also PASSED, checking day-8 recruits, bounded raids,
authority and existing saved data compatibility.

SOT30 integrity:
SOT30_INTEGRITY_PASS version=v5.5.7 knowledge=30/30
sha256sums=32/32 corpus_bytes=4054425 members=33/33
pinned_manifest_sha256=
0decd118a6300324b757d2587ec315a569aaf5800299d2b33bf046feaf56aeff

## Actual Windows game-window QA

An isolated UnrealEditor.exe was launched as -game -windowed at 1280x720
with a private log AIHeroesWorldWeekQA.log (own PID 38252).
Tools/qa_world_week.ps1 -TargetProcessId 38252 checked the exact PID,
its -game commandline, the private log, viewport dimensions, and sent
only window-specific Win32 messages. No global keyboard/mouse inputs.

The script selected FOREST, captured day 1, clicked NEXT DAY seven times.
Logs showed each day 2..8, income of 40 GOLD per day, and at day 8:
 [AI HEROES WEEK] week=2 day=8 recruits=7
 Day 8 ... [W2 D1/7] NEW WEEK: neutral guards +7 ...
 RIVAL RECRUIT: Day 8. OUTPOST reinforced (+22 guard HP) ...

No game save or live battle commit was logged.
Final: WORLD_WEEK_EXACT_WINDOW_QA_PASS (exit code 0).
Two screenshots were reviewed visually. On day 1 FOREST has 4 guards;
on day 8 the same FOREST has 6, while player army is unchanged and
resources reflect seven days of daily income. Photos captured only the
game window using PrintWindow, not desktop/other windows.
The own QA process was closed gracefully, not force-killed.

## Artifact byte/sha256 manifest (read-back snapshot)

| Project-relative path | Bytes | SHA256 |
|---|---:|---|
| Docs/HOTA_181_GAMEPLAY_RESEARCH_2026-10-28.md | 28255 | 18aaa4cf6854fbad7201f2160586829bf4569058eba6e1e4dd716af1c62c6c1f |
| Docs/WORLD_WEEK_GAMEPLAY_CONTRACT_2026-10-28.md | 4364 | b5a0187cb0ccf763a01a72a06fdd7d6df3df8dbe249c48074ecc62243473d5ed |
| Source/AIHeroes/Public/AIHeroesWorldWeek.h | 1552 | 743bd015ae119cf626cb44187ecc1c86184d406f1de12c2d8bc79da63accbd7b |
| Source/AIHeroes/Private/AIHeroesWorldWeek.cpp | 5416 | 953cc44036b4cbcfd7cafbca73c304dcb74c98acab41f41a8ae44164e0772c72 |
| Source/AIHeroes/Private/Tests/AIHeroesWorldWeekTests.cpp | 9359 | 426bc340459008d42edd6727f2dc56f7eb8222e0e22f1f9c60d57e3c9e361095 |
| Source/AIHeroes/Private/AIHeroesGameMode.cpp | 28832 | 2f05a33083d7733c5f2ae7fa0a77373eb2b06ef6ba682bfb00f3bf0f92bd8a63 |
| Tools/qa_world_week.ps1 | 3774 | f1a13bc1cf22ad8aaf8031d73863ebb438d477aafaf80a18612eae49614d38d9 |
| Docs/AIHEROES_WORLD_WEEK_DAY1_2026-10-28.png | 356077 | caeb631fb7bfc955ec1fca7e3b5d731a2e182c543be79167675732d631059a9c |
| Docs/AIHEROES_WORLD_WEEK_DAY8_2026-10-28.png | 359047 | e3f46de342c6a3035e02f78d0306c0c9dace4de91f31b0a8d6334658dcfda6d9 |
| Binaries/Win64/UnrealEditor-AIHeroes.dll | 1264640 | 4f7156f2309fafa6ba3b1fe2ed865b5616d82bec35f9b77de7f9f3d87fd1d669 |
| Docs/AutomationRuns/20261028T203357532Z/index.json | 38670 | 76820cc06257fb6e2bd521e81fc648115f7982903b7fa710c5e0e63d6caa9744 |

Hash matching proves the concrete bytes present at readback, not
ongoing identity if another author changes the shared working tree.
The artifacts are on the owner's Windows computer, not downloadable
sandbox files in this conversation.

## Remaining gaps and next original milestone

The studied HotA design is far richer than AI HEROES: 12 towns,
broad random zone templates, seven resources, spell guilds, heroes/
artifacts, sieges, creature trees, PvP and event scripting.
This research is a structured systems overview and source map, NOT a
line-by-line exhaustive catalog of all spells and unit values.

Recommended next original slice: graph-based 3D adventure map generator
with a reproducible seed and verified objective reachability, paired with
a weekly recruitment pool in the hero's HQ. Before expanding five fixed
map nodes, design and validate a new SaveGame schema and backward
compatibility/migration, never silently re-interpret the existing saves.

No Android APK, actual 1B/3B/8B model weights, network multiplayer,
production release or copyrighted HotA content were created.
No git commit, push, merge or deployment were performed here.
User save slots were not overwritten during the QA.

PASS: SOT30, built current Win64 DLL, fresh full 98/98 tests,
visible real-window weekly cycle and screenshots, artifact hashes.
PARTIAL: full genre feature parity and Android support remain future work.

∆DΩΛ:
∆: researched the 1.8.1 game systems and implemented an original week loop.
D: primary-source web evidence -> repo code -> compile -> full automated
   tests -> exact-window day1/day8 test -> verified screenshots and SHA.
Ω: 95% for this local game slice; no claims of full HotA parity.
Λ: revise after future rebalancing, source drift or a verified new map
   generator and backward-compatible save migration.
