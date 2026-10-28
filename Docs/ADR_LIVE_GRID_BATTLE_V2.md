# ADR — live grid battle V2

Date: 2026-10-28. Scope: AIHEROES gameplay only.

Decision: accepted for the selected first slice. Implementation: candidate;
integrated compile, automation and visual QA evidence are **pending**.
This ADR is not a claim of deployed gameplay, Android readiness, real LLM
inference, or execution of the full Iskra protocol inside the game.

## Context

The player selected a live, AI-controlled 3D battle on a grid, a readable
basic-geometry prototype, and campaign saves only outside active battles.
The existing deterministic resolver completes a battle synchronously. The
parallel replay/save work can present that completed result, but a replay
does not meet the selected requirement that the next action is chosen while
the battle is running. Existing source and player saves must be preserved.

## Decision

- Introduce a deterministic V2 combat session with `Begin`, a bounded `Step`
  producing at most one validated move/attack/wait event, and a terminal
  outcome. `Begin` must not calculate future actions or the winner.
- Use an open 8 x 6 square board, four-neighbour movement, Manhattan attack
  ranges and one figure per army stack. Living stacks block their cells.
  Existing HP, initiative and damage arithmetic remain the combat basis;
  movement and range intentionally change balance relative to legacy V1.
- Keep player authority over strategic attacks. During combat the hero AI
  chooses legal moves and targets; the player controls only the camera,
  pause, presentation speed and fast completion of the remaining session.
- Keep the 3D stage a passive view of validated events. Compute the next
  action only after presentation of the previous action completes. A
  headless V2 runner advances the same session for advice and verification.
- Freeze the campaign's pre-battle context and lock strategic mutation
  entry points while running or paused. Commit movement cost, casualties,
  capture, rewards and journal exactly once at the terminal result. Do not
  serialize an unfinished battle; save/load and slot changes are blocked
  until the final commit. A missing visual stage cannot grant rewards.
- New campaigns split the OUTPOST's ten Raiders into formations of six and
  four so target selection is visible. Existing saved garrisons stay as
  saved; validation accepts up to two otherwise valid guards of that node's
  allowed type. Friendly duplicate stack rules remain unchanged.

## Compatibility and interfaces

`FAIHeroesBattleRecord::RuleVersion` is a SaveGame integer defaulting to 1.
Existing records without this field retain legacy V1 meaning. `Capture`
accepts a final optional version argument; live-grid callers pass 2.
Unknown versions are rejected, including anonymous/empty records. Only an
otherwise empty V1 record represents an older campaign with no battle.

V1 verification still runs the original resolver, with one neutral stack
and at most 128 trace events. V2 verification runs the grid session to
completion, with at most two neutral stacks and 256 trace events. Both
compare the regenerated canonical trace, terminal flags, summary, rounds
and complete resulting armies. Semantic save read-back includes the rule
version. Trace text is evidence, never parsed into commands or trusted as
authority. Typed live events are transient; no mid-battle schema is added.

The campaign save schema remains compatible with existing optional fields;
there is no automatic rewriting or splitting of loaded garrisons. Version
metadata does not authorize replaying rewards into the campaign.

## Alternatives

1. **Precomputed replay:** reuses the current resolver with less runtime
   state, but was rejected because the user chose live action selection.
2. **Free-space tactical movement:** supports continuous navigation, but
   the user chose a grid; navigation and physics are not part of this slice.
3. **Mid-battle save/restore:** requires serializing initiative, cells and
   transaction state; deferred by the user's outside-battle-save choice.
4. **Change the legacy resolver in place:** smaller API surface but would
   invalidate old recorded battles and silently reinterpret save evidence.

## Consequences and price

- V2 needs explicit session, board, presentation and campaign-commit tests.
  A green compile is not proof of correct visible gameplay.
- The richer rules intentionally alter forecasts and battle outcomes. The
  active adviser must use V2 rather than quote a legacy V1 prediction.
- Legacy verification stays available alongside V2; this is deliberate
  compatibility code, not a second active campaign combat authority.
- Closing the process during an unsaved battle loses that session; loading
  resumes from the previous valid campaign save. This is not crash-resume.
- Basic geometry is prototype art, not authored character animation. Real
  model checkpoints, Android packages, and a broader art pipeline have
  separate acceptance gates and are not implemented by this decision.

## Diff scope and boundaries

Gameplay session/rules, passive battle stage, active GameMode/adviser,
HUD/controller/camera routing, versioned battle records, campaign guard
validation and focused tests are in scope. Existing parallel save/replay
enhancements must be re-read and retained rather than overwritten.

No changes to pinned `.iskra/sot30`, agent instructions, canonical memory,
connectors, global configuration, Git history or external services are
authorized. Campaign SaveGame is game persistence, not Iskra Archive or
training memory. No commit, push, deployment or Android installation is
part of this ADR.

## Tests / QA

Required before integrated acceptance:

- Begin has no future transcript; each Step performs at most one legal
  action. Test movement, range, occupancy, initiative ties, dead stacks,
  empty armies and the 30-round stalemate boundary.
- Incremental V2 and headless V2 produce identical complete results.
  Forecasts match live rules. Pause, frame rate, speed and fast completion
  cannot alter transcript, damage or rewards.
- Player input cannot bypass battle locks. Duplicate completion callbacks
  cannot apply a second cost or reward; invalid commit context cannot
  partially mutate the campaign.
- V1 defaults and V2 records verify and serialize; unknown versions and
  tampered traces, flags, summary or survivors fail. Two allowed neutral
  formations validate, while wrong types, a third formation and duplicate
  friendly formations fail. A loaded unsplit garrison is not changed.
- Repeated visible PC battles show actual grid movement and autonomous
  attacks, HP/count and protocol-dependent choices, pause/speed/finish and
  exact camera return without leaked stage actors.
- Run the full current `AIHeroes` automation group with no competing
  Editor-Cmd shader-cache user, plus the repository Unreal build and pinned
  SoT integrity check. Record fresh commands, results and screenshots.

Fresh integration evidence: UBT build succeeded; all 58 current AIHeroes
automation tests succeeded with zero warnings. An opt-in real-window smoke
captured start/pause/normal2x/result/return/dossier and verified an exact camera
return, one campaign commit and fresh screenshot writes. It performs no
save writes. See LIVE_GRID_V2_RECEIPT_2026-10-28.json for hashes and logs.
Manual mouse/keyboard save/load acceptance remains unverified because the
Windows UI bridge failed its sandbox metadata handshake. Scripted rendering
QA is not represented as manual graphics/input QA.

Concurrency boundary resolved: the user paused other writers and authorized
minimal completion of their unfinished dossier. Missing GameMode/HUD
declarations, full-report query hooks, modal hit routing and I/PgUp/PgDn/Esc
controls were connected without changing the dossier's read-only intent.
A unity-build helper-name collision was fixed without changing V2 rules.
All 75 source hashes and the DLL hash match the compiled snapshot after
the final 58-test run and real-window smoke. No post-QA source drift remains.

Integration ruling: the parallel recruitment adviser must allow equal
enemy UnitIds in distinct formations (Raider6+4), while continuing to reject
duplicate friendly recruitment stacks. A new regression failed before the
one-condition compatibility fix and passed in the full post-fix suite.
The VS Code automation task now validates fresh JSON: Unreal returned exit0
for that failing regression, demonstrating why exit status alone is unsafe.

## Rollback

Before any V2 save is written, revert only this feature's scoped changes,
preserving all pre-existing/concurrent replay and save work; do not reset
the worktree or delete player saves.

After V2 saves exist, a UI/runtime rollback must keep the V2 headless
verifier, version metadata and two-guard compatibility reader so those
saves remain readable. A complete downgrade to a V1-only binary does not
understand V2 evidence or two-formation snapshots: retain the original
save files and use a separately preserved pre-V2 save rather than silently
converting or overwriting them. No automated downgrade is authorized.

## ΔDΩΛ

- Δ: separate live grid combat from legacy replay and version its evidence.
- D: user choices, inspected active gameplay and bounded compatibility diff.
- Ω: 95% for the automated/scripted slice; manual input QA is still unverified.
- Λ: revise after fresh integration QA, a rules change, or a persistence
  compatibility failure; never rewrite the pinned canon to make it pass.
