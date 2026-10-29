# AIHEROES — Field Dwellings and Local Recruitment v1

Date: 2026-10-28/29 (Europe/Berlin UTC observation separate from Windows local time). Repository: `C:\github\AIHEROES`.
Status (updated after source-level/GUI verification): **WIN64 GAME + EDITOR BUILD PASS | LATEST UNREAL AUTOMATION 138/138 PASS (earlier 137/137), including six AIHeroes.Field tests | DEDICATED 1280x720 GAME GUI QA PASS | USER SAVE FILES UNCHANGED**. Actual packaged Win64 Game/Android release not implied; detailed frozen evidence in `Docs/FIELD_DWELLINGS_QA_2026-10-28.md`.

## External inspiration, original game implementation

Official Horn of the Abyss documentation https://h3hota.com/en/documentation explains that dwellings owned by a player accumulate units week by week while unowned dwellings reset to one weekly production allocation. The official zone template description https://h3hota.com/en/template-format also distinguishes the topology of zones/guarded connections from the creature dwelling objects placed *inside* those zones.

**AIHEROES does not copy** HotA content, specific creature types, artwork, maps, quests, object geometry, prices or production quantities. This feature uses only the strategic concept. Unreal Engine 5.8 SaveGame reference: https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine .

## The playable rule

There are two distinct 3D camp markers physically near existing authored nodes, not replacement world nodes and not hidden paths:

| Field object | Secure parent | Unit | Initial neutral stock | Weekly owned growth | Stock cap |
|---|---|---|---:|---:|---:|
| FOREST CAMP | FOREST | Militia | 5 | +5 | 50 |
| ORE AERIE | ORE | Scouts | 2 | +2 | 24 |

- New map topology: **unchanged (HQ, FOREST, ORE, OUTPOST, RUINS)**. An existing guarded road must be traversed to reach a field site.
- Each camp is independent from HQ stock (10/4 initial, own 7-day growth) and from neutral defenders. A site remains neutral until the player (1) captures the parent node through a guarded encounter, (2) moves the hero to the secured node by a human order, and (3) presses **FIELD 3** (keyboard 3, mouse/touch HUD action ID 204). Q/E are reserved for camera rotation, not field orders.
- Claiming does not cost resources, add soldiers or use an AI prediction. A **subsequent player FIELD 3 command** buys one fixed full militia/scout pack at the existing standard price and adds soldiers to the army. One 3 action can never claim and buy simultaneously.
- Unowned sites start with one stock pack and reset (not accumulate) on each Day 7→8, 14→15, etc. A player-claimed dwelling accumulates a pack on each boundary, subject to cap. Any loss of parent territory releases the dwelling on the next valid day transition; the claim cannot survive a saved game that declares the parent hostile.
- Recruiting is a preflighted atomic contract: inventory bounds, parent ownership, hero co-location, claimed status, affordability, canonical two-stack army, 500,000 max HP per stack, no duplicate UnitIds. Any failure returns without changing inventory, resources or army. No battle is simulated by purchasing.
- GameMode's EndTurn preflights field stock independently alongside daily resource income, neutral-week growth, HQ production and rival action before **one** campaign commit.
- Distinct map decorations have actor tags `AIH_Field_ForestCamp` and `AIH_Field_OreAerie`. They have no collision, so a touch/click still selects the parent world node. Their visibility is gated by the same fog-of-war knowledge predicate used for world nodes; hidden areas cannot leak field locations.
- The **FIELD 3** HUD control shares source rectangle and hit-test rules with keyboard/mouse/touch. It has action 204 and does not shift 0..12, 200..203, modal controls or live battle 100..103. It is invisible during live combat and does not punch through a modal dossier or the base shop.
- Each selected field node shows an on-map field label and selected summary. The user sees whether the site is claimed and how many recruits remain.

## SaveGame compatibility without destructive migration

Additive tagged v1 fields in `UAIHeroesCampaignSave`:
`bFieldDwellingsInitialized=false; bForestCampClaimed=false; ForestCampUnits=0; bOreAerieClaimed=false; OreAerieUnits=0`.

- Legacy saves omit them and validate as the exact `false / false / 0 / false / 0` tuple.
- On load, GameMode reconstructs one neutral pack per site **in memory**; no retroactive owner claim or prior week growth is invented, and loading alone does not rewrite a player file.
- New saves require the marker and bounded values; claimed states without owning FOREST/ORE are refused.
- `FAIHeroesSaveSystem::SameSnapshot` compares the new fields after A/B disk read-back.
- `CampaignFingerprint` includes both claimed flags and both inventories so live battle and base-purchase confirmation can reject unanticipated concurrent state drift.
- No new `FAIHeroesSavedNode` map elements are added; road V1 saves continue with V1, road V2 saves continue with V2. Fog-of-war optional fields remain independently validated.

## Evidence required to promote the feature

1. Editor Win64 Development build after addition of `AIHeroesFieldDwellingTests.cpp`, not an earlier build lacking this test registration.
2. All six `AIHeroes.Field.*` tests PASS: neutral reset vs accumulation, claims & ownership loss, atomic recruitment, v1 save rotation, responsive mobile hitboxes, end-to-end GameMode field capture/pack/week.
3. Full AIHeroes Automation regression; no old subsystem failures.
4. Isolated 1280×720 game-window smoke: click FOREST, explicitly authorize/finish the fight, FIELD button claim, 3 buy once, 3 out-of-stock refusal, seven NEXT DAY human orders, field week accrual, 3 buy again. Earlier QA receipts/screenshots used the old E binding; they are historical, not evidence for the new key.
5. Two screenshots, QA log, no duplicate battle commit, and exact pre/post real SaveGame inventory comparison (names/size/mtime/SHA256), all observed and hashed.
6. Read-back receipts list exact Windows paths, bytes, hashes and test outcomes. No commit/push/release/devices.

## Nonclaims

This is an original two-field prototype, not full HotA map generator or a networked dwelling economy. Actual phone touchscreen QA, Android SDK/NDK APK, real LLM inference, full hero growth and multi-player enemy re-capture remain outside this slice. Other agents may edit this working tree; test source/binary parity must be verified before any release.
