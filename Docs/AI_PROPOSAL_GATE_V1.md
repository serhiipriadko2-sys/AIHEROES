# AI HEROES — AI Proposal Gate v1

Date: 2026-10-28.
Status: implemented in the working tree; build/test evidence is independent.
Scope: deterministic read-only advice validation. No real LLM inference.

## Context and decision

The player manages strategy and authorizes attacks. In the future a local
or remote model may suggest game actions, but generated prose must never
be executable and no model can mutate the authoritative GameMode.

New interface: Source/AIHeroes/Public/AIHeroesAIProposalGate.h
Pure implementation: Source/AIHeroes/Private/AIHeroesAIProposalGate.cpp
Read-only integration: Private/AIHeroesAIProposalGameMode.cpp
Tests: Private/Tests/AIHeroesAIProposalTests.cpp

The proposal specifies kind (ADVICE / PROPOSE_ORDER / NO_ACTION), typed order
(NONE / MOVE / ATTACK), source (OFFLINE / LOCAL / REMOTE), observed day,
observed state revision, claimed model slot, target ID and bounded explanation.

The trusted snapshot contains the already filtered adviser state, current day,
opaque revision, model-slot tier, battle lock, model readiness and explicit
remote opt-in. No actors, file paths, secrets, network hooks, SaveGame object,
arbitrary commands or Iskra memory are passed to the model.

## Invariants

1. Reject malformed snapshots: invalid counters, oversized node/army arrays,
   duplicated visible node IDs, bad unit stats, unknown types, NaN or unsafe
   positions and resources/movement outside legal ranges.
2. Reject stale day or revision. The game derives revision from its current
   campaign fingerprint (CRC32). This is a freshness token, NOT a
   cryptographic proof or remote authorization.
3. Text is display-only: 1..512 UTF-16 units, no control characters,
   newline injection or empty/whitespace-only explanations.
4. Offline rules may not claim an actual loaded model.
5. Local model proposals require an independently verified loaded adapter,
   and the declared tier must match the actual slot.
6. Remote proposals require adapter readiness and explicit opt-in; gameplay
   slot 3B/8B alone cannot satisfy either condition.
7. ADVICE and NO_ACTION cannot carry any typed order or target.
8. PROPOSE_ORDER requires ANALYST protocol or above and no active battle.
9. Orders require a known, visible and correctly owned target, enough moves,
   a living army for attack, and an accurately distinguished MOVE vs ATTACK.
10. The canonical movement rules FAIHeroesTurnRules::TravelCost and CanMove
    are reused. The validator never performs the suggested action.

For every accepted proposed order, the result demands explicit PLAYER
CONFIRMATION. There is intentionally no Execute, StartBattle, MoveHero,
SpendResource or FileRead API in the gate.

## User-visible behavior

The offline proposer uses the already selected visible node; it never picks
or confirms strategic targets automatically. With SEED protocol only
ADVICE is available. After ANALYST, a node may generate a legal typed
MOVE/ATTACK proposal. The report shows PASS or BLOCKED with reasons.
All outcomes explicitly state that NO ACTION WAS EXECUTED.
A new paragraph is added only to the existing strategic adviser dossier
(not the recruitment or campaign objectives dossiers). H/I still work.

## Test matrix

Five Unreal tests under AIHeroes.Proposal:
- OfflinePreview: guarded/unguarded/owned, SEED, unknown selection
- AuthorityAndStaleness: day/revision, target, army, moves, battle lock,
  smuggled typed orders and unknown enum
- ModelAndConsentBoundaries: offline identity, model readiness, slot and
  remote opt-in
- InputAndSnapshotBounds: oversized text/snapshot, controls, duplicate IDs,
  invalid health, NaN and unreasonable positions
- DeterministicStaleFuzz: 10,000 fixed-seed invalid proposals rejected

Each test group is a deterministic software check. It does not prove
network inference, a genuine 1B/3B/8B checkpoint, device touch or Android.

## Safety and rollback

A valid proposal is still not an authorization. Integrate an actual inference
adapter only after versioned schema/structured parsing, explicit user
permission, resource budgets, origin attestation and separate QA.
CRC32 is an accidental-staleness guard, not an adversarial security token.

Rollback affects only the new proposal module, the private GameMode
declaration and one line in the strategy dossier; preserve all parallel
base, objectives, save, grid battle and replay work. Never rewrite a
canonical SoT30 file or player save to make the tests pass.

Status boundary: implemented != compiled != tests-pass != visible-in-game
!= model-loaded != mobile-ready.
