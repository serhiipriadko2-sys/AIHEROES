# Input / battle view cycle — 2026-10-29

## A Intake / B SIFT

User asked to continue comprehensive development. Fresh SoT30 integrity PASS.
Active mode remains AIHeroesGameMode; HEAD87d9012. Existing dirty work contains
previous docs/QA outputs only and is preserved. Three read-only audits found
the E camera/order collision and four specific battle-presentation defects.
The earlier 173-test result is a baseline snapshot, not proof for this cycle.

## C Frame

See ADR-20261029-03. This cycle corrects existing input/presentation behavior,
not a new battle mechanic, save format, model or map schema. Key3 is a local
implementation choice, not a newly claimed user design agreement.

## D Step

- Input implementer owns controller, field-key help strings and binding tests.
- View implementer owns Stage and new actor-based readability tests.
- Smoke implementer owns only the explicit development smoke harness.
- Root owns native visual verifier, VS Code tasks, common docs and serial QA.

Readability and input tests prepared against unmodified production Stage/input.
No agent is allowed to run a parallel native Editor/test/build or write Git.

## E Verify / F Close

RED evidence: `AutomationRuns/20261029T115747007Z/index.json`: **181 tests,
174 Success, exactly seven intended failures** (three actual-binding tests and
four actor-based presentation tests). E changed Forest.bClaimed from0 to1;
other assertions identified dead labels, yaw-only facing, missing stable IDs,
and duplicated reason. Independent read-only review confirmed the same result.
Production fixes were authorized only after this behavioral RED.

Earlier evidence is preserved rather than smoothed away:
- Build `BuildRuns/20261029T115245223Z` failed on an incorrect test-only include
  `Engine/URL.h`; changed to the locally verified `Engine/EngineBaseTypes.h`.
  This compile failure is not treated as behavioral RED.
- First successful test build was `BuildRuns/20261029T115430311Z`. Before tests,
  an external Git merge advanced HEAD to8a12d0e and changed controller Esc to
  cancel a pending supply quote. Strict provenance rejected that stale snapshot
  before launching Editor-Cmd. No merge/Git/index operation was performed here.
- That external Esc fix was preserved. `BuildRuns/20261029T115654779Z` captured
  the new stable source used by the real RED run (180 inputs).

## GREEN and native graphics read-back

- Final build: `BuildRuns/20261029T120219685Z/compiled.json`, 180 stable inputs
  plus fixed DLL. The previous GREEN build120113951Z also succeeded; one remaining
  convoy help string and the legacy field QA key script were then synchronized
  to3 before the final build. Only test-description wording changed in the
  existing field hitbox test; no conditions were weakened.
- Full GREEN: `AutomationRuns/20261029T120315037Z/index.json`, **181 Success,
  zero errors/warnings/skips**. The seven new regressions now pass.
- Independent reviewer confirmed all180 actual input hashes, DLL, report and
  provenance, plus RED and GREEN counters. External Esc supply-quote cancellation
  is retained. Other external QA-tool changes are preserved, not attributed here.
- Native runs, all exit0/scenario-specific PASS and source/DLL before-after match:
  - `VisualRuns/20261029T120427209Z_Victory/evidence.json`
  - `VisualRuns/20261029T120454606Z_Defeat/evidence.json`
  - `VisualRuns/20261029T120522060Z_TwoGroups/evidence.json`
- All18 fresh1280x720 images were viewed. Stable duplicated enemy group IDs,
  dead-label disappearance on both sides, updated FIELD3 HUD, pause/path/attack,
  results, camera return and dossiers were observed. Four save files were
  unchanged by count/hash in each run; no save/load command was issued.
- The Defeat invocation used lowercase `defeat`; wrapper normalization produced
  the canonical Defeat selector and passed. Invalid wrapper Scenario was rejected
  before native launch. Parser6 scripts, tooling23 fixtures, Codex static20 checks,
  final source/DLL read-back and diff whitespace check all PASS.

## F Close / limits

Scoped implementation/technical QA: **PASS**. Full human PC acceptance: **PARTIAL**.
Tests dispatch the real controller's bound delegates in a disposable world, not
OS keyboard input. Legacy targeted field QA script was synchronized, not executed.
VS Code tasks are available, but no claim that the Windows task UI was controlled.
Manual mouse/key input and visible save/load remain unverified; Android and actual
model weights are outside this slice.

The images still show pale-background contrast limits and motion blur on moving
world labels (notably Defeat/TwoGroups2X); the HUD is a readable fallback. No
backplate/layout/artwork redesign was silently added. These are explicit polish
items, not a claim of final graphics quality. Synthetic smoke armies do not
establish balance or change ordinary campaign guards.

Receipt: `INPUT_VIEW_RECEIPT_2026-10-29.json`. No Git/index writes, process
termination, SDK installation, private-canon changes or manual user-save edits
by this agent. Revert only named owned hunks/new files; keep external merges,
user saves and earlier evidence intact.
