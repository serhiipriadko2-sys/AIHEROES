# AIHEROES — Field Dwellings implementation & QA receipt

Date: 2026-10-28/29 (Windows local observation); actual repository:
`C:\github\AIHEROES`.
Lifecycle: **LOCAL SOURCE IMPLEMENTED | WIN64 GAME + EDITOR BUILD PASS | LATEST FULL UNREAL AUTOMATION 138/138 PASS (previously 137/137) | EXACT-WINDOW GUI QA PASS | USER SAVES UNCHANGED | NOT COMMITTED/DEPLOYED**.

## 1. Source-grounded decision

Public source: Horn of the Abyss official documentation https://h3hota.com/en/documentation states player-owned dwellings accumulate creatures every week; unowned dwellings reset to one weekly production. Its official template format https://h3hota.com/en/template-format distinguishes **zone topology/guarded passageways** from objects placed *inside* zones. AIHEROES implements the original gameplay **principle**, with new original objects and its own balances/geometry, not a copy of HotA units, art, maps, spells, faction or source code.

The field supply operates **independently** from existing HQ recruitment reserves, weekly neutral-guard reinforcements, road-version V1/V2 and rival NPC recruitment. It does not create a sixth/sixth+ saved map node. The five-node topology and old game movement budgets remain unchanged.

## 2. Exact gameplay changes

Two author-created field facilities (nonblocking Unreal basic-shape 3D markers, with colors changed by ownership):
- **FOREST CAMP** near the FOREST map node: starts with 5 Militia available, cap 50. When claimed, increases +5/week from Day 7→8; unclaimed resets to 5/week.
- **ORE AERIE** near ORE: starts with 2 Scouts, cap 24. When claimed, grows +2/week; unclaimed resets to 2/week.

The human must first clear the hostile parent, arrive there, then select it and explicitly press keyboard **E** or tap/click **FIELD E**. This first command only claims; another command purchases exactly one pack using canonical costs. Insufficient stock, currency, territorial ownership, hero co-location, or army capacity causes a fail-closed no-spend/no-army mutation. GameMode owns the only mutation; the pure rule module exposes no AI purchase capability.

On Day 7→8, the day-end planning step preflights independently: resource growth, neutral defense, rival action, HQ reserves, and field reserves. Failure cancels the entire day. Successful field growth is logged separately as `[AI HEROES FIELD] WEEK`. Claimed parent territory cannot be fabricated in saved data; losing the parent releases facility claim on a valid day tick.

Field markers are never collider/selection targets. Their visibility follows the **same fog predicate** `IsNodeDiscovered` as their parent nodes, so they cannot disclose unseen zones. The new HUD action ID204 shares geometry with mouse/touch hit tests; action IDs0..12, 200..203, battle controls100..103 and modal controls remain independently active/masked.

Optional schema-1 SaveGame properties: `bFieldDwellingsInitialized`, `bForestCampClaimed`, `ForestCampUnits`, `bOreAerieClaimed`, `OreAerieUnits`. Legacy v1 saves have missing marker / zero defaults and restore one neutral starter packet *in memory* without writing. New read-back compares all fields, validation rejects forged site claims or out-of-range stock. The live battle/BASE campaign fingerprint includes claimed status and stocks. Original human save slots were NOT accessed by tests except inventory/hash inspection.

## 3. Verified checks

**Win64 Editor build:** Unreal Engine5.8 `AIHeroesEditor Win64 Development` built with `-NoUBA -MaxParallelActions=1`. Latest source-linked build result **Succeeded** after an unrelated concurrent travel-presentation smoke loop C++ typing error was corrected by its owner. Initial failed build was not counted.

**Independent Windows Win64 Game Development build:** UnrealBuildTool serial `-NoUBA -MaxParallelActions=1` completed **50/50 compile/link/metadata actions; `Result: Succeeded`, `FIELD_WIN64_GAME_EXIT=0`**. Exact output `Binaries/Win64/AIHeroes.exe`, **333,004,800 bytes**, SHA-256 `1246caccb100c69f586e3737160ead8bb02dfc38cf3cad699b7fa293760b8bf5`, UTC 2026-10-28 21:22:48. The Game build compiled FieldDwelling rules/actors/UI/save and the same test sources, then linked the standalone Development executable. A Development executable is not a signed shipping installer or a released/mobile package.

**Full automated regression (latest, after parallel Editor rebuild):** `Docs/AutomationRuns/20261028T212428230Z/index.json`, **138/138 Success; Fail 0, NotRun 0, Warnings 0**, command `Tools/run_aiheroes_automation.ps1 -Filter AIHeroes`, exit **0**. This explicitly supersedes the earlier run `Saved/QA/TravelViewValidation_20261028T211414863Z/report/index.json` (**137/137 Success**) on the previous DLL. Both reports are preserved; all six newly authored tests **Success** on the latest DLL:
- `AIHeroes.Field.CalendarNeutralResetVsOwnedAccrual`
- `AIHeroes.Field.ClaimAuthorityAndTerritoryLoss`
- `AIHeroes.Field.GameModeHumanClaimPurchaseAndWeek`
- `AIHeroes.Field.RecruitAtomicNoFreeArmy`
- `AIHeroes.Field.ResponsiveControlsAndCombatLock`
- `AIHeroes.Field.SaveV1OptionalDataRecovery`

**Real game 1280×720 GUI:** Dedicated PID52252, exact `-game -windowed -ResX=1280 -ResY=720 -log=AIHeroesFieldDwellingQA.log`. `Tools/qa_field_dwellings.ps1` drove only verified-HWND mouse/key messages. First QA attempt failed because the test script expected an HQ-specific error although the game correctly had no selected node at boot. QA expectation was corrected; the COMPLETE rerun exited **0**, logging `FIELD_QA_PASS` and `FIELD_SAVES_UNCHANGED_PASS`.

Observed sequence: FIELD button ID204 refused without selection; FOREST clicked; human Enter authorized live battle; B finished; battle committed exactly **once** (9 actions); Enter returned to map; FIELD button claimed **FOREST CAMP** with stock5 and no spend; E bought 5 Militia locally; second E refused (0/5 remaining); seven human NEXT DAY presses advanced to Day8; log showed **FOREST +5 (5), ORE +0 (2)**, independently neutral +5 and HQ reserves +5/+2; E bought second field pack. No second battle commit. Player SaveGame inventory (filename, bytes, modification ticks, SHA256) matched exactly before/after. QA-owned game window was closed gracefully, without terminating other agents' windows.

**Visual QA:** Both 1296×759 PNG captures (including Windows titlebar) were directly inspected: forest green owned marker with `CAMP 5 Militia [OURS]`, ore marker marked `AERIE 2 Scouts [FREE]`, visible FIELD E button, clear goal/AI/replay controls, restored stock on Day8; hidden-zone icon visibility follows fog. Day8 event string truncates near `FIELD:` at 1280px due to HUD width limit, but the field stock is separately visible in map label and the full console log. This is a future UX polish, not a missing purchase.

**Source of truth:** SoT30 integrity **PASS**, Knowledge30/30, SHA file32/32, pinned manifest `0decd118a6300324b757d2587ec315a569aaf5800299d2b33bf046feaf56aeff`. `git diff --check` exit 0.

## 4. Frozen artifact evidence (Windows; project-relative)

The table covers durable *currently matching* source, test, screenshot, log, Game EXE and report files. The Editor DLL is intentionally **not included in this frozen checksum table**: it was overwritten by a parallel build after the 138/138 run, so current DLL bytes are not claimed as the exact tested runtime. At the 138/138 run the Editor DLL was 1,860,096 bytes with SHA-256 `587c45eda1ffed219ea6f1b34b2f05b0b335f1b62ddf35d781acd47772bd72ac`; the subsequent independently observed live DLL has the same byte count and SHA-256 `93a75c50547decf1a134e4212dfac5fbfa6760177141226bbbefa6218d951edd`. Future Editor readiness requires a new frozen-source test after the parallel writers stop.

| Path | Bytes | SHA-256 |
|---|---:|---|
| `Docs/FIELD_DWELLINGS_GAMEPLAY_CONTRACT_2026-10-28.md` | 6390 | `f02d95d8fe61170d30b106fd6eee5e0951384454c2f86e45bd92741eb5cf0ce3` |
| `Source/AIHeroes/Public/AIHeroesFieldDwelling.h` | 2530 | `d418805a433fd50af810c705e7842a22c817d73d6b64a0092c8b502af2b2a7d0` |
| `Source/AIHeroes/Private/AIHeroesFieldDwelling.cpp` | 6975 | `be1bf039316d2f0ed7a183325394f230ba459b8894f032c2cf4d27f232bc0629` |
| `Source/AIHeroes/Private/AIHeroesFieldDwellingGameMode.cpp` | 8034 | `9491ea10f920167ca949f1fe3d5d4be01edacd04574a00eb313f9f29e16473e6` |
| `Source/AIHeroes/Private/Tests/AIHeroesFieldDwellingTests.cpp` | 17907 | `8f687ff915276dcb44a803671eda9889c1eb83e838e95137365fa27f799e5fd4` |
| `Source/AIHeroes/Public/AIHeroesGameMode.h` | 10920 | `065be14920c5b4362ffac8eecfe8a84e6f834ed76def587041b0018473a90ca8` |
| `Source/AIHeroes/Private/AIHeroesGameMode.cpp` | 36170 | `628d9b6112236cbb110e9ed10be60194ad99a19605d9512c559d3396afb3818e` |
| `Source/AIHeroes/Public/AIHeroesCampaignSave.h` | 3472 | `f0e8f9024594a18526eb21a3d8b46871246a3d9848756bb55ad4efb49f61719e` |
| `Source/AIHeroes/Private/AIHeroesCampaignRules.cpp` | 9711 | `7e31c47d4afb5f4ac772588d57117cccdc804d965e10a176cea2b512bbc935f9` |
| `Source/AIHeroes/Private/AIHeroesSaveSystem.cpp` | 8900 | `d471e1f9e22661816b5138a649368ac66148ec8fcad2c19de302aa0d4e722a0c` |
| `Source/AIHeroes/Private/AIHeroesLiveBattleGameMode.cpp` | 12592 | `65b8b7bb4e10ad68159656cf81e906fbaf9077e4b476665173f93aa8196a0c0a` |
| `Source/AIHeroes/Private/AIHeroesHUD.cpp` | 19586 | `60a10f39204315dab1224484b59b99ebe9a7d59a7e61e4ad2abf3c6c968231fa` |
| `Source/AIHeroes/Private/AIHeroesHUDLayout.cpp` | 5308 | `9a6afe1f7f180146eb55d09818ed23380194d413cd4555a0f6dc876e83dc5902` |
| `Source/AIHeroes/Public/AIHeroesPlayerController.h` | 1329 | `6a7e3b909031231a59e7a2bcb80feece5e6d26e913e6c291546b373e426e9ecb` |
| `Source/AIHeroes/Private/AIHeroesPlayerController.cpp` | 13476 | `4b5fd7dd5285581bd95d9283ebdb46cb518af87f4faaad102d52318674694175` |
| `Source/AIHeroes/Private/AIHeroesFogGameMode.cpp` | 2315 | `0ae454bd09d046739941557938177e60ae6352257aeefa495afa87cd6c2a82ac` |
| `Tools/qa_field_dwellings.ps1` | 5951 | `e9ac9a4b6d18f4d9f7e0a62fc95a5e86a2dfc146100251c444c56280a8a70697` |
| `Saved/QA/TravelViewValidation_20261028T211414863Z/report/index.json` | 54159 | `c04cb190976320c0aae92ef4aeb6d2e34c2a5f197d3438abd769bcf0d246e707` |
| `Docs/AutomationRuns/20261028T212428230Z/index.json` | 54576 | `9b0e8c79f19641b9bd37cf997b9b32f1923327d39f9b7f830a0da54e8c1b51d9` |
| `Docs/AutomationRuns/20261028T212428230Z/run.log` | 431539 | `27bbb924d3d113ea8ab7612c9687e9f55b22af9599f918a6fd6bfac0900b3d4d` |
| `Saved/QA/TravelViewValidation_20261028T211414863Z/tests.log` | 430891 | `10c9035668cbd880d5a4dccf402c1bc75a942c7ccd7b38ea7b071841fac82d95` |
| `Saved/Logs/AIHeroesFieldDwellingQA.log` | 199081 | `8efd1da54aefd6851adedf9da57476c299dcea7ef8a15ceccafc69ba4fd0e68a` |
| `Saved/QA/FieldDwellings/FIELD_FOREST_CLAIMED.png` | 369544 | `080ae4f9b014dc87c4d0826cfb8b2a1f5437c3ae6b77ed0118cc594357b957c0` |
| `Saved/QA/FieldDwellings/FIELD_FOREST_WEEK2.png` | 384631 | `2dc7787fc922697a48ce695bb0ad2705fb0b9838ff812c1559e6181e0b74cf2c` |
| `Binaries/Win64/AIHeroes.exe` | 333004800 | `1246caccb100c69f586e3737160ead8bb02dfc38cf3cad699b7fa293760b8bf5` |

## 5. Boundaries

An independent Win64 Game Development build and an earlier Editor build passed. A full 138/138 Automation run passed against the Editor DLL version explicitly recorded above; **the live Editor DLL changed afterward**, so its current readiness is UNKNOWN pending frozen-source retest. **Neither binary is a signed/shipping installer**, and no distribution or deployment has been performed. Android SDK/NDK availability, APK/device QA, actual model weights/inference, multiplayer arbitration, generic physical world-node generation, full exploration and strategic balance remain unverified/not implemented in this feature. No live project/canon writes, Git commits, merges, pushes or deployments performed by this operator.

**Next original gameplay milestone:** map-based supply routes / faction-controlled dwelling logistics; version a typed ownership contract, verify illegal map transitions and use saved receipts before allowing non-local recruitment. A speculative AI plan must remain advisory until the human authorizes each purchase/move.

**∆DΩΛ:** ∆ — original two-site field production, separate claim and recruit actions, fog-safe visuals, bounded SaveGame migration; D — official source → code → Unreal Editor build → 137/137 complete automation → exact-window human-order simulation → screenshot inspection, SaveGame inventory proof and hashes; Ω — 95% for observed Windows Editor QA, lower for mobile and untested packaging; Λ — re-run after source modifications, new SaveGame schema or any future external dwelling/online integration.
