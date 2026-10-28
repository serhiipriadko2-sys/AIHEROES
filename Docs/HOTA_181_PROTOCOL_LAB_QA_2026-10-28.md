# AIHEROES — Horn of the Abyss 1.8.1 study + Protocol Lab implementation receipt

Date: 2026-10-28. Project: `C:\github\AIHEROES` (DESKTOP-ITUAMNF).
Status: **WINDOWS_EDITOR_BUILD_PASS | UNREAL_AUTOMATION_98_98_PASS | LAB_GUI_QA_PASS | SAVE_BYTES_UNCHANGED | HOTA_RESEARCH_CREATED | ANDROID_BLOCKED | LLM_NOT_CONNECTED**.

## 1. Scope and source hierarchy

This is a gameplay-research and original-game implementation, not a redistribution or clone of Heroes III/HotA. SoT30 project corpus (v5.5.7) was checked on the Windows machine: 30/30 Knowledge, 32/32 hashes, corpus 4,054,425 bytes, pinned manifest hash `0decd118a6300324b757d2587ec315a569aaf5800299d2b33bf046feaf56aeff`. The canonical project SoT was not edited.

**External [HOTA-FACT] sources**: official https://h3hota.com/ru/documentation, https://h3hota.com/ru/download, https://h3hota.com/en/template-format, https://h3hota.com/en/faq; release notes for version 1.8.1 (2026-08-25), plus classic-mechanic community references explicitly labeled as such. HotA creators do not approve use of HotA assets in other projects. No HotA art/music/campaign maps/copyrighted original content was copied into the game.

**Project [AIH-FACT] sources**: this working-tree's C++, Unreal tests, current binary and real window logs. Past chat or prior QA results do not override current source files. The analysis artifact is `Docs/HOTA_181_GAMEPLAY_AUDIT_AI_HEROES_2026-10-28.md`: it covers adventure exploration, combat/tactics/initiative, hero growth, town construction, weekly economy, mines, spells, artifact economies, creature dwellings, Cove/Factory/Bulwark, RMG zones, maritime modes, quests/events, AI, multiplayer templates and UI, each compared against existing AIHEROES code and an original feature plan.

**What the comparison says**: The dominant HotA strategic effect is multiple rhythms — movement/day, income/day, recruits/week, skill/battle decisions, city construction, defended passages, territory control and victory constraints. AIHEROES' distinctive loop is that a human authorizes strategic actions while the hero's combat policy acts autonomously and is auditable. Model slots are *gameplay themes*, not loaded neural weights.

## 2. Actually implemented: Protocol Lab

The new `FAIHeroesProtocolLabRules` accepts only a record verified by existing `FAIHeroesBattleRecordRules::Verify`. It reconstructs **four deterministic counterfactual battles** from the exact same army before-state under protocol tiers 0, 1, 2, 3. V1 uses legacy rules and V2 uses the GridBattle V2 rules. Each row shows an explicitly distinguished ACTUAL or WHAT-IF result, final army HP, outcome, number of rounds/actions and the first different action index. An invalid/tampered record is rejected and leaves no partial report.

The GameMode's new `OpenProtocolLab` is read-only. The report is exposed through existing modal dossier with action 203 (`LAB Y`) and keyboard `Y`; touchscreen/mouse use identical hit geometry. It is not accessible during live battle, historical replay or base purchases. Existing GAME/AI action methods do not receive any counterfactual output. No SaveGame schema, actor state, resources, game day, player army, camera or battle reward is changed by reporting.

The LAB dossier gets its own type flag so refresh keeps lab context; it does not insert the unrelated AI Proposal Gate paragraph into a counterfactual. No history is not a guessed outcome: it displays the explicit next step, *finish a guarded encounter*. The UI keeps the GOALS and other prior HUD actions.

**One actual scenario observed in the 1280x720 game**: FOREST on Day 1, Grid rules V2, ACTUAL TIER 0. All four tiers returned **WIN, player 116 HP, enemy 0 HP, two rounds, nine actions**. The three WHAT-IF policies first differed at action #3. This does **not** show better outcomes from an upgrade in that particular fight. That is an important negative result, not a hidden product claim.

## 3. QA and receipts

- Editor Win64 Development built under Unreal Engine 5.8 using serial low-memory UBT `-NoUBA -MaxParallelActions=1`. UnrealBuildTool result **Succeeded**. This was the most recent successful integrated Editor build before running tests and game QA.
- Fresh *full* Unreal Automation Framework result: `Docs/AutomationRuns/20261028T203357532Z/index.json`, **98/98 success, 0 failure, 0 notRun, 0 warnings**. Script `Tools/run_aiheroes_automation.ps1 -Filter AIHeroes` returned exit code 0.
- Five **new** tests all PASS: `AIHeroes.Lab.GridV2FourTierCounterfactuals`, `AIHeroes.Lab.LegacyV1Compatible`, `AIHeroes.Lab.RejectUnverifiedHistory`, `AIHeroes.Lab.RepeatableNonMutatingReport`, `AIHeroes.Lab.KeyboardAndTouchLayout`. Last test covers 1280×720, 960×540, 640×720, 480×800 layout geometry, action ID 203, no overlap and no use during live battle.
- A **fresh dedicated** Unreal `-game -log=AIHeroesProtocolLabQA.log` window (test PID 57068, client exactly 1280×720) was operated through `PostMessage` directed to its verified HWND, **not through global keyboard/mouse**. It produced: action203, `OPEN verified=0 variants=0` with no history → player-clicked FOREST → Enter explicitly authorizes a real V2 battle → one `[AI HEROES LIVE] COMMIT node=FOREST actions=9` → Y opens `OPEN verified=1 variants=4 history_version=2 no_order=1` → NEXT displays `PAGE 2/2` → close/reopen reads verified history again with no duplicate combat commit.
- Window QA script `Tools/qa_protocol_lab.ps1` exited 0: `LAB_QA_PASS`, `LAB_SAVES_UNCHANGED_PASS`. A pre/post complete SaveGame file inventory (file name, byte count, modified UTC ticks and SHA-256) matched exactly.
- All three PNG screenshots (no-history report, first page of valid 4-tier report, second page of same report) were created and examined visually. They show readable modal text and accessible NEXT/PREV/REFRESH/CLOSE buttons. The second page's first line is a carryover from line-based pagination; this is a UX polish opportunity, not a hidden missing policy. These are **scripted desktop GUI tests**, not human touchscreen tests.
- The test's own window was gracefully closed after evidence was captured; concurrent other-agent QA windows were not terminated. `git diff --check` exit 0. **No Git commit, push, merge, deployment or Android build** was performed by this operator.

## 4. Frozen item/byte/hash evidence

These are file hashes from direct Windows read-back after the latest QA execution. Items listed from project root `C:\github\AIHEROES`. They prove exact bytes **at that observation time**, not future invariance under parallel writers.

| Project-relative artifact | Bytes | SHA-256 |
|---|---:|---|
| `Docs/HOTA_181_GAMEPLAY_AUDIT_AI_HEROES_2026-10-28.md` | 34147 | `0b89c2bcd7b682d3ad35e8be757b83040a478872c4171a6fd8b40354b4b9c72d` |
| `Source/AIHeroes/Public/AIHeroesProtocolLab.h` | 1423 | `15f3cfcd3a339e0dd772dee30126d91bf862a6cd4c7c16da227652ac2ad3e0ac` |
| `Source/AIHeroes/Private/AIHeroesProtocolLab.cpp` | 5699 | `3582955e2ecbf5aa11e937127ef6148c487206bb12b8d74493bdbe7b9a83984c` |
| `Source/AIHeroes/Private/AIHeroesProtocolLabGameMode.cpp` | 1869 | `9ce224a7aabed715ff44c7b3f2684381a6a7aeab738c297c31c46b20225a6fc3` |
| `Source/AIHeroes/Private/Tests/AIHeroesProtocolLabTests.cpp` | 11238 | `b979008fa387ff144533176fbbfe39aa7d0567fb4c1719ac8900235b73582db9` |
| `Source/AIHeroes/Private/AIHeroesAdvisorDossierGameMode.cpp` | 3828 | `3fa68eb3b4ff7a46bb882c54ebee31258d33e55bbc8613767720fd11b3b85ef7` |
| `Source/AIHeroes/Public/AIHeroesAdvisorDossier.h` | 1875 | `c9e87299d3b452e8db5666621506f864a35d27dae65cada9dba82cfbe8b40767` |
| `Source/AIHeroes/Public/AIHeroesGameMode.h` | 8386 | `3395eab17652c871795bcfb6072a71e350e48a23eec397775257e1f91a1866da` |
| `Source/AIHeroes/Private/AIHeroesHUDLayout.cpp` | 4864 | `b486ee46a62a63aaeb6fc2d9c5e0c09525ece409d1fc3c41f42ef4b45c15c61c` |
| `Source/AIHeroes/Private/AIHeroesHUD.cpp` | 18263 | `30e6e0cb327e7d5e7b523318571dea544475eb9005a66e0c75414768a013cf5e` |
| `Source/AIHeroes/Private/AIHeroesPlayerController.cpp` | 13089 | `3db87f04865566b3a4fc8618d52793c3e708c60088e1cfa917c274acd7006c5c` |
| `Source/AIHeroes/Public/AIHeroesPlayerController.h` | 1295 | `0be8e625986d1bb6891898b9631e280563206e0aff23a59fc28fa4d92a845142` |
| `Tools/qa_protocol_lab.ps1` | 6257 | `2f474a8975bbfa73cab02a36271bdc606809a508bdce7822126e71bc196cefbb` |
| `Docs/AutomationRuns/20261028T203357532Z/index.json` | 38670 | `76820cc06257fb6e2bd521e81fc648115f7982903b7fa710c5e0e63d6caa9744` |
| `Saved/Logs/AIHeroesProtocolLabQA.log` | 195739 | `60d6fef3c643508df2f9efd3f94088d2d223d2132cae9b55cab59448c7ecd201` |
| `Saved/QA/ProtocolLab/LAB_NO_HISTORY.png` | 82072 | `5c85221f0a58d316ccdca5a08c319ad711e29c3f1c0304af7dd9451df31ff761` |
| `Saved/QA/ProtocolLab/LAB_FOUR_PROTOCOLS.png` | 88606 | `cd58f761d32fa5736666aa34bf1e1da3b2a3d937fbd6d539ec93ac6ca85bfa66` |
| `Saved/QA/ProtocolLab/LAB_FOUR_PROTOCOLS_PAGE2.png` | 85653 | `d6c105643d958c248557190d1b63c0c416876612c4cb6e4f322e1eea2ec22f81` |
| `Binaries/Win64/UnrealEditor-AIHeroes.dll` | 1264640 | `4f7156f2309fafa6ba3b1fe2ed865b5616d82bec35f9b77de7f9f3d87fd1d669` |

## 5. Remaining development and test gates

1. **Weekly economy and diplomacy**: The concurrently developed `FAIHeroesWorldWeekRules` currently reinforces *unowned neutral defenders*. It is not the same mechanic as HotA's player-owned dwelling stockpiles. A future separate player-recruit reserve needs validation, weekly rollover, game-order authority, SaveGame migration, old-save fallback and a human-visible production preview.
2. **Adventure path and rival AI**: Other concurrent modules implement deterministic graph exploration and raider turns; validate coherent path budgets, fog-of-war leaks and resource limits against the real map UI. Do not claim the opponent is a full HotA-level strategic AI.
3. **Cognitive evidence**: A policy change at action #3 with unchanged HP means distinct choices, not proven superiority. Build stratified benchmark scenarios (defender types, terrain, armies, seeds) and publish outcomes/losses/travel cost with uncertainty and claim ceilings.
4. **Experience/skills, towns, magic and events**: implement proprietary names, units, spells and visuals, versioned pure rule contracts, save migration and regression tests. Do not import HotA assets or proprietary content.
5. **Android/inference**: Android SDK API35, Build Tools35.0.1 and NDK r27c were missing at prior host audit; **no APK** or device QA here. GPU capacity reading is not model-ready; no verified weights, inference adapter, license or opt-in, so 1B/3B/8B labels remain fictional slots.
6. **Release**: Windows Editor build + automation + bounded GUI smoke does not prove packaged PC game, Android package, online play or production deployment. Multiple other agents change this working tree; freeze and re-run QA before any commit/release.

## 6. Final state

**PASS:** official HotA gameplay research artifact; new Protocol Lab source; Editor build; 98/98 fresh Unreal tests; actual 1280×720 GUI with 2-page verified lab; no repeated combat authorization; all campaign saves untouched; 19 artifact hash readbacks.

**PARTIAL/NOT CLAIMED:** manual mobile touchscreen testing, FPS/thermal/battery metrics, genuine neural inference, Android build, packaged release, third-party IP licensing, future external resource downloads. No source-of-truth/canon files changed.

∆DΩΛ:
∆ — reference-backed HotA system map converted into original AIHEROES 4-tier counterfactual lab with typed and verified historical evidence.
D — SoT integrity → official documentation → local code → Editor compilation → Automation JSON 98/98 → exact-window replay with no save mutation → byte/hash manifest.
Ω — 95% for verified local test and artifact results, lower for non-tested mobile/LLM and future gameplay balance.
Λ — re-run build, 98+ tests, PNG QA and hashes after any parallel source change, new HotA release or new battle rules.
