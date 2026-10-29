# AI HEROES — Zone Template v1 (seeded graph candidate)

Date: 2026-10-28
Status: ISOLATED PRODUCTION MODULE. It is NOT the active 3D campaign map.
Source context: original research in HOTA_181_GAMEPLAY_RESEARCH_2026-10-28.md
and the current, separately implemented AdventurePath graph contract.

## Research-based design decisions

[FACT] Horn of the Abyss's official template editor documentation states
that random maps are governed by a template describing the structure
and major features of the map. Its connection settings define permitted
zone links; standard connections can be guarded, wide ones are unguarded,
key-locked and fictional connections have different roles. Road rules
and zone-placement hints are separate from logical connectivity.

https://h3hota.com/en/template-format
https://h3hota.com/en/templates

[FACT] Epic's Unreal Engine 5.8 Procedural Content Generation (PCG)
framework supports point/attribute generation, graph operations, spawning,
editor inspection, runtime components and hierarchical world workflows.
PCG itself does NOT prove that a playable quest graph is reachable.
We therefore isolate game-topology validation as deterministic C++17
BEFORE any optional PCG visual/decorative generation.

https://dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-overview
https://dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-framework-in-unreal-engine

[FACT] Unreal Engine serialization versioning supports custom object
versions and conversions for backward compatibility. We do not change
existing save slots until their map schema/migration is designed, tested,
and explicitly approved for implementation.

https://dev.epicgames.com/documentation/en-us/unreal-engine/versioning-of-assets-and-packages-in-unreal-engine

[INTERP] The unique AI HEROES design must retain human strategic
authorization and AI tactical autonomy. Topology generation cannot make
an LLM authoritative over movement, attacks, or player save slots.

## Actual v1 algorithm (original implementation)

Portable production C++17 kernel:
  Source/AIHeroes/Public/AIHeroesZoneTemplateCore.h

Unreal adapter (uses the SAME portable code, not a second generator):
  Source/AIHeroes/Public/AIHeroesZoneTemplate.h
  Source/AIHeroes/Private/AIHeroesZoneTemplate.cpp

Shared, existing weighted path evaluator (reused):
  Source/AIHeroes/Public/AIHeroesAdventurePathCore.h
  Source/AIHeroes/Private/AIHeroesAdventurePath.cpp

The generator is a stable 32-bit-seeded deterministic template.
A candidate contains between 7 and 11 logical zones:
  HQ -> SUPPLY -> FOREST or ORE -> PASS -> OUTPOST or RUINS.
Optional WAY_01..04 create secondary roads to improve route choice
without bypassing a mandatory guarded checkpoint.

- Every node has canonical ASCII ID, typed role, integer grid position,
  bounded danger tier and guard flag; no terrain meshes are copied.
- Every bidirectional route is represented by two explicit directed
  links with the SAME positive movement cost, in 1..4.
- A fixed-width SplitMix32 function determines coordinate jitter, costs
  and some optional cross-links; it has no global RNG or clock.
- Hash is a reproducible 32-bit change detector over data. It is NOT
  cryptographic authorization, proof of provenance, or user consent.
- The original existing adventure path algorithm finds routes and forbids
  passing through undefeated hostile stops.
- In stage zero, the two resource zones are reachable independently.
  The checkpoint and late objectives remain inaccessible until one
  early guarded target is cleared.
- After the first guard has been cleared, PASS becomes an eligible
  guarded destination; clearing PASS makes both final objectives
  reachable. Optional zones have to be reachable eventually.
- Validation rejects unknown/duplicated nodes and roads, forged
  shortcuts, unpaired directions, malformed costs, position collisions,
  incorrect roles/guards/danger and hash/revision drift.
- No more than four optional branches; construction is bounded and
  failed attempts return an EMPTY candidate.
- The Unreal adapter validates 1:1 node metadata and directed edges.

The bDiscovered=true inside a generated graph is a VALIDATION ORACLE ONLY:
it proves eventual connectivity. A future player-view provider must
filter to discovered nodes and hide unknown routes/costs. Never present
this complete graph as discovered intelligence in the game.

## Acceptance gates

Native C++17 verification:
  powershell -NoProfile -File Tools/verify_zone_template_native.ps1

The script compiles the ACTUAL production header with installed MSVC,
using /std:c++17 /EHsc /W4 /WX and no package installations.
It covers 1,024 seeds x 5 branch counts = 5,120 generated candidate
worlds, guards, objectives, stable revisions, route reachability,
deep/early stops, negative tampering, fog-of-war concealment and
schema/size limits. It creates only Saved/QA verification artifacts,
never human SaveGame slots.

Unreal automation suites under AIHeroes.ZoneTemplate:
  RepeatableSeedAndRoles
  GuardedProgressiveReachability
  TamperedGraphFailsClosed
  SeedSweepAndBounds
  V1SaveNotSilentlyExpanded

The explicit legacy-save test enforces that v1 five-node campaign
validation rejects a 10-zone future draft rather than saving an
incompatible map silently. Versioned migration and A/B readback must be
designed before touching any production game slots.

Expected runtime stages:
  S0: deterministic graph candidate (THIS CHANGE)
  S1: seeded full-world metadata and reward budget verification
  S2: migration for future MapSchema v2 and deterministic seeds
  S3: render zones/terrain/roads with PCG, add selection and fog
  S4: player-confirmed action executor parity with AdventurePath
  S5: full save/load/replay regression and cross-platform QA

## Nonclaims and IP boundary

This is not the executable HotA RMG, map pack reader, town generator,
3D mesh renderer, fog-of-war, graph-driven human gameplay, or the
new dynamic save format. It neither downloads artwork or saves model
responses, nor uses characters/names/textures/sprites or music from HotA.
No AI model weights or Android APK are produced here. The active
five-node map and player saves are not altered by this module.

Rollback: remove ONLY these seven new dedicated files (core + Unreal
adapter + tests + native test runner + guide) after read-back and a
check for concurrent modifications. Do not reset the shared working
tree or overwrite other authors' work.
