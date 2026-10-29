# AI HEROES — Zone Expedition V2 (separate opt-in generated campaign)

Status: **NEW EXPERIMENTAL MODE**. Built source; verification is
separately recorded in Docs/ZONE_EXPEDITION_QA_2026-10-29.md.
Date: 2026-10-29, Windows, UE 5.8.4.

## 1. Problem and research basis

The five-site v1 campaign has actual UI, combat, player-owned attack
confirmation, fog, saves and deterministic economy, but its world
topology is hardcoded. A second, proven generator renders 7..11-zone
3D maps but has no playable campaign state. These subsystems should
not be joined by silently interpreting 5-site SaveGame fields as
7..11-zone data.

Official HotA template specification:
- https://h3hota.com/en/template-format
- https://h3hota.com/en/templates

[FACT] In HotA, random maps are based on a template describing
zone connectivity, guarded passage options, and roads. The
precise placement and contents can be randomized separately.
[INTERP] AI HEROES should implement its OWN dynamic zone-game
rules with reachability and fog validation prior to decorative
PCG. HotA's assets, data, RMG format and factions are not copied.

Epic Unreal 5.8 SaveGame guide:
https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine

[FACT] Unreal supports different USaveGame classes and distinct slot
names. For small paused-turn snapshots synchronous SaveGameToSlot is
supported, but disk writes still require read-back and validation.

## 2. Player experience (explicit alternate Windows GameMode)

Launch in Unreal `-game` with this **explicit map URL**:

`/Engine/Maps/Entry?game=/Script/AIHeroes.AIHeroesZoneExpeditionGameMode`

Optional bounded arguments: `-ZoneSeed=20261029 -ZoneBranches=3`.

Default campaign class and `Config/DefaultEngine.ini` are unchanged.
The separate ZonePreviewGameMode stays fully read-only.

At campaign creation:
- 7 base sites: HQ, SUPPLY, FOREST, ORE, PASS, OUTPOST, RUINS;
  0..4 generated side-path sites WAY_01..04.
- 4 known sites: HQ, SUPPLY, FOREST and ORE, with only 1 owned HQ.
  The other zones and their links are hidden.
- A hero starts at HQ with Militia x8 and Scouts x3,
  four-day-year? **No**: 8 movement points each day.
- Routes have seeded per-link costs 1..4. The original validated
  pathfinder blocks crossing a still-hostile guard, even if
  the guarded zone is known. No hidden-node shortcut exists.
- Visiting a site reveals its direct neighbors, but only after
  a successful, player-authorized arrival or capture.
- Forest, ore, pass, outpost and ruins have increasingly
  difficult original encounters. The full Grid V2 offline AI
  picks tactical actions AFTER the player confirms.
- This prototype resolves Grid V2 instantly. It is **not**
  the active campaign's animated live 3D tactical stage.
- Capturing sites gives one-time bounded resource/DATA rewards.
  Each player-ended day restores 8 movement, pays 40 Gold, and
  pays captured-mine income; the player recruits 5 Militia for
  50 Gold + 5 Wood **only at HQ**. Three optional offline
  cognitive protocol upgrades spend Gold/DATA. No model
  weights are loaded.
- OUTPOST and RUINS capture is an original prototype victory
  predicate; it is not a reimplementation of HotA victory.

Controls:

| Key / interaction | Meaning |
|---|---|
| Mouse click | Select only a discovered 3D zone; never attack |
| Tab | Cycle through currently known sites |
| Enter | Recompute current map, movement and army; **player-confirmed** move/attack |
| Space | Human advances one strategic day |
| R | Human recruits at HQ |
| P | Human unlocks offline AI protocol |
| K | Human saves **only** the expedition v2 namespace |
| L | Human loads a verified v2 snapshot **only if seed and branch count match** |

This is a keyboard/mouse Windows mode. Android touch, device
performance, sound, art and UI onboarding are not yet verified.

## 3. Architecture

Single source of truth for topology:
`FAIHeroesZoneTemplate::Generate` and portable
`AIHeroesZoneTemplateCore::Validate`.

`FAIHeroesZoneExpeditionRules` owns bounded, pure state evolution.
Its `PlayerView` returns ONLY discovered node IDs and roads whose
both endpoints were discovered. The full generator graph is
an internal validator only, not an AI prompt or playable hint.

`Preview` is read-only, cannot move the hero or request a battle.
Only `ConfirmPlayerDestination` invokes the existing
`FAIHeroesGridBattle::RunToEnd` after checking:
- a valid schema-2 state and exact seed+map revision;
- the target is known and reachable through the actual known roads;
- movement points cover the exact route;
- the target has defenders and the army is alive if combat is required;
- reward, visibility, army and campaign state validate after resolution.

All changes are computed into a local **candidate**; they are
committed only once after the final validation, no partial spending.
If a defender wins, the hero remains at the previous site;
movement and troop losses are applied only for the completed battle.
There is no cache-based attack authorization.

Discovery is intentionally not just a bitmask controlled by the
caller: validation recomputes the exact known-site set from HQ +
already-cleared sites' topology. Forged knowledge, stale map
revision, changed army stats, illegal position or apparent victory
through an undefeated checkpoint fail closed.

Game presentation is separate:
- `AAIHeroesZoneExpeditionGameMode` — explicit opt-in controller
  and 3D actors (engine built-in sphere/cube meshes), 3D
  camera, selection and current state;
- `AAIHeroesZoneExpeditionHUD` — read-only status;
- `FAIHeroesZoneExpeditionRules` — the actual game rules;
- `FAIHeroesZoneExpeditionStore` — explicit SaveGame authority;
- `UAIHeroesZoneExpeditionSave` — typed persistent state.

## 4. Save and migration boundaries

There are now multiple independent experimental schemas:

1. **CampaignSave v1**: original five-site human campaign.
2. **ProceduralCampaignSave v2**: a separate parallel
   generated-world representation (source-only preparation).
3. **ZoneExpeditionSave v2**: this playable generated-world
   sandbox, in independent A/B `AIHEROES_ZoneExpedition_v2_A/B`.

These schema-2 records are NOT interchangeable. No automatic
conversion, import, fallback or aliasing between any of the three.
Unifying the two future-world schema candidates requires a
separate ADR, migration tool and test evidence.

A/B saving occurs ONLY on human K. It validates the entire state
before writing, alternates to avoid overwriting the last valid copy,
and reads the new snapshot back for all field-by-field equality.
A corrupted alternate loads the other valid copy. If both copies
exist but fail, writes are blocked until manual repair; there is
no silent overwrite of player evidence.

Tests save to GUID-specific
`AIHEROES_ZoneExpedition_Test_<GUID>_A/B` and clean ONLY
those test-owned filenames. No test is allowed to write any
player's `AIHEROES_Campaign...` slot, or real expedition slot.

## 5. Known limitations

- One human hero and one generated map per session; loading a
  different seed is rejected rather than changing the world under
  actors without preflight.
- Five deterministic enemy encounter definitions are currently
  basic balance hypotheses; no sophisticated AI foe economy,
  separate NPC hero, diplomacy, spell tree or gear progression.
- There is no animated Grid V2 stage in this separate mode.
- Terrain is a prototype plane with built-in materials;
  no copyrighted HotA content, PCG terrain or world-atmosphere art.
- The dynamic map is not the default campaign.
- Synchronous SaveGameToSlot is suitable for this tiny snapshot;
  for production autosaves, switch to AsyncSaveGameToSlot
  with callback receipt and rollback/recovery testing.
- 1B/3B/8B are **themed cosmetic slots in the original campaign**,
  and this experimental mode does not load a real model checkpoint.

## 6. Acceptance and next step

Test `AIHeroes.ZoneExpedition.*` in Unreal Automation:
new state, known-only view, player-confirmed battle, after-visit
reveal, forged checkpoint rejection, day/recruitment/upgrade costs,
A/B saved-state readback and fallback, 640 seed+branch combinations.
Then run the full `AIHeroes` suite, a real Windows viewport QA
and file-hash comparison of all existing player SaveGame files.

Production promotion is **not automatic**. First compare
ZoneExpeditionSave v2 to the parallel ProceduralCampaignSave v2
and decide on one canonical schema; add a versioned migration,
independent UI/mobile and release QA.
