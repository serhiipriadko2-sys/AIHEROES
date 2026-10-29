# ADR-20261029-02 — Transaction and QA integrity

Date: 2026-10-29. Scope: active `AIHeroesGameMode`, PC development tooling.
Status: accepted and implemented. PC build, full automation and scripted
real-window smoke verified on 2026-10-29; manual input acceptance remains open.

## Context

Source inspection found two preflight gaps. Equal active/saved node counts
do not establish a one-to-one ID mapping: two active FOREST actors can replace
ORE and still pass the old lookup loop. A victory can also award resources
beyond the campaign save limit after movement, losses and journal changes
have already committed. Neither issue changes the deterministic battle rules.

An automation report proves the loaded binary's tests, not that the binary
matches the current source tree. Unreal's native exit code alone is insufficient.
Concurrent work makes a dated green report especially easy to misattribute.

## Decision

1. Validate a unique, exact active/saved node mapping and required references
   before applying a loaded snapshot. Keep schema and existing recovery slots.
2. Plan victory rewards with wide arithmetic and the existing resource bounds
   before committing any battle effects. On failure, abort without partial
   campaign changes; do not silently clamp or discard rewards.
3. Capture build-input and DLL hashes around a stable successful build. Allow
   strict automation to require that compiled snapshot and verify it both
   before and after tests. Retain explicitly binary-only legacy invocation.
   Coordinate participating automation runners with a mutex, while retaining
   the guard against an already running Editor-Cmd.

## Alternatives and price

- Count-only load validation is smaller but admits ambiguous actor mappings.
- Saturating rewards would hide overflow but change rewards/balance. Rejection
  preserves current rules at the cost of refusing a battle commit at the cap.
  The player must spend resources before retrying; completed battle persistence
  is not added here.
- Timestamp-only QA is cheaper but cannot establish content identity. Hashes
  add I/O and detect drift rather than preventing another tool from writing.
- A new procedural map/save schema would be broader and conflict with parallel
  work; it is explicitly excluded.

## Compatibility and balance

No save schema, old garrison conversion, battle rules version, reward amount,
model implementation, or game controls change. Valid existing campaigns and
normal rewards retain their behavior. Previously ambiguous maps are rejected.
QA provenance is local evidence, not a signed build attestation or deployment.

## Scope and ownership

- Load helper/tests and the load preflight in `AIHeroesGameMode.cpp`.
- Reward helper/tests and the preflight in `AIHeroesLiveBattleGameMode.cpp`.
- Build/test evidence tooling and the existing VS Code build/test tasks.
- This ADR, a progress log and a final machine-readable receipt.

Do not modify `.iskra/sot30`, unrelated game systems, user saves, Git history,
global settings, SDKs or model weights. No commits, pushes or deployment.

## Tests / QA

Require rejection of duplicate/missing/type-mismatched node mappings and
invalid restored references; accept reordered valid mappings. Test exact-cap
rewards, overflow, invalid resources, no-reward outcomes and atomic rollback
through a disposable live GameMode. Test evidence schema/hash/drift failures.
Build serially, then run the full current `AIHeroes` group and inspect fresh
JSON. A scripted real-window smoke is separate from manual input acceptance.
Record actual results in the final receipt; this ADR is not a PASS claim.

Verification read-back: `Docs/BuildRuns/20261029T112650766Z/compiled.json`,
`Docs/AutomationRuns/20261029T112705252Z/index.json` (173 Success, zero warnings),
and `Docs/VisualRuns/20261029T113106551Z/evidence.json` (six fresh frames,
native exit 0). Final source/DLL hashes match all 178 manifest inputs.
Installed compiler 14.51 is newer than UE's preferred 14.50; UBT emitted a
caution, not a build failure. These checks are not deployment or Android proof.

## Rollback

Revert only this cycle's named hunks/new files using their reviewed diff.
Restore prior task commands if needed. Do not reset the whole worktree, remove
user saves, or delete another writer's source. Keep evidence of any failed QA.

ΔDΩΛ: Δ transaction preflight and traceable QA; D source inspection to tests;
Ω pending execution; Λ failing regression, source drift or changed requirements.
