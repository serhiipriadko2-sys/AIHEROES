# Live grid battle V2

PC development slice: the player authorizes an attack; the rule-based hero
chooses one action at a time while a real perspective 3D arena presents it.
This is not precomputed replay, neural inference or an Android release.

## Play

Use the VS Code tasks to build, test all gameplay automation, and launch the
playable Windows game. Select a guarded node, then Enter to authorize battle.
No campaign movement/HP/rewards change before the single final commit.

During battle:
- Q/E: orbit; R/F: camera distance, also available while paused.
- Space/Esc or PAUSE: pause/resume both decisions and presentation.
- T or SPEED: 1x / 2x / 4x presentation; rules and results stay the same.
- B or FAST FINISH: calculate the remaining current session, at most 16
  actions per frame. This is not retreat and never restarts the simulator.
- Enter / RETURN TO MAP: available after the final result only.
- Recruitment, upgrades, day, selection, route, save/load and slot changes
  are blocked at GameMode authority level while the arena is modal.
- After return: K saves; L loads; existing three-slot/A-B recovery is retained.

The preserved parallel adviser dossier is now minimally integrated:
I or DOSSIER opens the complete latest report; PgUp/PgDn browse pages,
Esc closes it. REFRESH re-queries the current snapshot without issuing an
order. A stale report is labelled, and the modal consumes background clicks
and blocks strategic orders until closed. Dossier is transient UI, not AI
memory or a saved campaign field.

## Frozen rules

8x6 square grid, four-neighbor BFS, no obstacles/LOS/retaliation/randomness.
Two stacks per side at (0,2)/(0,3) and (7,2)/(7,3). Stack identity is side
and index, not UnitId. Living stacks block cells; dead stacks free them.
Militia/Bandit/Raider: movement2/range1; Scout3/3; Wolf3/1; Guardian1/1.
Existing damage and initiative stats are unchanged. Stable initiative ties
prefer player then stack index. Each activation attacks if in range, or
moves then re-evaluates for an attack/wait. Thirty rounds maximum.
Protocol tiers retain their different target priorities. A new campaign's
OUTPOST has Raider6+4; this changes battle balance, not just presentation.
Loaded garrisons are never split or replaced.

## Interfaces and evidence

`FAIHeroesGridBattle::Begin` deploys a session with zero future actions.
`Step` produces at most one Move/Attack/Wait receipt. `RunToEnd` calls the
same Step for advisory forecasts and completed-record verification.
The stage accepts initial armies/cells and individual receipts only.
The GameMode waits for each presentation, validates the frozen campaign
and final record, then commits casualties/cost/capture/rewards once.

Saved battle RuleVersion defaults to 1 for old files. V1 keeps the legacy
resolver; V2 uses grid rules, two enemies and a 256-event record bound
(normal engine maximum 240). Unknown versions fail closed.

Post-slice verified V2 historical replay (2026-10-28): V opens the 3D viewer
for either valid V1 or valid V2 records. V1 retains its separate original
visual stage; V2 reconstructs every typed Move/Attack/Wait action from
verified grid rules before presenting anything in the 3D live arena.
The viewer never re-applies campaign consequences. In V2 replay,
Space pauses, T changes 1x/2x/4x playback, B/Esc/Enter skip, and
Q/E/R/F operate its independent camera. J keeps showing the text
transcript of either version. See Docs/GRID_V2_REPLAY_QA_2026-10-28.md
for the build, 77/77 tests, game-window smoke and SHA-256 evidence.

## Verification

Run the full `AIHeroes` automation group after a fresh build, with no other
Editor-Cmd sharing the shader cache. Tests cover grid legality, exact
Step/forecast parity, protocol divergence, terminal bounds, pause/speed,
visual loss, campaign locks/commit/conflict, repeated fights and save
version compatibility. Compilation alone is not gameplay QA.

The VS Code automation task uses `Tools/run_aiheroes_automation.ps1`.
It requires a fresh per-run JSON report with every test in Success, because
Unreal's process exit code alone can be zero even with failed assertions.
Split enemy groups with equal UnitIds are also tested against the parallel
recruitment adviser; friendly duplicate groups remain invalid.

`AIHEROES: Visual live grid V2 smoke` is an explicit non-shipping launch
harness. It drives the real GameMode/actor ticks, captures start/pause/2x/
result/returned-map/dossier PNGs under Docs, and exits. It performs NO save writes.
It is scripted rendering QA, not proof of manual mouse/touch acceptance.
Ordinary launches do not enable the harness.

See ADR_LIVE_GRID_BATTLE_V2.md and the fresh QA receipt for actual statuses.
