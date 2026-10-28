# AIHEROES — strategic connectivity, proposal contract v1

Status: NATIVE_CPP_KERNEL_VERIFIED / UNREAL_ADAPTER_NOT_VERIFIED / NOT_WIRED_TO_CAMPAIGN.
Observed: 2026-10-28. This is original AIHEROES code, not imported Heroes III logic or assets.

## Purpose

Replace the assumption that every destination is connected by a straight line with
an explicit graph of known positions and permitted directed transitions. This change
is deliberately NOT connected to the active GameMode yet: changing the travel economy,
saves, forecasts and authorization together needs a separately tested integration.
The old AIHeroesTurnRules::TravelCost remains the live campaign rule in this slice.

## Production code

- Public/AIHeroesAdventurePathCore.h: portable C++17 production algorithm.
- Public/AIHeroesAdventurePath.h and Private/AIHeroesAdventurePath.cpp: Unreal types adapter.
- Private/Tests/AIHeroesAdventurePathTests.cpp: eight authored Unreal test groups.
- Tests/adventure_path_native.cpp: executes the SAME production core outside Unreal.

Core and adapter are not two independently implemented path algorithms. The adapter
converts Unreal values, invokes the core and maps the result back to stable FNames.
This avoids declaring a Python reimplementation to be proof of the C++ code.

## Input / output

Graph: revision, nodes and directed links. Node: stable ID, discovered, passable, hostile guard.
Link: known endpoints, positive integer movement cost, enabled/disabled.
IDs: 1..64 ASCII letters, digits, underscore or hyphen, case-insensitive; NONE is reserved.
Localized display names are separate and must not be used as IDs.
Maximum: 128 nodes, 1024 links, link cost 1..1,000,000. Duplicate IDs or directed links,
self-loops, unknown endpoints and invalid disabled links reject the ENTIRE proposal.
Bounds are checked before expensive work. Distances use 64-bit integers.

Output states:
- InvalidInput: malformed graph, origin or budget; no partial path.
- Unreachable: no known legal path; no hidden route/cost disclosed.
- NeedsMoreMoves: complete known path with exact deficit; no automatic day advance.
- Ready: reachable within current budget; still NOT player authorization.

Result includes start and destination, total cost, missing movement, graph revision and
whether the destination requires combat. It contains no win prediction.
Start == destination is a valid zero-cost no-op only for a valid known safe origin.
Hostile guarded nodes can be destinations but cannot be intermediate transit nodes.
A separately authored legal bypass is allowed. This is NOT a general zone-of-control
implementation: the map author must encode compulsory encounters in connectivity.

## Determinism and authority

Dijkstra has bounded O(V^2 + E) work. Equal-cost candidates use normalized lexical
settlement order, independent of input array order. This is a deterministic tie rule,
not a claim that the full route string is globally lexicographically minimal.
There are no clocks, random draws, global mutable state, actors, filesystem/network
calls, resource writes, battle commits or hidden day transitions in the production core.
Tests use an explicit PRNG seed to generate cases; production does not use that PRNG.

Before future execution the GameMode must reconstruct current graph/snapshot and
compare revision plus recomputed legality/cost. Revision is evidence, not a permission token.
A result must never be trusted just because an AI repeats a previous path.

## Actual verification

Native Windows MSVC build: PASS, /std:c++17 /EHsc /W4 /WX.
Native test process: PASS, 5,872 assertions.
Generated cases: 200 graphs, 1,600 queries, fixed seed 20261028.
Independent distance oracle: Floyd-Warshall with guarded intermediate vertices excluded.
Coverage: weighted detours, stop-at-guard, legal bypass, hidden/blocked nodes,
directed/disabled links, budget/no-op, stable ties, invalid input, maximum chain,
revision change, path edge existence, accumulated cost and expected terminal status.
This is finite test evidence, not an exhaustive proof of every possible graph.

Native receipt:
Saved/QA/AdventureNative_20261028T203556776Z/receipt.json

Unreal build attempt BEFORE extracting the portable kernel:
Saved/QA/AdventurePath_20261028T202907034Z/build.log
Result: blocked before source compilation because Live Coding was active in an existing
Unreal session. That process was not closed or modified. The final Unreal adapter and
its eight test groups remain NOT RUN; native PASS does not imply Unreal integration PASS.
The refreshed isolated verification script now copies the shared kernel too.

Commands from the repository root:
    powershell -NoProfile -File Tools/verify_adventure_native.ps1
    powershell -NoProfile -File Tools/verify_adventure_path.ps1

The second command requires a compatible idle Unreal build environment. Neither script
installs packages, edits global settings, publishes a build or writes player save slots.

## Explicit non-claims

No current in-game pathfinding replacement. No authored road/terrain cost model.
No rendered fog-of-war or world discovery system. No multi-day travel scheduling.
No ship/airship/flight/portal policy unless encoded as ordinary allowed links by a future
validated provider. No battle forecasts. No APK, Android performance result, live LLM,
multiplayer authority, shipping build or whole-game regression PASS.

## Integration acceptance / rollback

Next bounded step: author a small versioned map graph and test advisor/executor parity
for identical movement costs, a closed pass, a guarded destination and a changed revision.
Persist stable IDs with an explicit map-schema migration before altering existing saves.
Keep old distance-based campaigns versioned; do not silently reinterpret their budgets.
Only then wire the path preview and confirmation UI to the same graph provider.

Rollback for this isolated slice: remove only the new AdventurePath files and its native
harness after checking concurrent changes. No existing gameplay file was edited by this
slice. Do not reset other working-tree modifications or overwrite another developer's work.
