# M3 independent integrated-code review

Fresh read-only reviewer: gpt-6-astra / high, `/root/m3_final_review`.
Whole pre-fix integration reviewed against baseline 1ff88bb, not merely the
remaining working diff after external commit 1b976b7. All 9 reviewed hashes
matched integration_scope.json. Reviewer did not write files or execute QA.

## Findings

- Critical: none.
- Important: J/V historical user surfaces did not show absent recorded reward
  evidence. Build(record)'s fallback existed only as an API contract; live commit
  was the only production builder caller. This was an acceptance gap, not a
  resource-grant bug. Accepted for the one post-review fix pass.
- Minor deferred: LiveBattleTests holds a receipt reference and compares the
  member to the same live reference after callback. That one assertion is
  tautological. Independent real gold/data/movement checks still cover resource
  repeat safety; evidence-immutability assertion should later use a copied value.

## One fix pass / actual RED

HistorySurfaceDeclaresMissingAward failed both J and V message assertions in
Docs/AutomationRuns/20261030T134650396Z/index.json; replay opening and existing
corrupt-history gate worked. Then existing historical status surfaces received
an explicit absence note. No amounts were inferred, no new saved reward fields.
Existing canonical verification stays at J/V preparation boundaries, not frames.
Victory smoke now also captures HISTORY and checks campaign fingerprint after
replay. Other three smoke scenarios retain their existing six captures.

Final source artifacts are separate final_* snapshots. No second independent
review of post-review hunks is claimed; fresh full tests and visuals are the
fix verification, as required by the execution skill.

Reviewer otherwise traced full Equivalent binding, Verify, indexed groups and
ceil HP counts, actual reward-plan values, unpublished local preflight candidate,
post-write publication, cached HUD and start/abort/return/load resets. Save schema,
rules and balance unchanged. Actual load lifecycle, human/narrow UI, packaging,
Android, performance and full external Iskra runtime were not accepted here.
