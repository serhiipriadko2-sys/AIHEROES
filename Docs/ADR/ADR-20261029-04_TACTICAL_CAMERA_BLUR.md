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

Status: tests prepared; runtime verification pending.

ΔDΩΛ: Δ tactical-view readability; D prior images + local UE camera API → RED/GREEN;
Ω pending runtime; Λ temporal artifacts, source drift or failed QA.
