# AI HEROES вЂ” Procedural campaign v2 candidate: verified receipt

Date: 2026-10-29. Workstation: DESKTOP-ITUAMNF.
Local HEAD at read-back: cc062ba. No commit/push/deploy by this operator.
**Status:** source-written, C++17 native PASS, Unreal editor build PASS,
scratch serialization PASS, full Unreal Automation PASS, file QC PASS.
**Lifecycle boundary:** candidate only, NOT merged, not active GameMode,
not the production ZoneExpedition SaveGame, not Android or real LLM.

## AвЂ“C. Research, collision and decision

Existing fixed five-site UAIHeroesCampaignSave stays schema v1.
Concurrent ZoneExpeditionSave is a separate, independently implemented
schema-2 binary with eight movement points, bitmask progression and
a different A/B namespace. This new ProceduralCampaignSave stores
named site records and four moves/day. Identical schema integer does
NOT mean identical binary format or a valid automatic converter.
PROPOSED ADR (not accepted):
Docs/ADR/ADR-20261029-01_PROCEDURAL_SAVE_CONVERGENCE_PROPOSED.md

Evidence from actual installed Unreal Engine 5.8 and official docs:
- https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine
- https://dev.epicgames.com/documentation/unreal-engine/versioning-of-assets-and-packages-in-unreal-engine
- https://h3hota.com/en/template-format
The current module uses standalone USaveGame + schema marker, not a
registered GUID custom version or HotA-derived code/assets.

## D. Implemented isolated candidate

- Pure C++17 graph regeneration and 32-bit revision check, 7вЂ“11 zones.
- Stable canonical IDs and bounded friendly army/resource/protocol state.
- Guards/owned/hero location cross-checked against cleared routes.
- Read-only filtered graph excludes all hidden site IDs and road costs.
- No live save namespace, player-action execution or v1 conversion.
- Legacy classifier: valid v1 -> RequiresNewCampaign, invalid -> InvalidLegacy.

## E. Verification

- MSVC C++17 /W4 /WX: 5120 seeded snapshots, 41039 assertions; PASS.
- Unreal targeted Automation: 4/4 PASS.
- Unreal full Automation: 154/154 PASS, warnings=0, failed=0, notRun=0.
- New v2 Unreal tests: 4/4 PASS.
- Concurrent ZoneExpedition tests present in full suite: 6.
- Single GUID-scoped scratch SaveGame roundtrip, exact semantic read-back,
  validation after reload, rejection of corrupted loaded record.
- Temporary ProceduralV2 SaveGame slots remaining: 0.
- Original v1 player slots: not read, written, migrated, or deleted.
- Frozen tested DLL: 2022912 bytes, SHA256 f6edd6813c115d4e9689b6c3f928240c258d7bd3bdd5a83a5423d14cdc2eca9a.
- Frozen DLL SHA256 matched before and after the 154-test run.
- Owned source files were hash-checked before and after that run.
- QA script PowerShell parse errors=0; obvious secret scan findings=0.

## Concrete file manifest (bytes + SHA256)

| Project-relative path | Bytes | SHA256 |
|---|---:|---|
| Source/AIHeroes/Public/AIHeroesProceduralCampaignCore.h | 8894 | 1f7e09299b053bfa24cf4e747dbedb9d3f547e7937204abf31e8eeff663147c1 |
| Source/AIHeroes/Public/AIHeroesProceduralCampaignSave.h | 3391 | 89df791476ad77eb8b467c598c5f4e6009beacf67e73d48b013ed5823541e725 |
| Source/AIHeroes/Private/AIHeroesProceduralCampaignSave.cpp | 8075 | 9507376768d69a8167eafee7227eec0418753ab538639d775c93b5ab73dea28b |
| Source/AIHeroes/Private/Tests/AIHeroesProceduralCampaignTests.cpp | 12132 | 7cbbd71ee8cf6bcfa87fb30dfcafbf6d49eaadb3abcbc987d99bd3aa6803b069 |
| Tests/procedural_campaign_native.cpp | 9924 | 9294788af90ceb842c78cb921d75894816d67ec4726081f59f5ed854b009c306 |
| Tools/verify_procedural_campaign_native.ps1 | 3215 | ccb2073cf210b9c763663c83ca095290750f6b55787e22d8252ab19b192c35e2 |
| Docs/PROCEDURAL_CAMPAIGN_V2.md | 7404 | 61c58f039c8e335a3642bdd6a728bb7054028f8cbb1c7bfe36884b032eeff183 |
| Docs/ADR/ADR-20261029-01_PROCEDURAL_SAVE_CONVERGENCE_PROPOSED.md | 5966 | b656af8332d9af12ba7919099fbae3d08f6f44fd713184e9fb599727f89fef3f |
| Saved/QA/ProceduralV2Native_20261029T061012903Z/receipt.json | 1839 | 50c876d65a697d66c2e3423b3fe33cc894a6256ac0be2b720816b150c1627146 |
| Docs/AutomationRuns/20261029T061419237Z/index.json | 2277 | 13a16eccbd9132dd7053d9774a4e04e76c0a16cd6d489e966b2b08c80a9bac52 |
| Docs/AutomationRuns/20261029T061511227Z/index.json | 60979 | a3b7defb01eabbb8f24d4083ba55f52e5ad25bee5ef2ec215a5f7fb16d22673b |
| Saved/QA/ProceduralV2Verified_20261029T061511227Z/UnrealEditor-AIHeroes.dll | 2022912 | f6edd6813c115d4e9689b6c3f928240c258d7bd3bdd5a83a5423d14cdc2eca9a |

## F. Closure and limitations

PASS: owned files, portable C++17, 154/154 Unreal tests, scratch roundtrip.
PARTIAL: two independent schema-2 proposals require source-level reconciliation.
NOT RUN: live procedural GameMode wiring, touch/Android packaging,
player-facing production saves, authorized migration, real model weights.
Do not promote the experimental per-zone format to production without
an accepted ADR and interoperability tests against ZoneExpedition.

О”DО©О›: в€† separate portable, validated procedural snapshot candidate;
D native 5120 states -> Unreal compile -> 4 scratch/negative tests ->
full 154 tests -> verified hashes and proposed ADR;
О© в‰¤95% for observed development evidence; О› review after
acceptance of a single v2 save format and manual cross-platform QA.

