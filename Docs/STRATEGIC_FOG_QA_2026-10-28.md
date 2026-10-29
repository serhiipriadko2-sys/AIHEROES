# AI HEROES — Strategic Site Fog v1 — research, build and QA receipt

Date: 2026-10-28 (Europe/Berlin); remote workstation logs after local
midnight may say 2026-10-29. Remote repo: `C:\github\AIHEROES`.
State: **implemented → Windows editor compiled → automated-tested →
real-game validated**. Not a released PC/Android game, not GitHub
deployment/merge, not an installed model checkpoint.

## A. Intake and B. SIFT

The user requested to continue complex development with web research.
`python Scripts/verify_sot30.py --directory .iskra/sot30` returned
`SOT30_INTEGRITY_PASS version=v5.5.7 knowledge=30/30
sha256sums=32/32 corpus_bytes=4054425 members=33/33`.

[FACT/WEB] Official HotA map-template references:
- https://h3hota.com/en/template-format
- https://h3hota.com/en/templates

Its RMG maps use **zone topology**, guarded/unprotected connections,
and configurable road constraints. They do not imply any rights to
reproduce HotA maps, graphics, units or code. No HotA assets have been
imported. Official Epic SaveGame documentation:
https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine

[INTERP] The first useful original game feature is asymmetric **knowledge
of the playable campaign map**, not importing the separate experimental
7–11-zone generator into incompatible version-1 saves.
[HYP] The starting mask and adjacent-discovery rules will require
balancing in longer campaigns.

## C. Frame — delivered game behavior

- New campaign begins **SCOUTING 3/5**, with HQ, FOREST and ORE known.
  OUTPOST and RUINS are invisible and cannot be selected or traversed.
- Player chooses and confirms attack. Winning against FOREST reveals
  OUTPOST (SCOUTING 4/5). Winning against ORE reveals RUINS (SCOUTING 5/5).
  These effects depend on actual arrival, not a speculative route.
- Pure `FAIHeroesFogRules` applies a five-bit mask; invalid IDs,
  impossible masks, hidden hero/selection/owned sites fail closed.
- Existing native road/path code consumes bDiscovered; this is not a
  new second movement engine. Roads shown are 2 initially, 3 after
  FOREST becomes explored. Actors/collision, map labels, player hit
  targets and route rendering hide unknown sites.
- Typed offline advisor skips hidden targets and does not receive
  their garrison composition in its normal snapshot.
- Hidden hostile activity is not described as witnessed; the rival
  forecast is generic until OUTPOST becomes known.
- `FAIHeroesAIProposalGate` explicitly refuses hidden targets and
  invalid fog snapshots; it still cannot execute an order.
- `CampaignFingerprint` includes fog mode+mask, so stale battle
  and purchase receipts cannot be reused after discovery.
- SaveGame schema still **1**, with two optional fields. Old saves
  lacking them load as `bFogInitialized=false, ExploredSitesMask=0`
  and preserve their original **5/5 known** map. New saves retain
  discovered bits; A/B disk readback compares both fields. No
  existing human save slot was overwritten for a test.

## D. Step — actual code changes

- `Source/AIHeroes/Public/AIHeroesFogRules.h`
- `Source/AIHeroes/Private/AIHeroesFogRules.cpp`
- `Source/AIHeroes/Private/AIHeroesFogGameMode.cpp`
- `Source/AIHeroes/Private/Tests/AIHeroesFogTests.cpp`
- Integration: CampaignSave, CampaignRules, SaveSystem,
  GameMode, LiveBattleGameMode, TravelRules, TravelGameMode,
  RivalGameMode, Advisor, AIProposalGate, HUD and README.
- `Source/AIHeroes/Private/AIHeroesTravelSmoke.cpp`
  updated for new initial visibility; only the world test contract
  changed, not the player-authorized movement or battle rules.
- Narrow unrelated parallel compile fix:
  `AIHeroesTravelPresentationSmoke.cpp` iterates
  `TObjectPtr<AAIHeroesWorldNode>` with explicitly typed node.
  Existing travel-animation semantics left unchanged.

No git commit/push by this operation. Parallel changes, field
dwellings, travel animation and zone-preview work preserved.

## E. Verify — evidence and status

**Unreal Editor C++**:
`Build.bat AIHeroesEditor Win64 Development
-Project=C:\github\AIHEROES\AIHeroes.uproject
-WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1`
**Result: Succeeded** after the fifth fog/proposal test was added.
Compiler issued the ordinary installed MSVC 14.51 preference warning;
do not describe that build as warning-free.

**Complete Unreal automated test suite:**
`Tools/run_aiheroes_automation.ps1 -Filter AIHeroes`
**138/138 Success, 0 Fail, 0 NotRun, 0 test warnings, exit code 0.**
`Docs/AutomationRuns/20261028T212347070Z/index.json`
(54 583 bytes, SHA256
`7AD540AFEBCF5B6880E5D65862E6A747F171AD0ED62C2658AE3C0BB593A62328`).

Five individual Fog groups verified:
`AdjacentDiscoveryAndLegacy`, `TamperAndAuthorityBoundaries`,
`TravelGraphAndAdviceDoNotLeak`, `SaveValidationAndDiskRoundTrip`,
`ProposalGateRejectsUndiscoveredSite`. Other suite groups,
including Proposal and TravelV2, also passed.

**Real 3D Windows window, uniquely tagged**
`-log=AIHeroesFogQA.log`:
- Dedicated screenshot starting **3/5 known**; FOREST and ORE only,
  no visible OUTPOST/RUINS markers.
- Player click FOREST, Enter authorized battle, live Grid V2
  `BEGIN node=FOREST`, `COMMIT node=FOREST`,
  then `[AI HEROES SCOUT] discovered=OUTPOST known=4/5 no_order=1`.
- Player ends the turn to restore moves, selects ORE, confirms battle,
  `BEGIN node=ORE`, `COMMIT node=ORE`,
  `discovered=RUINS known=5/5 no_order=1`.
  Last screenshot shows DAY 2, 0/4 moves, 10 player units, DATA 4,
  and all five node labels.
- The two scripted test segments both hashed all files in
  `Saved/SaveGames` before and after, with unchanged file counts
  and SHA256. **No user save writes**.
- Acceptance markers:
  `FOG_MANUAL_PASS initial=3 after_forest=4 forest_battle=1
  player_order=1 no_save_writes=1` and
  `FOG_ALL_DISCOVERED_PASS sites=5/5 battle_player_authorized=1
  save_writes=0`.

**Independent adventure route smoke** with dedicated
`-AIHeroesTravelSmoke -log=AIHeroesFogTravelSmokeQA.log`:
`[TRAVEL V2 SMOKE] PASS roads=2_then3
hidden_target_blocked=1 fog_reveal=1 forest_cost=2 next_cost=1
commit=1 save_calls=0`.
Existing travel rule logic validated against the new visibility gate.
These are targeted QA windows, **not** an APK/device test.

## Concrete artifacts (relative to repo root)

| Path | Bytes | SHA256 |
|---|---:|---|
| `Source/AIHeroes/Public/AIHeroesFogRules.h` | 1123 | `499161B1415CE15D07EB2FC0F6331A5328B83CA1191E0C7D0C91BB1F311335A7` |
| `Source/AIHeroes/Private/AIHeroesFogRules.cpp` | 2972 | `532EAA5DAE8591C8652CC0C7CC1B2968AC556E59B02BDA6DEEA0D2B0DD1BDF70` |
| `Source/AIHeroes/Private/AIHeroesFogGameMode.cpp` | 2315 | `0AE454BD09D046739941557938177E60AE6352257AEEFA495AFA87CD6C2A82AC` |
| `Source/AIHeroes/Private/Tests/AIHeroesFogTests.cpp` | 13140 | `3F4EEBB302B41448E6107C1EEE3BC8123EFB9653A44709C8A28B612F0A054F99` |
| `Tools/qa_strategic_fog.ps1` | 3912 | `CA4EFEAAE3221E176388542E70209B1D5EAA19A4C2996DF90D7964124C406C12` |
| `Tools/qa_fog_second_branch.ps1` | 3644 | `D293B98495D8BDDFCE5FB4A32723C8635BB17AB3DD690326C21F9DF1D2CAA2FF` |
| `Docs/AIHEROES_FOG_START_2026-10-28.png` | 362909 | `E7DCC7AA9477CEE6E556FE74951180854AB47FF4F22F4AF40201240705AA9A88` |
| `Docs/AIHEROES_FOG_FOREST_REVEAL_2026-10-28.png` | 366909 | `DF6B08480F30BDF64962831BC74E7DB138FE7371B005EE5995B70119F9F036AE` |
| `Docs/AIHEROES_FOG_ALL_REVEALED_2026-10-28.png` | 372877 | `3FA407E8BF928DBD861F254C56834FDA7EAE1663D612092998FB39BE28742BBE` |
| `Docs/AutomationRuns/20261028T212347070Z/index.json` | 54583 | `7AD540AFEBCF5B6880E5D65862E6A747F171AD0ED62C2658AE3C0BB593A62328` |

Additional source contract:
`Docs/STRATEGIC_FOG_V1.md`; inventory manifest after this
receipt gives its latest SHA256, because the test count was updated
from four to five after the first research draft.

## F. Close — explicit non-claims and next steps

This feature implements **site-level knowledge**, not an opaque 3D
terrain shader or genuine line-of-sight algorithm; there are five
hardcoded known site IDs. The separate 7–11-zone template preview is
not the active campaign and is not silently migrated into old saves.
Internal typed snapshots still contain map topology metadata for
deterministic validation; an external real LLM must **not** receive
them without a separate privacy/redaction adapter and QA.

Original hero XP, item equip slots, enemy strategic hero movement,
dynamic fog, siege encounters, Android APK and real LLM inference
remain unimplemented or unverified.

**PASS** for this locally built/tested Windows five-site scouting feature.
**PARTIAL** for complete cross-platform game/release.

∆DΩΛ: ∆ — strategic unknown-site visibility and strict proposal gate;
D — compiled DLL, 138/138 Unreal tests, three rendered states,
route smoke, user-save file hashes, source/spec receipts;
Ω — 94% for this local checked snapshot;
Λ — re-test on parallel source changes; later expand into
multi-zone line-of-sight, movement animation and Android touch QA.
