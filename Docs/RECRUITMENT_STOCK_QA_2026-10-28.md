# AIHEROES — Weekly Recruitment Stock: Implementation, Build and QA Receipt

**Date:** 2026-10-28  
**Working repository:** `C:\github\AIHEROES`  
**State:** **WINDOWS_WIN64_GAME_BUILD_PASS | WINDOWS_EDITOR_BUILD_PASS | UNREAL_AUTOMATION_117_117_PASS | REAL_WINDOW_QA_PASS | SAVE_INVENTORY_UNCHANGED | ANDROID_NOT_RUN | LLM_NOT_CONNECTED**.

## 1. External evidence and IP boundary

User explicitly requested web research. The original system was motivated by official Horn of the Abyss 1.8.1 documentation about cumulative player-owned recruit availability across weekly boundaries; unowned dwellings follow a different weekly schedule. Primary documentation: https://h3hota.com/ru/documentation; release version: https://h3hota.com/ru/download; third-party content/licensing boundary: https://h3hota.com/en/faq. No HotA assets, characters, artwork, maps, music or copyrighted text were included. The AIHEROES-owned OUTPOST provides an **original supply bonus**, not a reimplementation of an existing HotA dwelling.

SoT30 local source-of-truth integrity at read-back: **PASS version 5.5.7, Knowledge 30/30, sha256sums 32/32, corpus 4,054,425 bytes**, pinned manifest SHA-256 `0decd118a6300324b757d2587ec315a569aaf5800299d2b33bf046feaf56aeff`. No project-canon file changed. No Git commit, reset, merge, push or deployment was performed.

## 2. Implemented original mechanics

- **Player-owned recruit stock**, a distinct resource from neutral guards: starter 10 Militia, 4 Scouts; Militia purchase consumes 5 units, 50 Gold and 5 Wood; Scout purchase consumes 2 units, 70 Gold and 4 Ore.
- **Weekly production** occurs only when Day 7→8, 14→15, etc.: Militia growth = `5 + 2 * HeadquartersTier` and Scout growth = `2 + (HeadquartersTier / 2) + (owned OUTPOST ? 2 : 0)`. Tier range 0–3; caps 250 Militia, 120 Scouts; saturation at cap. Other days do not change stock.
- **Atomic player-confirmed transactions**: strict preflight of stock and price before any reserve/currency/army mutation. Invalid stock or inadequate resources are a fail-closed refusal; no free third purchase. This keeps owner/military economics separate from the existing neutral growth and NPC raid/recruitment.
- **BASE GUI** shows stock and unavailable purchases. The **N recruitment advisor** sees typed stock counts and cannot recommend an unavailable offer even if the player has gold.
- **Daily state plan** validates economy, neutral-guard week, rival action and player recruit stock **before** one final day commit; failure leaves existing income and calendar untouched. An existing AI cannot purchase stock on its own.
- **SaveGame compatibility** is an additive optional tagged-property extension of v1: `bRecruitmentStockInitialized`, `MilitiaReserve`, `ScoutReserve`. Legacy false+0+0 initializes one starter stock on in-memory load, not retrospective weeks and not a disk write. New valid marker requires bounded fields. Semantic SaveSystem read-back checks new fields, and campaign/battle fingerprint includes reserve counts to catch state drift.
- **No AI model inference** was implemented. In-game 1B/3B/8B are still labels/model-slot themes, not real installed neural checkpoints.

Detailed contract: `Docs/RECRUITMENT_STOCK_CONTRACT_2026-10-28.md`.

## 3. Build and test evidence

1. **Win64 Game Development build** with UnrealBuildTool 5.8, `-NoUBA -MaxParallelActions=1`: **Result Succeeded**, exit **0**; 37 sequential build actions. Actual binary `Binaries/Win64/AIHeroes.exe`, 332,874,240 bytes.
2. **Win64 Editor Development build** also **Succeeded**, latest incremental 4/4 actions including UnrealEditor-AIHeroes.dll. Concurrent LiveCoding initially blocked an attempted Editor build; the other QA process was not terminated. Separate Game target compiled; after the blocker released the Editor build succeeded.
3. **Full Unreal Automation:** `Saved/QA/TravelValidation_20261028T205203365Z/report/index.json` reports **117 cases, 117 succeeded, 0 failed, 0 notRun, 0 warnings**. Relevant six tests:
   - `AIHeroes.Recruitment.StockWeeklyCalendar`;
   - `AIHeroes.Recruitment.StockOwnershipUpgradeAndCaps`;
   - `AIHeroes.Recruitment.StockNoFreeRecruitment`;
   - `AIHeroes.Recruitment.StockQuotesAndAdvisorNoPhantomOffers`;
   - `AIHeroes.Recruitment.StockSaveMigrationAndRotation`;
   - `AIHeroes.Recruitment.StockGameModeWeeklyEndToEnd`.
4. **Real 1280×720 Unreal game:** dedicated exact-PID window **17824** launched with `-game -log=AIHeroesStockQA.log`. Targeted messages (no global mouse/keyboard) and GUI screenshot automation:
   - Two successful Militia packs: reserves 10→5→0; third refused with no resources spent.
   - Two successful Scout packs: reserves 4→2→0; third refused with no resources spent.
   - BASE opened with both packs unavailable; screenshot saved and visually checked.
   - Seven player-authorized End Day actions; Day8 log: **neutral guards +7** (existing feature) and **player reserves +5 Militia, +2 Scouts** (new feature). Rival OUTPOST separately recruited 22 guard HP. No shared inventory conflation.
   - BASE re-opened, packages available; both types were successfully purchased once more, reserves to 0/0.
   - **Exactly 0 live battle commits** during the entire production QA.
   - Script `Tools/qa_recruitment_stock.ps1` exited **0**: `STOCK_QA_PASS` and `STOCK_SAVES_UNCHANGED_PASS`.
   - Full pre-/post- game inventory of existing `Saved/SaveGames` files compared filenames, byte lengths, UTC modification timestamps and SHA-256; no human save was written or altered. The QA-owned window was closed gracefully with PID verification; unrelated processes left alone.
5. QA screenshots: first shows **STOCK 0 MILITIA / 0 SCOUTS** and both offers **BLOCKED**; second shows **STOCK 5 MILITIA / 2 SCOUTS** and both **AVAILABLE**. Both screenshots actually inspected visually; each 1296×759 including the Windows titlebar.
6. **PowerShell script parse errors 0**; `git diff --check` exit 0. Any Git warnings about possible LF→CRLF conversion are non-fatal line-ending notices, not a source validation failure.

## 4. Frozen verification table (project-relative paths)

All values below were read directly from the Windows machine *after* the gameplay QA. These hashes identify the tested local content at the observation time; other developers are actively editing the same repository.

| Path | Bytes | SHA-256 |
|---|---:|---|
| `Docs/RECRUITMENT_STOCK_CONTRACT_2026-10-28.md` | 5173 | `bb93151b68eedb85200c72b55e9d283f62c7d5f0bd5051bff542cefd5491ab60` |
| `Source/AIHeroes/Public/AIHeroesRecruitmentStock.h` | 2149 | `198390953abae9a9223b2844fb826113f1f01232c3d9023bd66da1388a729134` |
| `Source/AIHeroes/Private/AIHeroesRecruitmentStock.cpp` | 3344 | `d299c718cb22702292debc6870b2d1110edf19b2438592f6d17a082957f70005` |
| `Source/AIHeroes/Private/Tests/AIHeroesRecruitmentStockTests.cpp` | 16395 | `9143e0ecae49a47aca2ff0507558ce3322d5716b2a0f79e21e67689f179bedc8` |
| `Source/AIHeroes/Private/AIHeroesGameMode.cpp` | 31919 | `9b45053e9b713d1061f654c3cb202a5d81884561dee3186e54f2789fa5557ed4` |
| `Source/AIHeroes/Public/AIHeroesGameMode.h` | 9172 | `2b5e04729bbd76715f2e9fddccfb09e557449de267c799d7c1c4c31870511ff3` |
| `Source/AIHeroes/Private/AIHeroesSaveSystem.cpp` | 8520 | `9bc16a845cc5510e75d01f1f9c990e4349da275775c9b2feed3eacb2046ca72a` |
| `Source/AIHeroes/Public/AIHeroesCampaignSave.h` | 2754 | `bef80355acadf7aefaded7a1415e56cff8caefee6684fdbd650b5c04cbf58aea` |
| `Source/AIHeroes/Private/AIHeroesCampaignRules.cpp` | 8321 | `3235e1d14bb62510eba9c79c31600a5ec0640f163a3e519f2ae5a6f70f35d4b6` |
| `Source/AIHeroes/Private/AIHeroesBasePanel.cpp` | 7358 | `44ae07a904a22d628763e7cb3727c1e5d088da2fa07d25953843f7170292f3b0` |
| `Source/AIHeroes/Private/AIHeroesBasePanelGameMode.cpp` | 4944 | `c52e1fa1349c6bcce2c23d2f1e07e307c2d81a39d49b3a253a7f98d05a928d61` |
| `Source/AIHeroes/Private/AIHeroesBasePanelHUD.cpp` | 5235 | `e49e1513ef9baaa634dc6b744cb1b3cbd27df7351e207a71fc602ec6e64146c3` |
| `Source/AIHeroes/Private/AIHeroesBaseGameMode.cpp` | 2628 | `09faee3705ce9a4c599a6406b01889988e149908ef91900db02ac3cf48657fef` |
| `Source/AIHeroes/Private/AIHeroesRecruitmentAdvisor.cpp` | 9997 | `00bd101d9f94d5fcd57ca19dbc6fede5a910ee0f19052a3b608ab342720ac5ac` |
| `Source/AIHeroes/Public/AIHeroesAdvisor.h` | 1062 | `bfdea7710c9d18994381dc7e0aff2a8086dabe751b6d4793149806847ab9172c` |
| `Source/AIHeroes/Private/AIHeroesLiveBattleGameMode.cpp` | 12204 | `91e9aa42c8f350056d7164cd93892a760e87650ee1afe559154bc715168ed2ca` |
| `Tools/qa_recruitment_stock.ps1` | 5324 | `36ebd487a4987957c568264d27235b50f6c196ad323124f598721bcac7fd26b6` |
| `Saved/QA/TravelValidation_20261028T205203365Z/report/index.json` | 46177 | `5ac5a097ad6118ee25a839ce3ea87b29db1d79a6fd693214bff9aeeb954e0f2a` |
| `Saved/Logs/AIHeroesStockQA.log` | 196526 | `84c862dc984e11d173be1afe1b791c0f7a7a48a1c436886069c1487a3632a217` |
| `Saved/QA/RecruitmentStock/STOCK_EXHAUSTED_DAY1.png` | 123734 | `8a1d2072398a7586b185c0a9635e2a5562404c79d038b5e7e374d5eadd7f3472` |
| `Saved/QA/RecruitmentStock/STOCK_NEW_WEEK_DAY8.png` | 124051 | `0487fcad5a662ce90360edca2fae6f6e4d82a6df9c9d01a978c2e0d4de77256e` |
| `Binaries/Win64/AIHeroes.exe` | 332874240 | `34ba750c81cfdd7db86b1ef6c163907cddcceabc5c430e2fc5cd7ad3d1015a29` |
| `Binaries/Win64/UnrealEditor-AIHeroes.dll` | 1683968 | `4bb15acbc4d7b36fdb0004b11f8513e1f7f83143b6c00ce6e08124a4412e449b` |

## 5. Limitations and next action

**Not established**: Android device experience, packaged/distributed game, actual 1B/3B/8B inference, multi-faction town/dwelling economy, independent native touchscreen test, multiplayer server, full XP/magic/artifact systems. Older save compatibility was tested by optional-field migration, validation, and isolated GUID slots; **not every user's historical save slot was manually opened**. GUI test checks stock gameplay only at desktop 1280×720. In screenshot, BASE text is readable at that size, but mobile legibility needs physical-device QA.

**Next original-game P1:** create actual owned recruit dwelling objects as differentiated world actors, distinct from HQ supply and neutral guard growth, only after designing ownership/revert conditions and safe save migration. Integrate into deterministic graph pathfinding and fog-of-war, then benchmark resource/army/tempo trade-offs over many fixed scenarios. Keep human confirmation of all purchases and battles.

**Boundary:** A current Git staged/unstaged source tree contains parallel other-agent work. Do not treat this local test receipt as commit, merge, push, installer, published release or verified-live mobile behavior. Freeze current source/hashes and rerun all release QA before packaging.

**∆DΩΛ:** ∆ — finite weekly player reserves with owned outpost production bonus, inventory-aware advisor, atomic buys and legacy-safe save fields; D — official HotA docs → C++ source → Win64 Game & Editor builds → 117/117 Unreal tests → exact game GUI evidence, full SaveGame fingerprint comparison and per-file checksums; Ω — 95% for observed Windows integration, lower for mobile/unrun historical saves; Λ — any further source delta, Android install, external dwelling expansion or new save schema needs fresh build, tests and receipt.
