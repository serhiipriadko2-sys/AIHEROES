# Adventure travel V2 - roads, advice and execution

Status: WINDOWS IMPLEMENTATION VERIFIED ON THE COMPILED SNAPSHOT.
Baseline HEAD: cc062ba2399adfb0de13c061e9d3912bea91b534.
Observed: 2026-10-28. The initial baseline passed all 98 automation groups.
The integrated snapshot passed 117 groups, including 8 TravelV2 and 8 AdventurePath.
Pre-existing rival, recruitment stock and zone-template changes were preserved;
this document does not claim authorship of those parallel features.

## Play and design
New campaigns use an explicit road network, not distance divided by 700.
All five existing sites remain known; this slice does not implement fog of war.
Undirected roads (movement points):

| Road | Cost |
|---|---:|
| HQ - FOREST | 2 |
| HQ - ORE | 2 |
| FOREST - OUTPOST | 1 |
| ORE - RUINS | 1 |
| OUTPOST - RUINS | 2 |

These costs are an original balance hypothesis, not copied HotA values.
A living hostile garrison is a permitted destination, never a transit node.
Only the destination is interacted with; crossing an empty site does not capture it.
The player must explicitly authorize every destination/battle.
Select a site: the HUD reports the path/cost or the reason it is blocked.
H asks about it. N compares recruitment. Z ranks routes at protocol tier 3.
Enter confirms the destination; failure spends nothing. Battle cost is applied
once, only when the verified result is committed, not when the arena opens.

## Compatibility and authority
SaveGame gains TravelRuleVersion, default 1 when the field is absent.
BeginPlay starts new campaigns at 2; loading restores 1 or 2 exactly.
Unknown versions fail closed; no automatic migration of a human save.
One pure travel service serves display, advice, recruiting advice, proposal
validation, two-stop plans and GameMode execution. Execution always re-reads
current state; a graph revision is a hint, not a permission or security token.
The two-stop planner clears only its first simulated captured site in a private
forecast branch. The real ownership and defenders never change during advice.
Battle commit binds the travel version in its frozen campaign fingerprint.
Only current map actors can be selected/executed; a foreign actor with the same
label is not a valid destination.

Road meshes use the authoritative link list, but have no collision/gameplay
power. Straight-line tactical-plan chords remain V1-only, avoiding a false
visual shortcut through a V2 guarding site. V2 displays road geometry and
planned stops; it does not animate strategic walking or claim path highlighting.

## Actual verification
Final build evidence: Saved/QA/TravelValidation_20261028T205737268Z/build.log.
Final full report: Saved/QA/TravelValidation_20261028T205737268Z/report/index.json.
All 117 tests succeeded, zero failed, skipped or test warnings.
The compiler warns that installed MSVC 14.51 is newer than preferred 14.50;
this is not represented as a warning-free compilation.

Eight new test groups cover topology/guards/budget; strategy/recruitment/gate
parity; hypothetical first capture; GameMode cost and atomic battle; version
drift abort; malformed map; versioned saves and A/B recovery; missing legacy
property. The eight earlier path-adapter groups also passed inside Unreal,
closing the previous native-only verification gap.

Real rendered scenario: blocked OUTPOST from HQ; FOREST battle costs 2;
exactly one commit, +2 DATA; OUTPOST then reachable from FOREST for 1 move.
The development-only -AIHeroesTravelSmoke harness drives real GameMode and
actor ticks. It issues no save calls and exits on completion. This is scripted
render QA, not proof of human keyboard/touch acceptance or Android performance.
Both resulting PNGs were visually inspected: roads, blocked target text, changed
ownership, remaining movement and new reachable price are visible.

Images: Saved/QA/AdventureTravelV2/ROAD_V2_BLOCKED.png and
Saved/QA/AdventureTravelV2/ROAD_V2_OPEN_AFTER_CAPTURE.png.
The four pre-existing save files were compared by name, byte count and SHA-256:
unchanged at the post-smoke read-back. Tests only write GUID-owned temporary saves.

## Reproduce
VS Code task: AIHEROES: Verify roads, advice and travel.
Command: powershell -NoProfile -File Tools/verify_adventure_travel.ps1 -Visual
This requires a free editor/automation slot and never terminates another session.
It compares source hashes before/after build/tests and the loaded module hash.
The first wrapper read the GUI log too early even though the actual scenario
later passed. The wrapper now explicitly waits on its exact child process and
checks that child's exit code plus the fresh PASS marker. A corrected-wrapper
rerun passed end-to-end: process exit 0, fresh smoke PASS, unchanged source and DLL.
Final evidence: Saved/QA/TravelValidation_20261028T205737268Z/receipt.json.
All 126 Source files remained unchanged across the final build/test/render cycle.

## Research used, not substituted for local tests
Primary sources checked 2026-10-28:
- https://h3hota.com/en/template-format - zones use declared connections;
  guards and road placement are distinct map design concerns.
- https://dev.epicgames.com/documentation/unreal-engine/run-automation-tests-in-unreal-engine?lang=en-US
  - command-line automation and JSON ReportExportPath.
- https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine?lang=en-US
  - explicit transfer between GameMode and custom SaveGame fields.
- https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.management/start-process?view=powershell-5.1
  - process handles, output redirection and waiting for spawned applications.

## Boundaries and rollback
No Android APK/device acceptance, real model inference, procedural map activation,
finished art, balance-playtest claim or cross-platform performance claim.
The current map is a small five-site prototype with instantaneous strategic moves.
Existing upper HUD labels can overlap the rival warning; UI polish is still pending.
No commit, push, deployment, model download or global configuration change performed.
Review scoped baseline copies under Saved/QA/AdventureTravelV2/baseline and current
diffs before rollback. Never reset unrelated concurrent work.

Delta: the path kernel now governs a playable campaign, not just isolated tests.
Trace: source read -> scoped patches -> Unreal build/report -> rendered smoke -> hash read-back.
Confidence: bounded to the tested Windows snapshot, never Android or release readiness.
Revisit: any topology, save-field, cost, combat or visibility-rule change requires parity tests.
