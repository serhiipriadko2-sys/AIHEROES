# Engine-owned request export for the local model bridge

Status: IMPLEMENTED; build, regression and live-scenario evidence pending.
Scope: the existing five-site AIHeroesGameMode, not the procedural expedition.
This module neither invokes a model nor changes the offline advisor.

## Data path

AAIHeroesGameMode::BuildLocalModelRequest (game thread)
-> internal advisor snapshot + current proposal revision
-> FAIHeroesModelRequest::Build
-> bounded aiheroes.hero-request.v1 JSON
-> optional future transport, not connected by this change.

The internal travel snapshot has all five topology nodes. Serialization is a
separate allowlist: only discovered sites and their disclosed guards leave it.
World coordinates, battle-history text, actor pointers, save paths, readiness
flags and other internal fields are not serialized. ASCII stable game IDs are
not localized display names. No arbitrary prompt text or player files enter.

Formal orders are recomputed with the existing proposal gate and travel solver.
SEED and live battle/replay states have no strategic order menu. Blocked roads,
unaffordable targets, attacks without an army and no-op travel to the current
site are excluded. Legal does not imply a winning or strategically good move.
Visible nodes are ordered by stable ID, independent of actor-array ordering.

Invalid tokens, state, ownership, topology or identifiers clear the output.
The serializer is read-only and builds into a temporary string before success.
Every emitted string is fixed text or validated restricted ASCII. Numbers use
bounded integer types. The complete UTF-8 request is capped at 65,536 bytes.
No additional JSON/network/plugin dependency is added to the Unreal module.

## Host obligations still outstanding

Request/session IDs are supplied by the trusted future caller; they do not
authenticate it. The smoke uses fresh GUIDs. A production asynchronous adapter
must own and invalidate sessions on load/new campaign, match responses to the
current request, recheck the complete current state and call the final game
gate on the game thread. CRC32 is an opaque revision, NOT a security signature.
This exporter does not implement the asynchronous caller or response consumer.
Model readiness remains false. No weights are installed or attested. Player
confirmation remains mandatory for strategic actions; battles remain rule AI.

## Verification

Tools/verify_model_request.ps1 -Visual composes the existing map verification
with six new AIHeroes.ModelRequest test groups and a separate opt-in scenario.
It never closes an existing game/compiler; finish other checks before running.
The scenario exports four real game states to a unique Saved/QA directory:
SEED, legitimately purchased analysis protocol, active battle, forest victory.
It uses normal game commands, never writes a campaign save, and compares state
before/after each export. Test scenario purchases/fights are not exporter writes.
The ordinary game does not run this scenario or write request files.

Tools/model_request_contract_v1.py is byte-identical to contract.py from the
previous AIHEROES_LOCAL_MODEL_BRIDGE_v0.1.0 archive (SHA-256
58b282d78ffa0ae65e008b155f4ea8dbdcc12edb91ff562b9edf13ebeafdd554).
It is a schema/candidate helper, NOT the complete transport installation.
Tools/verify_model_request_contract.py parses the four Unreal outputs through
that original strict schema, checks game facts and rejects 12 malformed cases.

New test groups: VisibleAllowlist; RoadGuardAndBudgetParity;
SeedBattleAndArmyLocks; MalformedInputsClearOutput; StableOrderAndReadOnly;
LegacyGoalsAndSlots. Full regression counts come from the actual run, not this
document. Requested graphical widths are 1280, 960 and 640 at height 720.
Image integrity or scripted success is not a whole-interface accessibility pass.

## Primary references consulted 2026-10-29

- Epic Run Automation Tests: command-line filtering and JSON ReportExportPath.
  https://dev.epicgames.com/documentation/unreal-engine/run-automation-tests-in-unreal-engine
- Epic TArray::Sort: pointer arrays pass dereferenced values to the predicate.
  https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Core/TArray/Sort
- Epic FFileHelper::SaveStringToFile: encoding and write flags for QA exports.
  https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Core/FFileHelper/SaveStringToFile
- llama.cpp server: schema-constrained chat completions; interface only, not a
  claim that a real checkpoint or this engine integration ran inference.
  https://github.com/ggml-org/llama.cpp/blob/master/tools/server/README.md

Rollback: remove only this module's additive files and reverse its two header
and one BeginPlay hook edits after checking for later changes. Scoped baselines
are under Saved/QA/ModelRequestWork_20261029. Never reset the shared worktree.
No commit/push/deploy, global configuration or player-save replacement belongs
to this change. Windows compilation is distinct from Android acceptance.
