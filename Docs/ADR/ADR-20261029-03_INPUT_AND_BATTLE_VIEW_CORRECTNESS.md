# ADR-20261029-03 — Input and battle-view correctness

Date: 2026-10-29. Status: implemented; RED/GREEN and three scripted native
graphics scenarios verified. Full manual PC acceptance remains separate.

## Context

The active controller binds E to field-dwelling interaction while CameraPawn
polls E for rotation. At an owned visited mine, rotating the camera can claim
a dwelling or purchase units. This violates the player's separate camera/order
intent. The current battle view also leaves a dead group's label above its
freed cell, omits stable side/index identifiers, faces text only horizontally
despite an elevated camera, and appends an already included event reason twice.
These were observed in source and the previous real-window frames.

## Decision

- Keep Q/E camera controls. Move the explicit field action to unused key 3,
  beside recruitment keys 1/2; retain its existing touch button/action ID.
  Update live help strings and exercise the actual input bindings in tests.
- Hide dead group labels with their figures; display HERO[index]/NPC[index]
  using the same zero-based stable indices as events. Face labels and damage
  text fully toward the camera. Remove duplicate reason concatenation.
- Extend the explicit development-only smoke with Victory, Defeat and
  TwoGroups fixtures, using the same incremental live engine. The latter is
  deliberately synthetic, not a balance test or a default campaign change.
- Make the visual VS Code task reproducible: a wrapper requires the compiled
  snapshot, serializes native work, checks native exit plus the harness marker,
  saves fresh scenario-specific frames and verifies source/DLL/save hashes.

## Alternatives and price

- Moving E rotation would break the established camera convention. A modifier
  chord still risks camera rotation while interacting. Key 3 needs a help update
  but avoids overlap and retains the touch path.
- A new screen-space nameplate layout could improve contrast further, but is a
  larger UI design. This slice corrects visibility, identity and orientation;
  it does not claim final artwork or perfect readability on every background.
- Reusing real user save slots in smoke is rejected. These fixtures perform no
  save/load; a safe isolated save/apply seam remains a separate future change.

## Compatibility / scope

No battle rules version, outcome, economy price, army limit, save schema, model
inference, canonical protocol or normal campaign garrison changes. New smoke
fixtures require an explicit development launch flag and are not shipped game
progress. Keep AIH experimental classes and all existing parallel work.

Scope: PlayerController bindings, field-key help strings, LiveBattleStage,
focused tests, existing smoke harness, visual verifier/task, documentation.
No commits, pushes, SDK/model installation or changes to private `.iskra/sot30`.

## QA

Run new readability/binding regressions against the old behavior first, then
apply fixes and run the full current AIHeroes group with source/DLL provenance.
Require identity for duplicate groups, dead-label hiding on animated and fast
paths, camera-facing during paused orbit, one reason, and cleanup. Camera-only
keys must not dispatch mutating orders. Explicit field interaction must retain
normal guard behavior. Run separate real-window Victory/Defeat/TwoGroups smoke;
inspect fresh frames and retain any failed evidence. This is not manual input
acceptance or graphical save/load acceptance.

Evidence: RED `Docs/AutomationRuns/20261029T115747007Z/index.json` has exactly
seven intended failures; GREEN `20261029T120315037Z/index.json` has181 Success,
zero warnings/errors/skips. Final build snapshot is
`Docs/BuildRuns/20261029T120219685Z/compiled.json` (180 inputs).
Victory/Defeat/TwoGroups evidence under `Docs/VisualRuns/20261029T1204*` and
`20261029T120522060Z_TwoGroups` contains18 fresh1280x720 frames and native exit0.
All frames were inspected; each run compared four existing save hashes unchanged.

Known presentation price: contrast on pale tiles, possible overlap and moving
text motion-blur remain polish work. The successful checks do not claim perfect
nameplate legibility at every camera pose, manual input, or campaign balance.

## Rollback

Revert only this cycle's owned hunks/new files after checking the current diff.
Restore the previous task command if needed. No reset/clean, foreign process
termination, user-save deletion or rollback of other writers' changes.

ΔDΩΛ: Δ input intent and faithful battle view; D source/image findings to tests;
Ω pending QA; Λ a failed regression, input conflict or new visual evidence.
