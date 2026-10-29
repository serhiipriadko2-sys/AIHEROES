# AI HEROES — Three-way Known-Frontier Dossier (v1)

Date: 2026-10-29. Scope: explicitly selected ZoneExpedition Unreal 5.8 GameMode.
LIFECYCLE: experimental feature; Win64 Editor build, 216/216 full
Automation, and real 5-frame window QA have PASS receipts below. This
document is not a packaged release or proof of Android compatibility.

## Research and design constraints

Horn of the Abyss documents template zones, guarded connections and
independent road policies: https://h3hota.com/en/template-format
and https://h3hota.com/en/templates. AI HEROES reuses that abstract
design distinction only; code, factions, maps, art, music and assets
remain original. The rendered map is never a source of hidden route
information or battle permission.

Unreal Engine 5.8 supports HUD DrawText for screen-space overlays:
https://dev.epicgames.com/documentation/unreal-engine/BlueprintAPI/HUD/DrawText
and JSON Automation reports for regression tests:
https://dev.epicgames.com/documentation/unreal-engine/review-test-results-in-unreal-engine

## User workflow

- H continues to advise one known reachable destination.
- Shift+H continues to inspect only the selected known destination.
- **D** toggles a separate read-only three-way comparison panel.
- **Esc** also closes the panel.
- While D is open, Tab/click, Enter, Space, R, P, H, K and L are
  blocked. This prevents stale action from the visible snapshot
  and accidental save/movement during a modal report.
- Closing D does NOT select the top-ranked destination, execute a
  movement, start battle, advance a day, spend, upgrade or save.
- Once closed, only the human's separate Enter confirms movement/combat.

Panel shows:
1. Up to three valid discovered, unclaimed alternatives;
2. Exact currently known route cost from FAIHeroesAdventurePath::Find;
3. Rule-based score, movement shortfall, guarded status and
   non-calibrated army reserve warning;
4. A short reason (supply route, wood shortage, ore shortage, or guarded
   checkpoint) based solely on player-visible or player-owned facts.

No percentage chance to win, HP casualty forecast, enemy strength,
magic knowledge, LLM answer or hidden route cost is invented or shown.
"GUARDED | OUTCOME UNKNOWN" is intentionally retained even at maximum
offline protocol tier. A small friendly army receives a caution rather
than an unsupported battle outcome.

## Data flow and bounded reasoning

Existing FAIHeroesZoneExpeditionRules::PlayerView is called first,
validating the SaveGame and reconstructing only the currently
discovered graph (not the full generator oracle). An invalid state
returns bValid=false with zero options and a generic error.

For each unclaimed visible node, a read-only pathfinder calculates the
shortest legal known road. Unreachable routes are omitted; destinations
which need more moves are retained as explicitly non-actionable
future options. Never send hidden node IDs to this logic.

The current deterministic score is:

    (unguarded ? 130 : 85)
    + protocol-specific known-resource preference
    - 13 * exact movement cost
    - 180 - 12 * missing movement (if movement is insufficient)
    - 500 (if guarded and no living troops)
    - 65 (if guarded and total friendly HP < 48)

This is a ranking heuristic, NOT a probabilistic combat model or
evidence of actual human preference. Rationale is displayed and
can be changed by future playtesting without changing authority.
Ties: lower movement cost first, then ascending site ID. At most
three ranked choices appear. No direct action callback or save key
runs within the panel.

SourceRevision detects map-visibility staleness but does NOT
represent authorization, signed player consent or model truth.

## Files in this slice

- Source/AIHeroes/Public/AIHeroesZoneExpeditionAdvisor.h
- Source/AIHeroes/Private/AIHeroesZoneExpeditionAdvisor.cpp
- Source/AIHeroes/Private/Tests/AIHeroesZoneExpeditionDossierTests.cpp
- Source/AIHeroes/Public/AIHeroesZoneExpeditionGameMode.h
- Source/AIHeroes/Private/AIHeroesZoneExpeditionGameMode.cpp
- Source/AIHeroes/Private/AIHeroesZoneExpeditionHUD.cpp
- Tools/qa_zone_expedition_dossier.ps1
- Docs/ZONE_EXPEDITION_DOSSIER_V1.md (this file)

Five existing source files were byte-for-byte backed up to
Saved/QA/ZoneDossierBefore_20261029 before editing.
No edits to the legacy fixed five-site campaign, existing v1 or v2
SaveGame classes, their slot names, GitHub, Iskra canon, Android
settings, network features or neural model bindings were intended.

## Acceptance gates

- Clean Unreal Editor Win64 Development build with source/config hashes.
- Four AIHeroes.ZoneExpeditionDossier.* Automation tests for known
  three-way ranking, exact path cost/guard parity, no army/no moves,
  640 seed+branch combinations, tampered map, post-battle visibility,
  stable ties, and no state mutations.
- Full AIHeroes suite plus compiled-manifest parity, no warnings,
  failures, missing cases or skipped tests.
- Original owner-window QA, D -> GPU screenshot -> blocked Enter/Space
  -> Esc -> player-confirmed FOREST, NEXT DAY, confirmed ORE.
- Original H / Shift+H still work before D.
- Four or five GPU frames captured with Unreal FScreenshotRequest
  (never desktop screenshot or PrintWindow black swapchain).
- Snapshot filename/count/SHA256 of existing player save files
  unchanged, no K/L commands.
- Verified bytes, hashes, and a standalone artifact receipt.
- No automatic migration between incompatible schema-v2 candidates.

## Verified 2026-10-29 Windows evidence

- Editor Win64 Development: PASS; 204 pinned input files, build verified.
  Docs/BuildRuns/20261029T141711156Z/compiled.json
- Frozen tested DLL, 2589696 bytes, SHA256:
  e36d684c6b446e3f291698076164e3e93eff6b2a83488c900770cfc0ac362383
  Saved/QA/ZoneDossierVerified_20261029T141711156Z/UnrealEditor-AIHeroes.dll
- Four new dossier tests PASS:
  Docs/AutomationRuns/20261029T140903700Z/index.json
  The focused run was build_verified=false; it tested the existing
  binary before later final source-pinned full regression.
- Whole AIHeroes suite: 216/216 PASS, failed=0, warnings=0,
  notRun=0, build_verified=true:
  Docs/AutomationRuns/20261029T141959618Z/index.json
  Source and binary drift checks were run by the wrapper.
- The first coordinated Editor build was explicitly rejected by
  provenance because concurrent art source files were added DURING
  compilation, even though UBT compiled successfully. The later
  204-input build passed after the shared source tree stabilized.
- Real separate game mode: PASS with key H, Shift+H, D, Enter and Esc,
  then human-confirmed FOREST and ORE battles, Space next day.
  The D panel blocked an Enter and a Space while open.
  VisualRuns/ZoneExpeditionDossier_20261029T141012189Z contains
  five genuine Unreal GPU screenshots and copied QA receipt.
  Total PNG bytes = 1795530; all screenshots 1280x720.
- Captured example:
  SUPPLY 2 moves (no guard), FOREST 3 (guard outcome unknown),
  ORE 6 (guard outcome unknown). Scores are transparent heuristics.
  I reviewed the final D screenshot: labels are readable over a dark
  modal panel. The map behind it remains visible but partially covered.
- No save command in the QA process; counts, filenames, SHA256 of
  preexisting user .sav files matched before and after. QA-owned
  Windows window closed gracefully.

Status boundaries:
PASS only with actual build/test/log hashes and read-back.
PARTIAL for heuristic UX, balance, original art, physical Android,
real inference and non-shipping demo.
NOT CLAIMED: packaging, release, branch merge, live deployment or
GameMode replacement.

Delta D Omega Lambda:
Delta bounded transparent strategic comparison for known destinations;
D source + test/viewport receipts;
Omega <=95% where observed;
Lambda revise after playtesting, Android UI/accessibility, and validated
battle-ready forecasts become available.
