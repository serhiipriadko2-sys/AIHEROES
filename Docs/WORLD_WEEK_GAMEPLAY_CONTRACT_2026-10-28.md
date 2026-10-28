# AI HEROES — Seven-Day Frontier Pressure, original gameplay contract

Date: 2026-10-28. Unreal Engine 5.8; active AIHeroesGameMode.
Development source: Docs/HOTA_181_GAMEPLAY_RESEARCH_2026-10-28.md
Source inspiration: the strategic day/week cadence of classic turn-based
strategies, including Horn of the Abyss. The following implementation is
an ORIGINAL AI HEROES rule and is NOT a 1:1 reproduction of HotA dwellings.

## Player-facing rule

One strategic day passes when the player presses NEXT DAY or Space, after
confirming that no live battle or modal adviser is active. Daily movement
returns to four and existing mines/HQ produce income. At the transition
from day 7 to day 8, day 14 to day 15, etc., surviving hostile outposts
are reinforced, so waiting too long makes unconquered sites harder.

Reference balance v1:
- FOREST: 2 additional Wolves per surviving guard stack per week;
- ORE: 2 additional Bandits per surviving guard stack per week;
- OUTPOST: 1 additional Raider per surviving formation per week;
- RUINS: 1 additional Guardian per surviving guard stack per week;
- HQ/owned targets: no hostile reinforcement.

An individual stack has a 40-unit growth ceiling. No neutral unit grows
on non-boundary days. A destroyed stack remains destroyed; a captured
site cannot respawn enemies. All additions are whole unit HP: no healing
or fractional rounding of a wounded surviving unit. Reclaimed sites would
need a separate future siege/recapture feature.

The full info line shows the week and day [Wn Dd/7]. At a week transition
the event feed reports the additional neutral units. Player choices,
ownership and purchases remain entirely human-authorized.

## Determinism and transaction boundary

GameMode::EndTurn preflights all of the following BEFORE applying changes:
1) game/action lock and campaign day-range;
2) all live nodes present;
3) daily resource income <= save limits;
4) read-only snapshot of all five nodes and valid types/IDs;
5) FAIHeroesWorldWeekRules::PlanNextDay calculates the ENTIRE future state.
6) Separate Rival NPC behavior, if present, is preflighted against the
   *post-week* OUTPOST garrison before any player state is modified.

If ANY preflight fails, no resources, day, guards, movement or related state
is mutated. At success, the GameMode (not the rules module) commits the
prevalidated world only once as part of one EndTurn action.

The functions WeekNumber, DayInWeek and IsWeekTransition are pure and
bounded for all days 1..1000000. Maximum day cannot advance further.

## Save and replay compatibility

No new SaveGame fields. The existing UAIHeroesCampaignSave already stores
Day and each node's Defenders[].TotalHealth. Growth adds full unit HP to
those ordinary fields, and unchanged fixed units retain attack, defense,
initiative and UnitHealth.

Existing V1/V2 combat replay versions, saved battle record, player armies
and selected node are not rewritten. Future procedural maps MUST upgrade
this five-node rules contract before changing topology.

## Required negative tests

- Day 1 / 7 / 8 / 14 / 15 / 29 exact calendar transitions
- Exactly one growth per seven-day period, not daily
- Multiple neutral formations grow independently
- Owned/empty/dead units do not respawn
- Stack maximum limits / overflow do not corrupt the campaign
- Corrupted/duplicated IDs, wrong guard type, third guard formation,
  enemy HQ ownership cause all-or-nothing refusal
- Existing SaveGame schema accepts post-growth day and guard HP
- Rival NPC day-8 recruiting does not erase weekly growth or double-charge
- Real visible window: NEXT DAY x7, [AI HEROES WEEK] week=2 day=8
  recruits>0, screenshot HUD and unchanged hero-owned regions

## Nonclaims / follow-up

No actual weekly town creature-pool recruitment, multiple cities,
7-resource marketplace, fog-of-war, online multiplayer, siege battle
or real model inference in this feature. Different opponent AI choices
may change balance: tune only after replayable playtesting metrics.
No user save should be overwritten during QA.

[PROJECT ADAPTATION] References:
https://h3hota.com/en/documentation (General gameplay / Adventure map)
https://h3hota.com/en/template-format (zone connections and guards)

Development maturity is BUILD_PENDING / AUTOMATION_PENDING / GAMEPLAY_QA_PENDING
until separate concrete receipts prove the corresponding stages.
