# Independent read-only review — M2

Reviewer: gpt-6-astra / high, task `/root/m2_final_review`.
Scope: review_package.md + owned_changes.patch / owned_changes.json.
Reviewer verified all 14 pre-fix file hashes. No writes or execution by reviewer.

## Findings returned

- Critical: none.
- Important in own diff: none.
- Minor: AIHeroesBattleFallbackView.cpp counter `Раунд 2 / 7` ambiguously
  combines round and action count. Deferred to cohesion pass; add label test.

## Read-back findings

Valid UObject check excludes destroyed Stage. Normal HUD remains bound to
presented Stage data; fallback gets a copy of an already-produced event.
Unknown Reason remains explicit event data. No battle rule/save schema change.
No protected commit/abort or strategic-lock authority changes in reviewed diff.

## Limitations / root ruling

The reviewer noted pre-existing missing-Stage one-Step-per-frame pacing: the
2X image and generic smoke PASS wording do not measure speed effectiveness.
Root re-graded that existing issue as Important within M2's usability scope.
One fix pass followed; no second reviewer was requested. New timing test first
failed on all three exact 1x/2x/4x counters:
Docs/AutomationRuns/20261030T124904265Z/index.json.

Fix: transient presentation-only countdown, pause freezes it, speed scales its
duration, fast finish bypasses it. Battle engine and save schema remain unchanged.
The original reviewed patch/hashes are preserved; final_owned_changes artifacts
and the final suite describe the post-fix code. Do not claim that post-fix hunks
were independently re-reviewed.

Narrow layouts, human UI interaction, forced GC, package/Android and actual
model inference were not accepted by this review. Parallel FullCampaignJourney
test is not attributed to this task.
