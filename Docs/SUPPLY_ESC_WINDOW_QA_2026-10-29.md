# AIHEROES — Safe Convoy Esc: Win64 Editor and Real-Window QA Receipt

Date: 2026-10-29. Host: DESKTOP-ITUAMNF. Project: `C:\github\AIHEROES`.
Lifecycle: **LOCAL SOURCE VERIFIED | UNREAL EDITOR TESTS 181/181 PASS | REAL-WINDOW X/ESC QA PASS | HUMAN SAVES UNCHANGED | STANDALONE GAME BUILD PENDING | NO COMMIT/PUSH/DEPLOY BY THIS OPERATOR**.

## Scope and provenance

- This evidence follows merged GitHub PR #1, https://github.com/serhiipriadko2-sys/AIHEROES/pull/1, which adds fail-closed Esc cancellation and stale-convoy invalidation. The observed local Git HEAD was `8a12d0e`, with additional unrelated concurrent uncommitted source changes. PR merge is **not** proof of a verified game build.
- Game principle only: official Horn of the Abyss documentation https://h3hota.com/en/documentation describes weekly dwelling supply; this convoy system, escort surcharge, troops and map geometry are original AIHEROES design, not HotA asset reuse.
- Unreal Engine 5.8 automation instructions: https://dev.epicgames.com/documentation/unreal-engine/run-automation-tests-in-unreal-engine. Separate pure C++/Editor tests from actual Win64 window interaction.

## Verified build and full automation

- Unreal Editor Win64 Development source build **Succeeded**, using serial `-NoUBA -MaxParallelActions=1`; tested DLL bytes and SHA-256 are recorded below.
- Fresh full Unreal Automation report `Docs/AutomationRuns/20261029T120315037Z/index.json`: **181/181 success**, failed=0, notRun=0, succeededWithWarnings=0. This includes seven `AIHeroes.Supply.*` tests, three `AIHeroes.InputBindings.*` tests, and four `AIHeroes.LiveBattle.Readability.*` tests; each succeeded.
- SoT30 validation: `SOT30_INTEGRITY_PASS version=v5.5.7 knowledge=30/30 sha256sums=32/32`, pinned manifest `0decd118a6300324b757d2587ec315a569aaf5800299d2b33bf046feaf56aeff`. Project canon unchanged. `git diff --check` exit 0.

## Dedicated owned-window gameplay verification

- A unique non-shipping fixture (`-AIHeroesSupplySmoke`) started `UnrealEditor.exe -game -windowed -ResX=1280 -ResY=720 -log=AIHeroesSupplyEscQA_20261029c.log`; owned process PID **47360**, actual 1280x720 client. Only this PID received targeted Windows keyboard PostMessage for **X (0x58)** and **Esc (0x1B)**. No global pointer/keyboard manipulation; no SaveGame write command.
- The starting fixture has HQ, FOREST and OUTPOST already secured, forest camp stock 10, gold 240, wood 35, ore 30, movement 4. These are disposable development-only fixtures, not real campaign rewards.
- Step 1: X showed exactly one **read-only** quote of five Militia for 65 gold, five wood and one movement point; no dispatched supply.
- Step 2: Esc canceled quote with `commit=0`; visible screen still shows gold 240 / wood 35 / movement 4 and Militia 8.
- Step 3: X required a new independent quote rather than committing a previously canceled purchase.
- Step 4: second X dispatched **one** forest pack (`commit=1`): gold 240->175, wood 35->30, moves 4->3, army Militia 8->13, field stock 10->5. ORE and Headquarters recruiting stocks remained separate.
- Step 5: one new X produced only another quote; a final Esc canceled it. Total events observed in game QA: three quotes, two cancels, **exactly one dispatched convoy**.
- Existing SaveGame inventory was compared before/after by file name, size, modification-time ticks and SHA-256; script produced `SUPPLY_SAVEGAMES_UNCHANGED_PASS`. The isolated window was gracefully closed after recording evidence.
- Script result `SUPPLY_GUI_PASS` and process exit `KEYBOARD_ESC_GUI_EXIT=0`. Five PNGs were created; `CONVOY_CANCELED.png` and `CONVOY_DELIVERED.png` were additionally examined visually, confirming resource/army/field changes.

## Test harness correction (transparent failure history)

- First preliminary QA was blocked because its process used a split `-log= tag` argument; the PID/log exact-match guard correctly refused that window. Own window closed, never global input.
- Second preliminary QA used targeted WM_MOUSE messages. Unreal reads its mouse position via `GetMousePosition()`, which cannot be changed by posting mouse lParams, so the button action was not invoked; failure was in the test harness, not proof that the real UI cannot be clicked.
- The **successful fresh test** uses direct X/Esc key events addressed to the owned window. Native mouse and physical touchscreen action remain outside the claim; Unreal Automation tests independently verify that the UI hitboxes and action205 are non-overlapping.
- The script was improved to check two Esc cancellations, no purchase on re-quotation and a preserved player SaveGame inventory. Each unique log tag creates its own screenshots directory; previous failed-test evidence remains available for inspection.

## Frozen byte/hash manifest

Every item is project-relative to `C:\github\AIHEROES`, read directly from Windows after the window exited. Values describe the observed local state at that instant; parallel developers may subsequently change source files.

| Path | Bytes | SHA-256 |
|---|---:|---|
| `Tools/qa_supply_convoy.ps1` | 7142 | `b3e8180e84e48311c822fc7ea776e597503c4777f1087097e72989e2c3a9f9bd` |
| `Source/AIHeroes/Private/AIHeroesSupplyGameMode.cpp` | 6831 | `fa288cbbbc58415fbc2b518f185ae91716efe34210e00319a3d9ebedfddb322e` |
| `Source/AIHeroes/Private/AIHeroesPlayerController.cpp` | 14477 | `53a5229712227eb86b696d85f2fa3977688dfe9337c53a8539384b960d2d300a` |
| `Source/AIHeroes/Private/Tests/AIHeroesSupplyLinesTests.cpp` | 19536 | `d4bd4ee5970514ae79faab3236a6b292e387c32dcd9a2e1d92af6c01396b6d18` |
| `Binaries/Win64/UnrealEditor-AIHeroes.dll` | 2222592 | `efc0d1ab50d0b2d367e9f850daeb18a29c896a958d4a1df83bfcfaf6c0e9b1fd` |
| `Docs/AutomationRuns/20261029T120315037Z/index.json` | 71799 | `cf8e2b3affc734345153eab696343a6a55ec7d23d92958dff3b02afd50d458f9` |
| `Saved/Logs/AIHeroesSupplyEscQA_20261029c.log` | 195876 | `ab6c745b03cd3894685d77a21f71ec1610bd9f1e32ce3c49a7de54481c1474a5` |
| `Saved/QA/SupplyConvoy/AIHeroesSupplyEscQA_20261029c/CONVOY_BEFORE.png` | 362324 | `51189805d30e8924be26fc0e0d7893dd1511fb6dc825834658408ac142f404fb` |
| `Saved/QA/SupplyConvoy/AIHeroesSupplyEscQA_20261029c/CONVOY_CANCELED.png` | 357961 | `057545320101affea87ade34ebb877f4ba7b8cab3eadfb509ebad0419443a428` |
| `Saved/QA/SupplyConvoy/AIHeroesSupplyEscQA_20261029c/CONVOY_DELIVERED.png` | 361562 | `b4c1bde988bed53586da96d39d944c61e4289645991b9a9da2e2e1d98f105eec` |
| `Saved/QA/SupplyConvoy/AIHeroesSupplyEscQA_20261029c/CONVOY_QUOTE.png` | 361512 | `963383017af4c4c5410f79b499d1daf1acabbc1c30a3dc34c1ed32e5edf3a709` |
| `Saved/QA/SupplyConvoy/AIHeroesSupplyEscQA_20261029c/CONVOY_REQUOTED.png` | 363019 | `b495182ec30a2cba23a164281663860af4954d306d3c336aaa84a93ba4a51049` |

## Status boundaries / next gate

**PASS** for current Editor/Automation 181/181, actual 1280x720 X/Esc gameplay and exact pre/post human saves comparison. **PENDING**: refresh `Binaries/Win64/AIHeroes.exe` because its last observed build timestamp is 2026-10-28 21:22:48 UTC, older than this source; a separate `AIHeroes_v2_1_review` Unreal Editor build is running on the same memory-constrained host and must not be terminated or overlapped. A future Win64 Game rebuild requires new byte/hash read-back and no competing UBT process.

**NOT CLAIMED**: physical Android touchscreen testing, signed Windows installer, Android APK, multiplayer-server correctness, genuine 1B/3B/8B model weights/inference, shipping deployment or Git commit/push by this operator.

∆DΩΛ: ∆ — end-to-end Esc cancels actual convoy authorization and only a fresh second X purchases; D — current source and official UE documentation → fresh Win64 Editor build → 181/181 Unreal tests → exact-game-window screenshot/log/save QA → hash manifest; Ω — 95% for these observed local Windows QA results; Λ — re-run after source drift or when isolated Game Development rebuild is available.
