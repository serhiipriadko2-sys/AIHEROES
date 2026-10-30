# Execution ledger — plan: Docs/MAIN_CAMPAIGN_VERTICAL_SLICE_2026-10-29.md

## Contract / 2026-10-29

User requested to continue after the V2 plan. Execute inline in the existing
visible workspace; do not create branches/worktrees, commit, push or install.
First gate is M0, then a small M1 regression/fix, not all milestones at once.

Ruling: keep the current workspace and existing VS Code task scripts instead
of a Git worktree — the user explicitly requested this workspace and prohibited
Git mutations. Price: concurrent writers can invalidate source/binary parity;
record hashes and fail closed rather than resetting someone else's changes.

Pre-flight: M1 consumes canonical NextFocusId and existing TravelRules paths.
It produces read-only instructions/CTA evidence, not movement, changed costs,
battle rules, purchases or save fields. Preserve parallel BaseCopy/Aftermath/
ExplorerCopy/unity-helper repairs. Baselines are in
Docs/ExecutionRuns/MAIN_CAMPAIGN_M0_M1_2026-10-29/baseline.json.

## M0 / initial resource failure, then native baseline verified

- SoT30 integrity PASS, 30 knowledge / 32 hash entries. Canon unchanged.
- Computer UI bridge failed before app discovery: missing sandboxPolicy.
  No keyboard/mouse fallback or claim of an invoked VS Code UI task.
- Ran the exact task script Tools/build_aiheroes_editor.ps1 with normal UBT
  cache access. Fresh attempt: Docs/BuildRuns/20261029T184120592Z/build.log.
- Native build FAILED, exit6: C3859/C1076 and Windows1455, committed
  memory45.58/47.48GiB at UBT startup. No compiled receipt published.
- No Windows settings/pagefile changes; no foreign process closures.
- Requested user to free memory; user replied "освободил". Retried only after
  that material environmental change.

Ruling during resource failure: M0 was not complete. While waiting, run only low-memory diagnostics of
the actual portable C++ path/guide code. Do not call them an Unreal build,
gameplay acceptance or permission to skip M0. Keep production source unchanged
until the baseline can be checked; a diagnostic RED is useful progress, not DONE.

## M0 / refreshed baseline

- Build 20261029T185108175Z compiled successfully, but ExplorerHUD changed
  during that build. No compiled receipt was published. It is not frozen PASS.
- Stable recheck: Docs/BuildRuns/20261029T185342729Z/compiled.json, 252 inputs.
- Full native group: Docs/AutomationRuns/20261029T185354869Z, 274 Success,
  zero warnings/failures/notRun/inProcess, source + DLL checked before/after.
- Required new groups were actually discovered, including PlayableJourney,
  BasePanelCopy, BattleAftermath, LiveBattleFallback, VictoryOverlay,
  ReliefV5 and ArchitectureV6. Discovery does not prove optional assets imported.
- Existing map task: Saved/QA/MapReadability_20261029T185458870Z/receipt.json,
  stable source/DLL/saves; visible 1280/960/640 scripted captures. Inspected
  1280 ready and 640 over-budget images. Compact army summary is still absent.
- Existing battle task: Docs/VisualRuns/20261029T185713423Z_TwoGroups/evidence.json,
  six fresh screenshots, native exit0, user save hashes unchanged. Start frame
  inspected. Scripted fixture is not continuous campaign or manual-input QA.
- Content inventory captured separately: 53 files, content-manifest.json in
  the execution folder. Verify it again after the final visible scenarios.
- Static contract 20/20 and playable-loop static/build freshness PASS.

Ruling: the native baseline is usable for bounded M1 work. M0's complete visual
matrix remains partial: base screenshot and manual VS Code interaction are not
proved by the unavailable Computer UI bridge. Do not call it human acceptance.

## M1 / independently reproduced, then own bounded completion

At the initial read the guide mapped NeedsMoreMoves directly to End Day.
The real authored roads can require ORE-HQ-FOREST-OUTPOST cost5 at daily4.
A parallel writer has already added a route-only regression to TravelIntegration
tests; preserved it. An independent writer subsequently completed the over-day
guide branch. Preserved it instead of claiming authorship or implementing it twice.

Portable actual-C++ diagnostic (not Unreal ABI):
- archived baseline: Saved/QA/MainGuideDiagnostic_20261029T184956719828Z,
  EXPECTED_RED, over-day instruction fails, ordinary next-day case passes;
- adopted current body: Saved/QA/MainGuideDiagnostic_20261029T185023196806Z,
  PASS_PORTABLE_ONLY, both cases pass. Full native suite above also includes
  the new over-day regression.

Remaining source findings: Ready only checks the route, so an empty army still
gets an enabled guarded-attack CTA. Conversely Guide always asks for recruitment
even for a safe visit. Also the named intermediate owned stop is not independently
checked against today's remaining moves. These are display/query issues;
GameMode already blocks an empty-army battle and allows an unguarded visit.

Own production scope: only read-only guide/order eligibility and a non-selecting
TravelRules query for owned waypoints. No prices, movement, battle logic or save
schema changes. Five current files captured as m1_before_* before own edits.

Own RED preparation: introduced bTargetGuarded as context data (default true,
no behavior change yet) and AIHeroesCampaignGuidanceTests.cpp. Native regression
must fail on the legal unguarded empty-army visit before behavior is corrected.

## M1 / implementation and verification

- Native RED: Docs/AutomationRuns/20261029T190006409Z/index.json. Exactly one
  selected test failed on the two expected safe-visit assertions. UBT succeeded;
  Unreal itself returned 0, but the strict report wrapper correctly rejected it.
- Implemented shared CanOfferOrder(route, army, battle) display eligibility.
  The guarded empty-army action is disabled; the safe visit is not. Guide uses
  the known objective's actual guards rather than making recruitment universal.
- Added GetTravelPathTo as a const, non-selecting query. Existing selected-path
  query delegates to it. Hidden/unknown sites are rejected before path search.
  A named owned waypoint now also has to be reachable today. If only a later
  full day's leg is legal, the guide distinguishes resting for that leg from
  magically traversing the complete five-move route.
- Added four native regression groups: empty army / guards, eligibility truth
  table, rest-for-staging only, real GameMode query immutability and fog boundary.
- A first GREEN build hit our test-fixture C2248 private-field access, not an
  environmental failure. Fixed the test through DispatchBeginPlay and public
  scouting queries; did not expose private fields or add a test-only fog setter.
- GREEN compiled snapshot: Docs/BuildRuns/20261029T190649733Z/compiled.json.
  Full strict suite: Docs/AutomationRuns/20261029T190945101Z, 278 Success,
  zero warnings/failures/notRun/inProcess, fixed 253-input snapshot and DLL.
- A parallel coordinated QA held the mutex once. Our run refused to start;
  no second Editor-Cmd or forced termination. Retried after that work exited.
- Extended the existing TravelView smoke (not another gameplay mode): real
  forest battle and return, read-only base, explicit empty-army guarded/safe
  fixtures, safe confirmation, EndTurn and a five-move route with a real legal
  HQ staging query. Fixture edits are deliberately not campaign balance proof.
- Saved/QA/MapReadability_20261029T191125370Z: 278 tests again, 21 fresh PNGs
  across 1280/960/640, zero source/save drift and the same DLL. Base images at
  all three widths, 1280/640 staged-route and 640 guarded/safe cards inspected.
- Final DLL scripted battle smoke: Victory 20261029T191436957Z, Defeat
  20261029T191504667Z, TwoGroups 20261029T191532284Z in Docs/VisualRuns.
  All three passed; 18 additional fresh captures, pause/speed/fast finish,
  one commit and exact camera return, no save writes. Inspected TwoGroups 2X
  and Defeat result. Separate disposable launches are NOT consecutive campaign
  battles or human-input acceptance.
- Static loop contract now covers the shared CTA and independently reachable
  waypoint; 16 checks PASS with current source/binary match. PowerShell parser
  PASS. All changes since the baseline in Source/Config/project are exactly
  the seven own source/QA files; adopted parallel code was preserved.

## Decisions / price / rollback

Use one small display predicate and the authoritative existing path finder,
not a second movement system, AI permission or a cached execution route. Price:
several small read-only path queries per frame on this five-site campaign.
No balance, rule-version, save schema, agent instructions or canon mutation.
Own change artifact: m1_owned_changes.patch + m1_owned_diff_manifest.json in
the execution folder. Rollback only these hunks after checking current hashes;
never restore an entire shared file over later changes. No rollback performed.

## Remaining acceptance / next

M0 native + scripted graphical baseline verified. M1 implemented and checked
automatically; full human acceptance remains PARTIAL. The Computer UI bridge
was unavailable, so exact task scripts were run from the workspace terminal,
not claimed as clicked VS Code tasks. No manual keyboard/mouse pass, continuous
campaign, save/load UI acceptance, package, Android or real LLM inference.

Known remainder: compact map lacks army summary; base still uses a different
palette; some event toasts remain English. In the synthetic safe-visit frame
the earlier failed-attack toast is still visible because guards were reset
directly; the enabled safe CTA and subsequent actual confirmation both pass.
Do not read that stale fixture toast as an authoritative safe-visit rejection.

Next bounded milestones: validate existing M2 presentation-loss fallback in a
live disposable battle, then finish actual M3 reward/aftermath evidence before
M4 cohesion/compact-army polish and M5 continuous campaign acceptance.

ΔDΩΛ: Δ M1 display/query gaps closed without changing rules; D native RED →
GREEN 278/278 → same-DLL scripted frames and battles; Ω95% for automated
observations, not a human playability rating; Λ source drift or human acceptance.

## M2 / resumed 2026-10-30, test-first

Context refreshed from clean HEAD 6bfe493, active main GameMode, pinned SoT30
PASS and matching M1 source/DLL. No assumption of a human playthrough.
Current context: Docs/PROJECT_CONTEXT_2026-10-30.md.

Ruling: continue in the user-requested shared workspace, without worktrees,
commits or deletion of execution evidence. Keep the existing ledger instead
of commit-oriented SDD helper state. Price: final source/DLL drift gates remain
mandatory. A fresh read-only final reviewer is required by the execution skill.

Pre-flight M1→M2: M1 produces legal strategic CTA; M2 must preserve GameMode
locks while presentation fails. M2→M3: no speculative rewards or new save fields;
result stays tied to the existing protected commit. Normal Stage reads its own
presented action; only missing Stage may read the latest completed logical Step.

RED inputs: destroyed Stage before GC; real Attack event with side+index,
damage7 and Reason=highest threat. Added optional event input / reason output
as data-only API surface; existing fallback behavior is still unchanged pending
observed failure. Baselines for 14 files, 53 Content and 4 save files are in
Docs/ExecutionRuns/MAIN_CAMPAIGN_M2_2026-10-30/baseline.json.

No M2 completion claim yet. Do not count a compilation error as regression RED.

M2 first GREEN: 283/283, no test warnings; immutable snapshot
Docs/BuildRuns/20261030T123902574Z/compiled.json. It includes the parallel new
AIHeroesFullCampaignJourneyTests.cpp (not ours), which uses GUID-owned QA slots.
Initial MissingStage + Victory/Defeat/TwoGroups scripted graphics passed.

Fresh read-only review (gpt-6-astra) found no Critical/Important own-diff defect.
Minor deferred: fallback label "Раунд n / actions" is ambiguous; make labels
explicit in the planned cohesion pass, with a label regression.

Final: Ruling: reviewer left the pre-existing no-Stage frame-rate pacing outside
the patch. Re-grade as Important for M2 usability: 2x/4x buttons should change
text display duration rather than silently do nothing. Cost of leaving it is
an unreadable fallback that can finish before the player can react. One fix pass:
native RED for text timing, then a transient display timer only; never change
GridRules, outcome, saved state or authoritative per-action content. No re-review;
verify that fix with RED→GREEN and a fresh full suite as the skill requires.

Final: fixed text pacing — SpeedIsPresentationOnlyWithoutArena failed on the
three expected 1x/2x/4x counters, then passed in the full suite 284/284,
warnings=0, report Docs/AutomationRuns/20261030T125527178Z/index.json.
Final compiled snapshot: Docs/BuildRuns/20261030T125053914Z/compiled.json.
Transient 0.6 base presentation seconds, speed-scaled decrement, paused freeze,
fast-finish bypass and per-session resets; no timer in GridRules or saved state.
The native test also verifies the resulting V2 record and exactly one commit.

Ruling: reuse BattleArt's presentation-only formatter for normal presented
actions and missing-stage completed-event copies, rather than a second rule or
caption subsystem. Price: a little extra presentation-code coupling, but one
translation vocabulary. Unknown reasons remain bounded quoted event data.

Fresh review summary and its exact scope are preserved in
Docs/ExecutionRuns/MAIN_CAMPAIGN_M2_2026-10-30/independent_review.md.
No second review of the post-review timing fix was claimed. Final visual runs
are being repeated on the post-fix DLL; earlier 283-test frames remain history.

## M2 / verified automated checkpoint, 2026-10-30

Post-fix visible scenarios all PASS on the same DLL/source snapshot:
Docs/VisualRuns/20261030T125755552Z_MissingStage,
20261030T125823282Z_Victory, 20261030T125850919Z_Defeat,
20261030T125918547Z_TwoGroups. Six fresh PNGs each, native exits0,
source/DLL/manifest stable, four existing user saves unchanged.
Inspected final MissingStage PAUSED/RESULT, TwoGroups 2X and Defeat RETURN.
Speed effectiveness is grounded in the new native timing regression, not in
the filename 2X or the generic harness PASS wording alone.

Task M2: implemented + native/scripted verified. Human UI / forced GC / narrow
viewport text acceptance remain unverified. VS Code task added and parsed;
Windows UI connection failed with missing sandboxPolicy, so task scripts ran
through the workspace terminal, not falsely claimed UI task clicks. No process
was forcibly closed, no installs/commits/pushes/OS settings or save schema changes.

Final: minor (deferred): distinguish round and action count explicitly in the
fallback header; change belongs to the cohesion pass and needs a label test.
The reviewed patch is retained separately from final post-fix hunks.

Next: M3 actual committed reward/aftermath receipt, preserving the existing
BattleAftermath and save compatibility. Parallel native end-to-end campaign
test remains external authorship; its successful result is included in 284,
not relabelled as human campaign acceptance or this task's implementation.

ΔDΩΛ: Δ robust, reason-grounded, speed-controlled text combat; D two initial
REDs + timing RED → 284/284 GREEN → four final visible scenarios; Ω95% for
automated observations; Λ source drift, narrow-layout or human feedback.

## M3 / 2026-10-30, parallel ownership discovered

Initial SoT integrity PASS, clean HEAD 1ff88bb and no native process. Captured
nine baseline files, 53 Content and four saves. Before our implementation,
another writer changed the same M3 production files. Our first-file context
check failed atomically; no production hunks were applied. Preserve external
Aftermath/GameMode/HUD and their test work rather than implementing M3 twice.

Ruling: switch to independently owned regression coverage until ownership is
coordinated. Price: M3 implementation acceptance is pending a stable integration
window. Asked user to pause parallel M3 writing; no editor/process force-close.

Own addition: AIHeroesAftermathIntegrityTests.cpp, two positive/negative-control
cases for invalid replay evidence and same-metadata receipt confusion. Details
and candidate fixes are in MAIN_CAMPAIGN_M3_2026-10-30/PARALLEL_HANDOFF.md.
No runtime failure or M3 completion claim until the native report is read.
