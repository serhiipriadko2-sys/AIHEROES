# AI HEROES — Portable procedural campaign snapshot proposal (schema 2)

Date: 2026-10-29. Status: NON-AUTHORITATIVE EXPERIMENTAL SAVE CONTRACT.
It does not replace the playable fixed five-site campaign or the separately
developed ZoneExpedition game mode. The candidate is not a production slot.

## Scope / source of truth

The current AI HEROES repo has three DIFFERENT state surfaces:

1. Fixed classic campaign:
   UAIHeroesCampaignSave + FAIHeroesSaveSystem; five fixed nodes, schema v1;
   real resilient slots, existing gameplay, no automatic map migration.
2. Concurrent experimental procedural expedition:
   UAIHeroesZoneExpeditionSave + FAIHeroesZoneExpeditionRules +
   FAIHeroesZoneExpeditionStore; generated 7..11-site model with human
   confirmed actions, 8 daily moves, bitmask progress and its own A/B
   slot namespace. This is the MORE COMPLETE procedural gameplay path,
   and should be selected as future authority after independent QA.
3. This C++17 portable procedural snapshot *candidate*:
   AIHeroesProceduralCampaignCore.h and separate
   UAIHeroesProceduralCampaignSave. It persists canonical per-zone
   status and uses four daily moves, following the original classic
   default; it intentionally does not expose a public save slot, a
   GameMode, an action executor or an A/B store.

**Conflict: A vs B.** Both experimental procedural classes currently
store "schema 2" but are NOT wire-format or rules compatible.
ZoneExpedition uses a 32-bit explored/cleared mask, 8 moves/day and a
player-confirmation action pipeline. ProceduralCampaign uses named
zone records and 4 moves/day with only validation/projection.
The matching schema number never authorizes conversion between them.
No casting, merging, hot swapping, or silent migration is permitted.
An ADR and conversion QA are required before selecting one canonical
save layout or deleting the experimental alternative.

## Independent portable implementation

  Source/AIHeroes/Public/AIHeroesProceduralCampaignCore.h

Production Unreal bridge, distinct UClass and no fixed production slot:

  Source/AIHeroes/Public/AIHeroesProceduralCampaignSave.h
  Source/AIHeroes/Private/AIHeroesProceduralCampaignSave.cpp

Tests:

  Tests/procedural_campaign_native.cpp
  Tools/verify_procedural_campaign_native.ps1
  Source/AIHeroes/Private/Tests/AIHeroesProceduralCampaignTests.cpp

The code uses the existing AIHeroesZoneTemplateCore::Generate, which
is shared with the 3D preview. A SaveGame stores the seed, side-route
count, generator schema and topology revision but deliberately DOES
NOT serialize a second copy of the entire graph.

Every validation regenerates the canonical complete map and checks:
- exact generator schema, seed, route count and 32-bit revision;
- exactly the canonical 7..11 ordered stable zone IDs;
- fixed initial known set HQ, SUPPLY, FOREST and ORE;
- safe unguarded nodes, undefeated guards and owned territories;
- every claimed zone reachable from HQ without crossing living guards;
- hero stands only in owned discovered territory;
- saved selected destination is known to the player;
- action/staleness revision bounded; no cleared guards at revision 0;
- day/moves, Gold/Wood/Ore, AI protocol tier, model-capacity slot,
  DataFragments and save sequence within deterministic limits;
- only exact known friendly unit types/stats, at most two stacks;
- no IO or mutation on read-only validation.

BuildPlayerGraph derives a fresh graph containing ONLY discovered site
IDs and the connections for which BOTH endpoints are discovered.
Hidden node names, road costs and mission goals never leave this API.
Every action revision changes the proposal revision; this does not
authorize action, movement, recruitment or combat.

NOTE: The graph fingerprint is a deterministic 32-bit change detector,
not cryptographic authentication, and ActionRevision does not prove
human consent. A future server/host authority must independently check
all game actions. The actual playable expedition already has a separate
ConfirmPlayerDestination, which this candidate deliberately does not
replace.

## Legacy five-node boundary

AssessLegacy reads and validates UAIHeroesCampaignSave v1, then returns
either InvalidLegacy or RequiresNewCampaign. The latter explains why
a five-node old campaign CANNOT be safely expanded into ten seeded
zones without inventing battles, guards, fog, army travel and node
ownership. This method never opens a player SaveGame slot, writes to
disk, alters the passed legacy object or calls SaveGameToSlot.

Unreal disk tests, if successful, use:
  AIHEROES_ProceduralV2_TEST_<fresh GUID>
only. A try/fail path must never open or overwrite user v1 slots.
The test must delete its own temporary slot after read-back and verify
it is absent.

## Current external references (not hidden implementation claims)

Official Unreal Engine 5.8 saving guide:
https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine
Epic's SaveGameToSlot / AsyncSaveGameToSlot guidance:
async is recommended for interactive game saves, sync may suffice
for small paused snapshots and deterministic automated test fixtures.

Unreal Engine object versioning:
https://dev.epicgames.com/documentation/unreal-engine/versioning-of-assets-and-packages-in-unreal-engine
UE custom versions support FGuid-based serialization upgrades. This
candidate uses a standalone SaveGame UClass + explicit schema value,
NOT a registered FCustomVersion yet, and NOT a verified migration.

HotA official template editor:
https://h3hota.com/en/template-format
HotA distinguishes logical zone connections, guarded passages, and
roads. AI HEROES uses the general design concept with entirely
original source code, IDs, art and gameplay; it does not import assets
or template files from Horn of the Abyss.

## Acceptance

- Native production-header MSVC C++17: /std:c++17 /EHsc /W4 /WX.
  1024 seeds x five route counts = 5120 states.
  Negative cases: wrong versions, forged map revisions, inaccessible
  owned objectives, hidden target proposals, impossible resources,
  forged army, duplicate IDs, invalid selection, oversized movement.
- Unreal Editor build of the entire repository (including parallel
  authors' new modules): no C++/UHT/linker errors.
- Four Unreal tests under AIHeroes.ProceduralV2:
  InitialStateAndFog, RejectTampering, ScratchDiskRoundTrip,
  LegacyRequiresNewCampaign.
- Full pre-existing AIHeroes Automation tests: no regressions,
  fresh JSON report and zero skipped tests.
- Verify source bytes/SHA256, test report, and all artifact receipts.
- No edits to default GameMode, live v1 campaign SaveGame class,
  v1 rotating save system, active config, or user .sav files.

No real model weight inference, Android APK, generated PCG terrain,
multiplayer action authorization, production v2 writes, v1-to-v2
migration or deployment is implemented here.

## Recommended next architecture decision

Choose ONE future live procedural save contract after reviewing
ZoneExpedition's actor/HUD integration and its existing A/B store.
Create an ADR comparing the bitmask runtime to the canonical-ID
portable model: cost of 8-vs-4-movement semantics, migration,
visibility provenance, recovery, actions and cross-platform parity.
Do not merge the two v2 snapshots by simply matching their integer
schema field. Until ADR, label the per-zone candidate experimental.
