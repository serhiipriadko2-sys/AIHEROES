# AI HEROES вЂ” Zone Expedition offline adviser: verification receipt

Date: 2026-10-29 | Workstation: DESKTOP-ITUAMNF | Unreal Engine 5.8.
**Status:** LOCAL SOURCE + EDITOR BUILD + 4 TARGETED + 208 FULL TESTS + H/SHIFT+H GAME-WINDOW QA: PASS
Current shared Git HEAD observed: 
f96acd1
. No commit, push, merge or deployment was performed by this operator.

## AвЂ“C. Research, rule boundary and scope

Research basis: https://h3hota.com/en/template-format and
https://h3hota.com/en/templates. HotA zone templates separate roads,
guarded connections and zone placement. No HotA assets or code copied.
Unreal test procedures: https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unreal-engine
and https://dev.epicgames.com/documentation/unreal-engine/review-test-results-in-unreal-engine.
The advisor only uses the existing validated ZoneExpedition PlayerView;
it neither requests the full generated-world oracle nor touches the
five-node production GameMode, SaveGame class or player save slots.

## D. Changes

- New read-only FAIHeroesZoneExpeditionAdvisor with typed status,
  discovered-only target ranking and weighted legal movement cost.
- Tier SEED/ANALYST/LEDGER/PLANNER changes rule-based advice only.
  No enemy casualty forecast without actual evidence, no neural weights.
- H proposes a known destination; Shift+H inspects only selected zone.
- Enter still explicitly authorizes travel and any tactical battle.
- Integrated only into opt-in ZoneExpeditionGameMode + its HUD.
- Four focused Unreal tests: fog, no-orders, no-army/no-moves, 640 seeds,
  post-confirmed-battle read-only state equality with negative controls.

## E. Evidence and failure repair

- Frozen Editor Win64 Development DLL: 2500608 bytes, SHA256 559d072765c27520cbfc3944345bf53339ca772b036358f0a5fb293db19558e6.
- Compiled input manifest: 197 Source/Config/project entries.
- Compiled source manifest was checked against all six owned C++ source
  files at receipt creation; source SHA256 values match the tested build.
- Targeted: 4/4 tests PASS, with verified compiled-source provenance.
- Full: 208/208 Unreal Automation PASS, zero warnings/failures/skips,
  build_verified=true; runner report hash matches recorded provenance.
- One earlier test failed due to an invalid fixed discovered-count bound;
  it was replaced by full FAIHeroesZoneExpeditionState reflection equality,
  cleared-mask/movement checks and negative controls; the final run passed.
- Real 1280x720 game process launched with exclusive window identity;
  H suggested visible SUPPLY for 2 moves with no move/attack.
  Shift+H inspected FOREST for 3 moves and declared battle outcome UNKNOWN.
  Only subsequent Enter confirmed forest combat. Human Space advanced day;
  another Enter separately confirmed ORE combat. Two battles verified.
- Four genuine Unreal GPU screenshots, total bytes=1574835; each sample >8 distinct colors, source-copy hash matched.
- User save filenames, counts and SHA256 stayed identical before/after
  window playthrough; K/L and all SaveGame operations absent from test.
- Exact QA process window was closed gracefully, not force-killed.
- New PowerShell QA script parse errors=0; source secrets-pattern scan clear.

## Concrete byte/sha256 manifest

| Project-relative path | Bytes | SHA256 |
|---|---:|---|
| Source/AIHeroes/Public/AIHeroesZoneExpeditionAdvisor.h | 1454 | 4b7a8bc2c58a81adee284a69c970070eae27f1d04d860613610e3b39b424a730 |
| Source/AIHeroes/Private/AIHeroesZoneExpeditionAdvisor.cpp | 7853 | e1bebe1a94b92d97eeac19e41d3bc8918625c07c509e65cc8de465a92f2a0c4c |
| Source/AIHeroes/Private/Tests/AIHeroesZoneExpeditionAdvisorTests.cpp | 10606 | 8d0cf8f73cdb443d2323a0e5816ef12fe16314a6b6df331489f8878d53d2a159 |
| Source/AIHeroes/Public/AIHeroesZoneExpeditionGameMode.h | 2322 | aa8e777338f26c6b412fec1f6394c914958d49eb08e3d210c8fd71513db21b7a |
| Source/AIHeroes/Private/AIHeroesZoneExpeditionGameMode.cpp | 19475 | ab12b5e9bcf4395f36a41a676f83af27497a004caed1fa3357c89ed0a466a91b |
| Source/AIHeroes/Private/AIHeroesZoneExpeditionHUD.cpp | 2155 | 1a41a14a2e5fee88c32cd7fd9a06abd03ca577a72e2e9b3e9352059b14ae7c29 |
| Tools/qa_zone_expedition_advisor.ps1 | 10480 | 3c7821469eb93011064cc540f27920acada5d04357f1d373a4f9ee20197f4bd9 |
| Docs/ZONE_EXPEDITION_ADVISOR_V1.md | 7131 | d43d89603e8b6658a1e9eaba9c97aa2fc524572e37f3c112497f51c123d5f467 |
| Docs/BuildRuns/20261029T132326707Z/compiled.json | 54386 | 7c854c410fd5de669a5bede57f13fe226533ba1acc8b7be5182bd2e68026fcc9 |
| Docs/AutomationRuns/20261029T132355357Z/index.json | 2387 | cd92e518b0f2f12b26e0da0806d54c716b5eb05980156fe5816e1656aca6a217 |
| Docs/AutomationRuns/20261029T132355357Z/provenance.json | 54526 | d95a3c67787b5d37e8b3e4094547b20739c09aa2b8d9b4c25bc290843205f491 |
| Docs/AutomationRuns/20261029T132743622Z/index.json | 82505 | 3cc694fc0ca4940b5da1cd1f2f272f687c8db83abe1a26dcba7e2ba0fbf04548 |
| Docs/AutomationRuns/20261029T132743622Z/provenance.json | 54506 | 2762dbb3bd93149efce035f88ab403dc995233dd21b467d0fa99e4335e72b374 |
| Docs/VisualRuns/ZoneExpeditionAdvisor_20261029T133133956Z/initial.png | 370325 | 15e903c35a399985b6c574e109941eac5cb12087660547b5e965f20f6912ee1a |
| Docs/VisualRuns/ZoneExpeditionAdvisor_20261029T133133956Z/forest.png | 402722 | 4b1d3eee4ebe6d8cd32992af93c8892d6dad8eedc352c9454b75f7df595437c1 |
| Docs/VisualRuns/ZoneExpeditionAdvisor_20261029T133133956Z/day2.png | 397694 | 7b34cc37caee70481d14c1ec20afa05afca48fb83085298cd6ea216c3db00445 |
| Docs/VisualRuns/ZoneExpeditionAdvisor_20261029T133133956Z/ore.png | 404094 | 6d0306bec9e390d6b357cd323ac57acb730a5038865dce989c4969cdadd47dae |
| Docs/VisualRuns/ZoneExpeditionAdvisor_20261029T133133956Z/receipt.json | 3184 | 1a59cb2bb39ea612774265c58db92ea611649e28b0b84d30990bc8a8a4715852 |
| Saved/QA/ZoneExpeditionAdvisorVerified_20261029T133133956Z/UnrealEditor-AIHeroes.dll | 2500608 | 559d072765c27520cbfc3944345bf53339ca772b036358f0a5fb293db19558e6 |

## F. Limitations and rollback

PASS: editor-compiled local rules, full tests, real-game targeted input,
GPU captures and local artifacts verified byte-for-byte.
PARTIAL: turn-based heuristic adviser UX; no Android device QA,
no actual LLM inference or animated procedural Grid V2 battle stage.
NOT CLAIMED: Win64 packaged/signed build, GitHub push, production mobile
deployment, acceptance of the separate competing v2 SaveGame schema.
Rollback: only three advisor C++ files plus their surgical include,
H binding and HUD text edits; pre-edit copies are held under
Saved/QA/ExpeditionAdvisorBefore_20261029. Do not reset foreign changes.

в€†DО©О›: в€† read-only offline procedural hero advice with known-only
target selection; D official docs -> code -> edit-safe backups ->
4 focused tests -> 208/208 regression -> 2-key window QA -> SHA files;
О© <=95% for verified Windows-local development; О› reevaluate after
Android/input testing, schema convergence and genuine model integration.
