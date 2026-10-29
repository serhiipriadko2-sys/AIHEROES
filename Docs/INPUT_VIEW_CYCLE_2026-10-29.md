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

Readability tests prepared against unmodified production Stage; RED run pending.
No agent is allowed to run a parallel native Editor/test/build or write Git.

## E Verify / F Close

Pending. New fixtures will distinguish automated native-window checks from
manual keyboard/mouse/save-load acceptance. Preserve failures, then verify
fresh source/DLL identity and the full current test group.
