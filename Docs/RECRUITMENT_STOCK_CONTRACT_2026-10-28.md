# AIHEROES — Player Recruitment Stock, weekly economy v1

Status: implementation and verification in progress, 2026-10-28. Project: `C:\github\AIHEROES` / Unreal Engine 5.8.

## Why

The official Horn of the Abyss v1.8.1 documentation states that player-owned external creature dwellings accumulate their availability between weeks, while unowned dwellings reset to one week's supply. **This document does not copy HotA's units, quantities, prices, artwork or external dwelling configuration**. The original AIHEROES economy had day-by-day mines and a seven-day neutral-guard growth, but recruits could previously be bought indefinitely if the player had currency.

Primary reference: https://h3hota.com/ru/documentation ; version verification: https://h3hota.com/ru/download ; copyright boundary: https://h3hota.com/en/faq.

## Original AIHEROES rule contract

- The initial headquarters stocks **10 Militia and 4 Scouts**, tracked as individual recruit units (not purchase packs). An ordinary player purchase uses **5 Militia** for 50 G + 5 W or **2 Scouts** for 70 G + 4 O.
- Start of the next week (exactly day **7→8**, **14→15**, and every other valid seventh-day boundary): HQ stocks accumulate **5 + 2×BaseTier Militia** and **2 + floor(BaseTier/2) Scouts**.
- A **captured OUTPOST** adds **+2 Scouts/week** to the shared HQ recruiting reserve. This is an original ownership/supply bonus, *not* a claim that OUTPOST is an actual external creature dwelling. An unowned OUTPOST gives no player recruits; its neutral guard growth and NPC activity remain separate.
- Stock caps: **250 Militia** and **120 Scouts**. Growth saturates at cap. On any other day transition, the stock is unchanged. The day and HQ tier must be valid before any inventory mutation. There is no catch-up production for older campaign saves.
- Purchases are still explicitly authorized by the player. A stock check happens **before** the resources are charged; affordability and army 500000 HP cap are separately checked. A failed transaction changes neither stock, currency nor player army.
- The **BASE** offer quotes show current availability and block depleted packs. The **N** recruitment advisor has access only to typed read-only stock counts, and never recommends unavailable purchases.
- GameMode's `EndTurn` independently preflights neutral-defense growth (`WorldWeekRules`), rival turn (`RivalStrategy`), resources and player-recruit reserve before committing the next day. One failed preflight stops the entire day; no partial income or recruitment growth.

## Save migration and anti-corruption

`UAIHeroesCampaignSave` retains schema version 1; the new data consists of *optional tagged UPROPERTY(SaveGame) fields*:

```cpp
bool bRecruitmentStockInitialized = false;
int32 MilitiaReserve = 0;
int32 ScoutReserve = 0;
```

- Prior v1 saves omit all three fields, so validate as `false/0/0`; on load, GameMode creates only **one starter reserve** in memory. No older A/B or legacy file is rewritten as part of loading.
- New writes require marker `true` and values inside `0..250` and `0..120`. Marker `false` with any nonzero value is invalid; neither extreme nor negative values enter a validated campaign.
- Rotating two-copy SaveSystem read-back now compares all three new fields semantically. A bad newest slot is rejected and the last valid one is the recovery candidate.
- The live-battle preflight fingerprint includes both reserve numbers. Any stock mutation during a running fight invalidates the pending battle commit rather than granting a stale receipt.
- Only owned QA prefix GUID slots can be written/deleted by automation tests; real `Saved/SaveGames` paths and slot A/B files must remain untouched by GUI smoke tests.

## Scope / non-goals

This is a **minimal original recruit-stock economy** with an owned-outpost supply bonus. It does **not** yet implement captureable physical external dwelling actors, town-specific production trees, multiplayer arbitration, hero XP or models. No AI may make a purchase, and no real LLM (1B/3B/8B) is connected. Android SDK/API 35, Build Tools 35.0.1 and NDK r27c require separate installation and device QA.

## Required verification

1. Serial Editor Win64 Development build after no concurrent Live Coding process holds the target DLL.
2. Full `AIHeroes` Unreal Automation (including six `AIHeroes.Recruitment.Stock...` tests), 0 failed, 0 skipped.
3. Real 1280×720 game: buy two packs each, third pack refused without spending, verify day 7→8 restocking and another accepted purchase.
4. GUI test checks all preexisting real saves by filenames, bytes, timestamp and SHA-256 before and after without pressing Save.
5. Cross-check the current C++ hashes, test JSON report, screenshots and a separate QC receipt. Re-run after any parallel source modifications.

Verified on Windows 2026-10-28: **Win64 GAME BUILD PASS; Editor BUILD PASS; UNREAL AUTOMATION 117/117 PASS (six Stock tests); REAL 1280x720 GAME QA PASS; PLAYER SAVES UNCHANGED.** Full per-file SHA-256, screenshot and console receipts: `Docs/RECRUITMENT_STOCK_QA_2026-10-28.md`. This is not an Android, packaged release or deployed checkpoint acceptance.
