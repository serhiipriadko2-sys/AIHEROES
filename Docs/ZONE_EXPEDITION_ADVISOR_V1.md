# AI HEROES — Read-only offline adviser for procedural Zone Expedition

Date: 2026-10-29. Scope: independently selectable UE 5.8 ZoneExpedition.
Status: IMPLEMENTED AND WINDOWS-VERIFIED (editor build, 4/4 targeted,
208/208 full Unreal tests, real H + Shift+H window QA). No real neural
inference, Android device QA or production deployment in this feature.

## Design motivation and external references

The official Horn of the Abyss Template Editor documentation describes
interconnected zones, distinct guarded and wide connections, and optional
roads as separate structural rules:
https://h3hota.com/en/template-format
https://h3hota.com/en/templates

[INTERP] In AI HEROES, the hero may offer helpful destination advice,
but the map generator and its full topology MUST remain hidden from
player-level reasoning until reconnaissance. The 3D rendering of a node
is not permission to command movement, and an adviser forecast is NOT
a human confirmation to attack.

Epic's UE 5.8 SaveGame guide distinguishes synchronous and async saving:
https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine
This adviser has zero SaveGame calls and makes no disk writes. Future
snapshot-backed advice must remain read-only and verify current state.

## Actual implementation

- Source/AIHeroes/Public/AIHeroesZoneExpeditionAdvisor.h
- Source/AIHeroes/Private/AIHeroesZoneExpeditionAdvisor.cpp
- Source/AIHeroes/Private/Tests/AIHeroesZoneExpeditionAdvisorTests.cpp

Bindings in explicit opt-in ZoneExpedition GameMode and its HUD:
- H: ask offline hero to recommend one reachable, unclaimed, discovered
  zone. Selection, movement, fights, recruitment and spending unchanged.
- Shift+H: inspect the selected visible zone and report its true known
  route cost, guarded/no-guard status and movement shortfall.
- Enter remains the ONLY player authorization for movement and battle.
  Pressing H never silently selects or enters the recommended site.
- The HUD identifies the controls and tags output READ-ONLY HERO ADVICE.

The implementation consumes ONLY
FAIHeroesZoneExpeditionRules::PlayerView (validated player-side graph).
It never imports FAIHeroesZoneTemplate's full validation oracle.
Advice excludes hidden sites completely. Selected hidden, unknown and
forged names receive the SAME neutral explanation and no route cost.

Uncleared *visible* sites are evaluated with the authoritative
FAIHeroesAdventurePath::Find result and current movement budget.
An unguarded hub can be suggested; guarded sites require living troops.
The adviser does not run a battle for a covert casualty forecast:
it reports "UNKNOWN" rather than pretending to know a battle result.
A deterministic heuristic ranks only visible, reachable choices using
movement points and low-reserve pressure. Cognitive protocol tiers change
the depth of explanation and the resource/guard heuristic, but are not
real LLM checkpoints or RPG army upgrades.

The advice carries the visible-graph SourceRevision as a staleness
marker only. No boolean, reference, text, rank, route revision or
returned struct authorizes GameMode input, SaveGame operations or an
expedition state commit. This protects player control.

## Acceptance gates and non-claims

- Baseline non-procedural five-site campaign unchanged.
- Base ZoneExpedition gameplay and A/B save formats unchanged.
- A freshly built Win64 Editor and 4 added Unreal Automation tests.
- Known-only, seeded/adversarial fog case: no PASS/RUINS leaks.
- Read-only property: movement, HP, Gold, Data, discovered/cleared
  zones, hero position and journal unchanged after advice.
- Zero army / zero movement / forged map revision fail safely.
- Tier 0 vs tier 3 explanations differ but route prices do not.
- 128 seeds x five branch counts = 640 adviser target checks.
- Prior human-confirmed battle reveals a new checkpoint but an
  inspection cannot capture it or launch a second fight.
- Full AIHeroes Automation suite and exact compiled-source evidence.
- Window QA: visible H / Shift+H advice should be checked without
  issuing any movement/attack or SaveGame writes.

Limits: this is a deterministic offline rule-based adviser, not an
actual 1B/3B/8B neural model or a network model adapter. The game does
not grant the adviser combat authority or expose unknown enemy units.
The active default five-site game retains its separate mature
FAIHeroesAdvisor. This addition is limited to the explicitly launched
procedural 7–11-zone mode. Android touch interaction and UI readability
on physical devices are unverified.

## Verified Windows acceptance, 2026-10-29

- Editor: Tools/build_aiheroes_editor.ps1, Win64 Development PASS;
  Docs/BuildRuns/20261029T132326707Z/compiled.json
  was tied to source/config hashes and the compiled editor DLL.
- Targeted: AIHeroes.ZoneExpeditionAdvisor.* 4/4 Success, 0 warnings:
  Docs/AutomationRuns/20261029T132355357Z/index.json
- Complete suite: AIHeroes.* 208/208 Success, zero failures,
  skipped tests or warnings, with build_verified=true:
  Docs/AutomationRuns/20261029T132743622Z/index.json
- The seed sweep covers 640 initial advisor states. The test asserts
  a chosen site is actually discovered, reachable and human-owned
  actions remain untouched. A post-battle test compares exact reflected
  campaign state before and after inspection, with negative controls.
- Window-targeted game QA using only its own verified game PID:
  Tools/qa_zone_expedition_advisor.ps1 -Seed 20261029 -Branches 3.
  H advised visible SUPPLY for 2 movement points. Shift+H inspected
  selected FOREST for 3 moves, explicitly stating battle outcome
  UNKNOWN. Neither action committed a battle, moved the hero or saved.
  Subsequent Enter confirmed FOREST, Space ended the day, and separate
  Enter confirmed ORE; 2 actual offline Grid V2 battles PASS.
- Four fresh 1280x720 viewport-native GPU images were verified for
  non-blank content, bytes and matching SHA256 and copied to:
  Docs/VisualRuns/ZoneExpeditionAdvisor_20261029T133133956Z/
  Stages: initial, forest, day2, ore; total PNG bytes: 1574835.
  Copy of test receipt is in that folder, verified byte-identical.
- Save directory file-name/count and SHA256 map was compared before
  and after the window run; user saves unchanged; no K/L commands.
  Test window closed gracefully, no desktop-wide input.
- The first targeted suite was NOT all green: a test assumed fewer
  than seven discovered sites after FOREST. That assumption ignored
  legitimately revealed side routes. It was replaced by exact-state
  CompareScriptStruct equality and direct cleared-mask/moves assertions;
  final targeted 4/4 and full 208/208 then passed.

See Docs/ZONE_EXPEDITION_ADVISOR_RECEIPT_2026-10-29.md for the
file-by-file hash manifest and verified lifecycle boundaries.

Rollback: remove only the three new ZoneExpeditionAdvisor files and
surgically undo the H/Shift+H includes, bindings and HUD text after
read-back. Copy-original backups were saved under
Saved/QA/ExpeditionAdvisorBefore_20261029. Do not reset other agents'
changes, remove the old campaign or rewrite any player's save slot.
