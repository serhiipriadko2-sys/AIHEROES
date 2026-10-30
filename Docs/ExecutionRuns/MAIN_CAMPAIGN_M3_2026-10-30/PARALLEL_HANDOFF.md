# M3 — independent regression handoff, 2026-10-30

Started from clean HEAD 1ff88bb. While preparing M3, another process changed
Aftermath, GameMode and HUD. Our attempted production patch failed its FIRST
file context check; it applied no hunks. All current production M3 changes and
the existing Aftermath/LiveBattle test edits are external authorship.

Preserve those files. Our own source addition is only
`Source/AIHeroes/Private/Tests/AIHeroesAftermathIntegrityTests.cpp`.
Before any implementation, coordinate ownership / a stable writing window.
Asked the user to pause the parallel M3 writer; no force-stop or reset performed.

## Independent checks / expected failures, not yet claimed runtime proof

1. `CorruptTraceHasNoCasualtyFacts`: construct a genuine V2 record, verify it,
   corrupt an event, prove the canonical verifier rejects it, then require the
   aftermath not to claim casualty facts. Current builder only inspects health
   shape, so source suggests it still accepts this bad history.
2. `SameMetadataCannotBorrowReward`: two genuine records with the same
   node/day/rule version but different protocol/trace. A receipt for one must
   not attach to the other merely because those three metadata fields match.
   Positive control must accept its own receipt; no unbounded fake input.

Native results must be read before either is called a verified defect.
Do not count compiler errors as RED. Only build when the existing native work
has ended, through the shared mutex and source/DLL hash checks.

First native run 20261030T131839045Z failed its positive controls: our fixture
used modified unit profiles, which Grid V2 correctly rejects. NOT regression
RED. Corrected fixture to canonical Militia/Scout/Wolf profiles; rerun required.

## Verified transition / user-paused handoff

Corrected run Docs/AutomationRuns/20261030T132137682Z/index.json: two genuine
negative controls failed, while genuine-record and own-receipt positive controls
passed. This is the actual regression RED: corrupt trace still received counts,
and another full record borrowed same-metadata reward evidence.

Before the user's pause arrived, the parallel writer added full BoundRecord
identity, canonical verification and a cache API. These are adopted external
changes, not ours. User then explicitly paused parallel M3 writes. Current files
were read back and copied into handoff_snapshot.json / handoff_* for attribution.

Own minimum integration after that pause: HUD uses the prepared cache rather
than verifying each paint (separate source-contract RED→GREEN); load clears the
cache too; view preparation is preflighted before the irreversible transaction
and only published after actual writes. Existing fabricated record tests were
converted to real canonical Grid V2 outcomes, keeping invalid-input controls.
Added actual ruins gold/data/cache lifecycle checks and a genuine V1 draw case.

Original reviewed/parallel code remains in the handoff copies. Do not attribute
the whole final Git diff to this executor. Full acceptance needs the final
native report, live result frames and immutable source/DLL proof.

## Candidate follow-up (not applied)

Use full `BattleRecordRules::Verify` at preparation/opening boundaries and
cache the resulting view; do not replay the battle each HUD frame. Bind the
transient commit evidence to the full record (or a canonical fingerprint),
not just node/day/version. Continue using the actual already-computed reward
plan, never current node reward rules to infer historical awards. Save schema,
GridRules, balance and private SoT30 remain unchanged.
