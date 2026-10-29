# ADR-20261029-04 — Tactical camera blur

Context: previous real-window Defeat/TwoGroups 2x frames show moving world
labels smeared by rendering. Stable group IDs and correct camera-facing do not
alone make moving text readable. HotA research reinforces separating readable
state from presentation effects; it is not evidence that our renderer works.

Decision: disable motion-blur amount only on the disposable tactical camera,
with an explicit local post-process override and active blend weight. This
camera is shared by live V2 and validated V2 replay. Keep the strategic camera,
global renderer config, rules V1/V2, event timing, outcome and save schema.

Alternatives: global renderer/CVar changes are broader and affect other views;
screen-space nameplate redesign is larger and remains a separate design.
Price: less cinematic motion smoothing in tactical presentation. Temporal-AA
ghosting/contrast/overlap can still remain; do not promise pixel-perfect labels.

QA: add a real disposable-stage camera test before the source fix; observe RED,
then GREEN. Assert override/zero amount/positive weight, paused orbit retention,
and unchanged independent camera defaults. Build against the actual current
source snapshot, run the full current AIHeroes group and native 2x fixture.
Preserve parallel ModelRequest work, but do not claim it is inference or our
implementation. Research documents are design input, not automatic mechanic
changes or permission to reuse protected game content.

Scope: one Stage camera hunk, one readability test, research/QA docs and receipt.
Rollback: only this test/hunk/new docs; no reset/clean, external edits, user-save
deletion, process closure, Git writes or private-canon changes.

Status: camera hunk compiled in
`Docs/BuildRuns/20261029T122621300Z/compiled.json` after the genuine RED report
`Docs/AutomationRuns/20261029T122444769Z/index.json` (override absent, amount 0.5).
Later existing full-suite artifacts were read back and compared:
`Docs/AutomationRuns/20261029T161717792Z/index.json` + `provenance.json`, matched
to `Docs/BuildRuns/20261029T161702109Z/compiled.json`: 245/245 Success, no warnings,
including TacticalCameraHasNoMotionBlur. This is verification of stored evidence,
not a new native run by the research pass. Source/DLL equality is snapshot-scoped.

Fresh native execution was not performed after the approval service returned
"workspace is out of credits". Independent-agent review also failed to start.
Concurrent art/source changes were then observed; the frozen 245-test result is
not promoted to current-worktree PASS. No approval bypass, process termination,
SDK install, Git write or new graphics claim is implied. Fresh same-build moving
frames remain pending; previous 2x images predate this override and are not proof
that temporal ghosting/overlap is fixed.

ΔDΩΛ: Δ tactical-view readability; D prior images + local UE camera API → RED/GREEN;
Ω 95% for observed source/build/report linkage, lower for visual effect;
Λ temporal artifacts, source drift or failed fresh QA.
