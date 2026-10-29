# AIHEROES — Human-authorized supply convoy v1

Date: 2026-10-29. Local implementation project: `C:\github\AIHEROES`.
Current state: **implementation in progress; not called tested/published until named receipts have read-back**.

## SIFT — external design references and distinction

Official HotA documentation https://h3hota.com/en/documentation records externally owned dwellings' weekly creature accumulation and local visit/recruitment. Official HotA random template description https://h3hota.com/en/templates and zone/roads reference https://h3hota.com/en/template-format distinguish connections, guards, roads and zones. This feature borrows only the abstract interaction of **strategic road security and local garrison supply**; **all unit/cost/escorting rules and UI are original AIHEROES mechanics**. It does NOT represent HotA's original logistics system, or reproduce any asset, map, faction, unit, audio or copyrighted code.

Epic's Unreal Engine 5.8 SaveGame model https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine permits additive state saves and disk-read-back; Unreal Automation guidance https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-test-framework-in-unreal-engine says tests must not assume runtime state and must clean generated files. Consequently, this feature intentionally adds **no new SaveGame property** and runs all fixture tests in disposable worlds or a dev-only disposable process.

## Player-facing mechanic

Player owns a Headquarters (HQ), forest and ore field dwellings can be claimed by visiting local sites with **E** (existing mechanic). A new **CONVOY X** operation delivers **one complete standard pack** to the player's hero if physically at HQ. This does **not** move the hero and does not automatically recruit when a zone or road changes.

The player must:
1. Discover FOREST/ORE (no knowledge leak through a convoy hint).
2. Capture the selected source territory and claim its camp locally with the existing E command.
3. Capture **OUTPOST** to disable its raiders and secure the merchant escort corridor.
4. Return hero to HQ and **select the source field** (FOREST/ORE).
5. Press **X / CONVOY** once to see the price and movement cost **without spending**.
6. Press X / CONVOY a **second time without altering state** to confirm one delivery.

Supply uses the authoritative HQ-to-source link of travel V2, not 3D mesh geometry. Legacy V1 retains its direct-distance world travel, but still needs the owned OUTPOST, source claim, HQ and hero at HQ. This transport security requirement is a **new AIHEROES balance policy** motivated by existing rival raids; HotA does not dictate it.

| Source | Pack | Local price | Convoy surcharge | Final | Strategic move cost |
|---|---|---|---|---|---|
| FOREST CAMP | 5 Militia | 50 gold + 5 wood | +15 gold | **65 gold + 5 wood** | **1** |
| ORE AERIE | 2 Scouts | 70 gold + 4 ore | +15 gold | **85 gold + 4 ore** | **1** |

Existing HQ recruitable reserves remain **unmodified**. Source stock decreases by exactly one purchased pack; it will accrue next week under the already installed capped local growth system. Convoys are disabled when the source is undiscovered, territory/source unowned, OUTPOST hostile, authoritative road absent, player hero absent from HQ, moves exhausted, supply stock low, resources insufficient or the 2-stack army cannot fit the packet.

## Architecture / no hidden authority

`Source/AIHeroes/Public/AIHeroesSupplyLines.h` + `Private/AIHeroesSupplyLines.cpp`: pure, deterministic `Quote` and `Dispatch`, no world actors, random seeds, file/IO, telemetry, LLM or background turns. Typed access snapshot is read-only; `Quote` stages a tentative purchase on copies. The existing FieldDwelling rules now share **one private friend-gated** `BuyAuthorized` core for both physical and escorted purchase; no fake hero locality or duplicate recruitment rule divergence.

`Private/AIHeroesSupplyGameMode.cpp`: resolves actor identities, true fog knowledge, real HQ->FOREST/ORE road from the V2 graph and hostile OUTPOST security, and only the player-confirmed request calls mutation. `PendingSupplyFingerprint` is *transient* and contains the exact current campaign state (owner flags, selected source, army, resources, hero position, move budget, fog, save slot and field stock); any change before confirming re-quotes instead of buying. Loading explicitly clears a pending quote. It is never saved, and the AI can't invoke it.

`Private/AIHeroesPlayerController.cpp`: X binding + button action205. `Private/AIHeroesHUDLayout.cpp` + `HUD.cpp`: one authoritative rectangle for drawing/hit-tests at 1280×720, 960×540, 640×720, 480×800, 360×640 and 320×640, with no overlap with GOALS/LAB/FIELD; a pending quote turns its label into **CONFIRM X**. Battles/3D replay/base-shop/advisor modal inputs block convoy actions; zoom and combat keys unaffected.

`Private/AIHeroesSupplySmoke.cpp`: **NON-SHIPPING**, opt-in `-AIHeroesSupplySmoke` fixture only. It starts a disposable game with forest/outpost secured and 10 field troops so the GUI test can prove read-only preview and next-click purchase without altering any real SaveGame. This does NOT unlock anything in normal gameplay.

## Acceptance and source/data boundaries

Required:
- Win64 Editor/Development build, new tests registered, full AIHeroes Automation suite no failures, **six AIHeroes.Supply.\*** tests.
- Check hostile outpost, undiscovered source, unclaimed field, absent HQ/hero, invalid road, exhausted movement, malformed army, near-capacity army, low resources, invalid site, stale budget; all failures must preserve original state.
- Check quoted price, human 2-action re-confirmation after state drift, one and only one pack, 15-gold escort premium, source stock decrease, 1 movement point, no HQ reserve change, no extra battle or hero move.
- Exact-window GUI screenshots: CONVOY BEFORE, CONVOY QUOTE, CONVOY DELIVERED, verified window PID & dev-only launch flag; verify real saved-game file inventory (name, bytes, UTC modification, sha256) unchanged.
- Win64 Game development binary is updated (not just Editor), `git diff --check` passes, source SHA-256 and Windows receipt read-back verifiable. SoT30 file integrity unaffected.
- Do NOT claim Android APK, device responsiveness, real LLM inference, production load balancing or release until separately tested. Current map remains the 5-node authored slice; future procedural maps will require a versioned network provider rather than a silent fallback.

**Lifecycle boundary:** local source change does not imply commit, merge, deployment, packaged installer, Android port or verified live-model execution.
