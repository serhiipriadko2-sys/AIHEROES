# ADR-20261029-01 — One authoritative procedural campaign save contract

**Status:** PROPOSED — not accepted, not implemented as a unified format,
not merged, not deployed, not verified-live.

**Scope:** AI HEROES gameplay/application architecture only; does not
modify Iskra SoT30 canon, Project Instructions, connector routing or
agent-memory policy.

## Context [FACT]

The repository currently has two *separate* experimental procedural
schema-2 SaveGame classes, created during concurrent development:

A. UAIHeroesZoneExpeditionSave (ZoneExpeditionRules/Store), which has
a generated 7..11-site world, progress bitmasks, eight movement points
per day, deterministic encounters, human-confirmed actions and a
dedicated A/B save namespace.

B. UAIHeroesProceduralCampaignSave (ProceduralCampaignCore/adapter),
a portable C++17 + Unreal validator with canonical ordered per-zone
records, four movement points per day, a typed friendly army, strict
fog projection and no production save slots or action executor.

Both reconstruct the same *generator* from seed and topology revision,
but their state semantics are not equivalent. Integer SchemaVersion=2
does not indicate a compatible serialized binary, movement policy,
fog provenance, battle history or update contract. The existing
classic five-node UAIHeroesCampaignSave v1 remains the active campaign
and is incompatible with either format without explicit migration.

## Decision proposed, not adopted

Prefer A, the separate ZoneExpedition action/SaveGame system, as the
future **gameplay authority** after its complete native/Unreal QA.
Keep B as a disposable, isolated **cross-platform validation
reference**, not another live slot or production serializer.

Before selecting a canonical v2 format:

1. Freeze a single schema/serialization specification for the
   dynamic campaign, including movement budget, army, progress,
   human order confirmations, fog reveal, save sequence, journal,
   generator revision, and versioned conversion policy.
2. Map the selected fields explicitly between Unreal and portable
   C++17 data. Any non-lossless field blocks conversion, with an
   explanation instead of a guessed default.
3. Audit existing A/B write and read-back recovery, corrupted-latest
   fallback, zero-authority stale preview, and valid-but-tampered
   enemy/capture state. Use only new GUID test slots until accepted.
4. Add game-mode integration only after a validated action
   replay contract and a gameplay UX gate.
5. Preserve old v1 user snapshots. Never alter v1 slot names or
   rewrite legacy data merely to inspect it.

## Alternatives

- Adopt per-zone portable B as authority immediately:
  rejected for now because B lacks a player-confirmed action system,
  save rotation and the 8-move expedition rules. Would create two
  incompatible procedural play loops.
- Merge A and B automatically based on schema number:
  REJECT: unsafe, ambiguous, likely to corrupt progress.
- Delay any v2 state work:
  possible but loses deterministic validation and cross-platform
  test coverage now already available.

## Consequences / price

Positive: one future world authority; stable deterministic topology;
explicit legacy boundary; portable contract testing; less false
confidence about v2 compatibility.

Cost: a written conversion matrix and acceptance tests; a future
consolidation commit; higher short-term code size while experiments
coexist. No production migration is executed by this ADR.

Risk: allowing two types named v2 to coexist indefinitely would
invite accidental wrong-class loading, incoherent movement balances
and stale proposal authorizations. Restrict both to their unique
UClasses and never share slot namespaces.

## Tests / QA / acceptance

- T1 Unreal editor Win64 Development clean target builds.
- T2 Full AIHeroes Automation report with zero failures and skipped.
- T3 Portable production C++17 source compiles at /W4 /WX and passes
  5,120 seeded variants plus negative tampering/fog tests.
- T4 Separate GUID-scoped v2 scratch SaveGame disk roundtrip;
  semantic field read-back; verified deletion of only own temp slot.
- T5 V1 valid fixture returns RequiresNewCampaign, invalid v1 returns
  InvalidLegacy; no old user save or GameMode is modified.
- T6 Prove a live ZoneExpedition state can be reloaded with its own
  A/B policy, independently of B.
- T7 Boundary: two classes with schema number 2 are never treated
  as equivalent; no silent converter or shared production prefix.
- T8 Manual Windows QA and Android build/test required before actual
  gameplay-live or cross-platform claims.

## Diff scope / rollback

Candidate B owns only:
- Source/AIHeroes/Public/AIHeroesProceduralCampaignCore.h
- Source/AIHeroes/Public/AIHeroesProceduralCampaignSave.h
- Source/AIHeroes/Private/AIHeroesProceduralCampaignSave.cpp
- Source/AIHeroes/Private/Tests/AIHeroesProceduralCampaignTests.cpp
- Tests/procedural_campaign_native.cpp
- Tools/verify_procedural_campaign_native.ps1
- Docs/PROCEDURAL_CAMPAIGN_V2.md

A is controlled by its own parallel implementation; do not modify,
reset or remove it as part of this proposed ADR. If a future accepted
ADR chooses A, B can be removed *after* read-back and a dependency
search, provided tests and independent validator coverage have been
migrated and the correct release receipts recorded.

**Builder/package mirror:** not needed (no Iskra canon change).
**Live verification:** pending; not claimed.
**Owner / approver:** AI HEROES project owner; proposal only.

## ΔDΩΛ

∆: proposed single canonical procedural-gameplay save contract.
D: compare actual ZoneExpedition and ProceduralCampaign sources,
   record incompatible v2 semantics, demand converter and QA gates.
Ω: confidence capped at 95% for observed source facts; proposed
   architecture is not live verification.
Λ: revisit after acceptance and documented field-level compatibility,
   complete SaveGame/route tests and integration receipts.
