# AI HEROES: verified Known-Only Journey v1 integration

2026-10-29. Local workstation: DESKTOP-ITUAMNF.
Observed Git HEAD: d0513f2 (no commit/push by this operator).

## Lifecycle labels

PASS: offline source implementation and I hotkey, read-only legal
multi-day itinerary over player-visible graph, deterministic
stage/rest planning, Editor compile, complete 245/245 Automation
suite, two real confirmed battles with I-preview, six engine-native
GPU screenshot copies, and compiled Win64 Development AIHeroes.exe.
PARTIAL: touch/mobile UI does not yet expose I; touch hardware,
notch/DPI, Android build, real local LLM, player balance still open.
NOT CLAIMED: packaged/signed Windows release, APK, merged branch,
Git push, cloud runtime or deployment. The shared repo may change
after this frozen provenance and requires a fresh build/retest.

## A/B. Research and boundaries

Original game idea: logistics across procedurally generated
zones, guarded passes and known roads. Source design reference:
https://h3hota.com/en/template-format
https://h3hota.com/en/templates
No HotA textures, factions, text, game code, or map assets copied.
All future arrival days are conditional, never win probabilities.

## C/D. Source and user-visible behavior

The new ZoneJourney component uses validated PlayerView, then
stages at legal known unguarded sites; moves cost <=8 each.
The pathfinder does not plan through a guarded intermediate site.
Dijkstra ordering: earliest day, fewer human confirmations,
less movement, stable lexical tie. 14-day bounded horizon.
I previews selected known destination and writes a HUD/log line,
but moves/End Day/battles and SaveGame are never called.
The previous H/Shift+H/D/Enter/Space controls are preserved;
modal D blocks I. Future GUI should expose itinerary via touch.

## E. Build and real-window verification

Editor: 229 pinned source/config files, frozen module 2830848 bytes, SHA256 f2fdd99eb5038f00883ad65b0cf3e7653e0e3ca9bb09dc4936790d91341c0bbd.
Full Engine Automation: 245/245 PASS; warnings=0, failures=0, skipped=0; build_verified=true.
Test report SHA256: 74459041c620d87406c3d759c8801d6f1da3c14fa056da16896c53437378edc3.
Six new ZoneJourney tests passed including 640 seed/branch worlds;
unreachable guarded traversal, invalid map and 9-move shortcut
staging are checked against the standard adventure pathfinder.
The QA process targeted its own validated Unreal -game window.
I logged a preview with zero orders; I/Enter/Space were blocked
in the D modal; only a later human Enter caused each battle.
Six screenshots, 1280x720, total PNG bytes 3172689.
SaveGame before/after filenames and SHA256 were identical.
Compiled Development Win64 AIHeroes.exe, bytes 333720064, SHA256 0e5e15d3fc46befa0fac06c514468da90f81e61d20ede9186731e7d4bd7cf963. Not packaged or signed.
Before overwriting the previous game exe the original binary was
backed up with hash read-back under
Saved/QA/ZoneJourneyBeforeWin64Game_20261029/.
Two integrated GameMode source files were separately backed up
under Saved/QA/ZoneJourneyIntegrationBefore_20261029/.

## Artifact manifest: bytes and SHA256 read-back

| Relative path | Bytes | SHA256 |
|---|---:|---|
| Source/AIHeroes/Public/AIHeroesZoneJourney.h | 2492 | 4713bb04dcc97dca218744cbe0c38ca8c5edb857e5bd440027dc5d942cc364a5 |
| Source/AIHeroes/Private/AIHeroesZoneJourney.cpp | 12250 | f4ea8eaf6d976143e945b7f4936a9f5cea91e9fb56f16608622bc76557e471c1 |
| Source/AIHeroes/Private/Tests/AIHeroesZoneJourneyTests.cpp | 11826 | b24a6b3c3c9f19ab6040d5b4875abc44ddc7595df77a45416823eb17c69ea48e |
| Source/AIHeroes/Public/AIHeroesZoneExpeditionGameMode.h | 3139 | b87195daf6fb0ad16c0b1bec436adec3f6cd46bc33c395730c856a395c9e1e82 |
| Source/AIHeroes/Private/AIHeroesZoneExpeditionGameMode.cpp | 28546 | 49fc160281d29ac55cfa3d0bd985942dd518c269fa4c01d2519de2572ea7f1d2 |
| Tools/qa_zone_journey.ps1 | 12542 | 2ef861622efd13e71562d3d1c5cdc68079ad4bfcf20bcf0cfaef717ef833056b |
| Docs/ZONE_KNOWN_JOURNEY_V1.md | 9463 | 79419c56e8966de7425d064b4b8debf30be8749a537b6b4b6fdbcf0776f3f1a3 |
| Docs/BuildRuns/20261029T161702109Z/compiled.json | 63193 | b681d8494db8124aa3852e24642dc7cddac99873451d620d2ad5d053778e683b |
| Docs/AutomationRuns/20261029T161717792Z/index.json | 97360 | 74459041c620d87406c3d759c8801d6f1da3c14fa056da16896c53437378edc3 |
| Docs/AutomationRuns/20261029T161717792Z/provenance.json | 63313 | 06966e882a95547e18b41b6d364e5b7a54e5b3650502a77146fa2a7766dae741 |
| Docs/BuildRuns/20261029T161930415Z_GameWin64/game_compiled.json | 64362 | 3402df037a7e9e8bb80628475059bf5f141ff8be7ebb6a60112f6744ce422c4c |
| Docs/BuildRuns/20261029T161930415Z_GameWin64/build.log | 9228 | 62dce86f201a731d689fa5a1f88af5ccb04832f1b4b8807ecc5e6f968b538f65 |
| Docs/VisualRuns/ZoneJourneyVerified_20261029T161330869Z/initial.png | 578707 | e6c1331e0e546a9a2ba7c9bbe0b3bb68f656ea44865abf70a3a42f80971ac5ce |
| Docs/VisualRuns/ZoneJourneyVerified_20261029T161330869Z/journey.png | 596640 | 0245ac4e1e688fa544511088c67ebf8fe9120bd4137965ba968ca9f3fba63d5b |
| Docs/VisualRuns/ZoneJourneyVerified_20261029T161330869Z/dossier.png | 311388 | 8fbccfb4dfc94c9904e84996c5e2e94987f5fc9b391542679789a3ac38936a46 |
| Docs/VisualRuns/ZoneJourneyVerified_20261029T161330869Z/forest.png | 567258 | bc8eafd60410cb0a83a264371d6a11e9a6308f77298ec24e4cfd0bf5ee2251bf |
| Docs/VisualRuns/ZoneJourneyVerified_20261029T161330869Z/day2.png | 562526 | 8c5d61d1bbb876f1a8de484dd07d8005eb0401da0fb15a6a7c0370e574dc803d |
| Docs/VisualRuns/ZoneJourneyVerified_20261029T161330869Z/ore.png | 556170 | 3592507ae061ac93dbf6470526f5bfd4c5af0662ff011efd47646e1e70e553ce |
| Docs/VisualRuns/ZoneJourneyVerified_20261029T161330869Z/receipt.json | 5049 | 367f2f959d0eec8ae6827809afbb827787bb4c0fa598ae1d24cbc8567df5e2df |
| Saved/QA/ZoneJourneyVerified_20261029T161330869Z/UnrealEditor-AIHeroes.dll | 2830848 | f2fdd99eb5038f00883ad65b0cf3e7653e0e3ca9bb09dc4936790d91341c0bbd |
| Saved/QA/ZoneJourneyVerified_20261029T161330869Z/AIHeroes.exe | 333720064 | 0e5e15d3fc46befa0fac06c514468da90f81e61d20ede9186731e7d4bd7cf963 |

## Frozen scope and non-claims

This receipt verifies the exact captured files, not other agents
work after this build. It does not show a shipped Android APK.
No production SaveGame was written or migrated by this scope.
The live scene artwork shown in QA was created by other agents.

Delta D Omega Lambda: delta legal, visible multi-day itineraries;
D validated graph => code/tests => tested DLL => window QA
=> compiled Game EXE => 21 checksummed artifacts;
Omega <= 95 percent for observed local Windows effects;
Lambda recheck after source changes, UX playtests, hardware touch.
