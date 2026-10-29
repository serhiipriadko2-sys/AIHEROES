# AIHEROES — Visual QA / Amber March Review (2026-10-29)

## Scope and status

**Verified:** 10 preexisting PNGs load, meet their declared 1280×720 or 960×540 pixel dimensions, have nonempty map content, have independent SHA-256 values, and show pixel-level changes between gameplay states. This is a **static frame integrity check**, NOT a contrast-ratio audit, gameplay-run proof, matching-build cross-resolution comparison, or Android touch acceptance.

Reproducible script: `Tools/qa_zone_visual_frames.ps1`. Latest receipt: `Docs/VisualRuns/VisualFraming_20261029T160254218Z/visual_frames.json` (8,544 bytes, SHA-256 `2cd5bf975eff7987c42754061379823fa5dd33d89863e7ad05a5ffd87f8e9561`).

### Snapshot A — 1280×720 / recent ZoneExpedition

Frames: `Saved/QA/ZoneExpedition_20261029T155215303Z/{initial,dossier,forest,ore,day2}.png`.

Direct visual inspection of initial, forest and dossier:
- Ten-zone fantasy strategy map now has recognizable forest, rocks, castle, roads, mining sites, hills and settlement props instead of the old pure-gray node/plane prototype.
- Dedicated top resources, movement/day counters, left objective/exploration panels, right selection summary and ten prominent bottom action buttons.
- Human command gating is still stated on screen: AI is a read-only advisor; player confirms movement/combat. Offline-model caveat is visible.
- Dossier has three alternatives and a close control; action commands are occluded while the modal is active.

Pixel evidence: central map ROI (29–72% screen width, 25–65% height) is **99.95% non-dark** at sampled pixels with **101 distinct quantized RGB16 bins** on the initial frame. The minimum change between initial and any other state, sampled over the screen, was **22.6%**. These are descriptive sampled pixels, not overall scene fidelity or text-contrast metrics.

### Snapshot B — 960×540 / earlier compact ZoneTouchUI

Frames: `Docs/VisualRuns/ZoneTouchUI_20261029/compact_960x540/{initial,dossier,forest,ore,day2}.png`.

Direct visual inspection of initial and dossier:
- Compact layout presents ten large bottom buttons, essential player resources and an actionable read-only route modal.
- The map itself uses an older, simpler gray route-diagram treatment with extensive dark negative space. This is **not proof of a regression**: the compact frames were captured earlier than Snapshot A and lack a matching compiled-source manifest.
- Initial map ROI is non-dark at all sample points, but has **7 RGB16 color bins**, versus 101 for the newer desktop snapshot. **These values MUST NOT be used as same-build or same-resolution art-performance deltas.**
- Minimum changed screen fraction across states is **2.32%** (lower partly because the screenshot set is from a different visual generation).

## Decision and prioritized implementation

1. **P0 — one build, two viewport sizes.** Once concurrent agents stop writing, compile the exact ZoneExpedition source snapshot and capture 1280×720 and 960×540 from the **same DLL hash and scene seed**, plus a 720×1280 portrait mobile proxy. Attach that manifest to the screenshots. Never compare different art generations as responsive parity evidence.
2. **P0 — verify real legibility.** Manually sample foreground/background color pairs of mission text, item captions, selected route and modal body text. Xbox XAG 102 recommends **at least 4.5:1 contrast for normal-sized text** and 3:1 for large-sized text. Pixel variety or text appearing visible in an image does not prove these numerical ratios.
3. **P1 — consistent Safe Zone margins.** Epic UE 5.8 UMG Safe Zones documentation describes notches, overscan, and screen density/orientation. Current deterministic touch rectangles and Windows click proxies are useful, but **physical Android portrait/touch QA is not yet demonstrated**.
4. **P1 — readability hierarchy.** Prioritize objective, selected tile, risk of combat and next available action above developer-only seed and internal trace strings. Keep guard uncertainty explicit; do not reveal hidden threats or tactical results through decorative copy.
5. **P2 — visual polish without gameplay drift.** Improve terrain edge transitions, silhouette/readability of waystones/bridges and selected-route highlighting. Re-run the full Unreal Automation and exact-window tests after art integration, and keep AI recommendations advisory.

## Outside sources

- Official HotA feature and design reference: https://h3hota.com/ru/documentation
- Microsoft XAG 102 contrast guidance: https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/102
- Epic UE 5.8 UMG Safe Zones: https://dev.epicgames.com/documentation/unreal-engine/umg-safe-zones-in-unreal-engine
- Epic UE 5.8 packaging/build pipeline: https://dev.epicgames.com/documentation/unreal-engine/packaging-your-project

## Lifecycle

**PASS** for screenshot file integrity and frozen package Gameplay QA. **NOT VERIFIED**: same-source responsive parity between the two screenshot sets, contrast-ratio conformance for text, signed shipping, physical Android touch, genuine LLM inference, full current working-tree Game build. No source camera/combat/UI mutation or Git commit/push/deployment performed in this review.

`∆DΩΛ`: ∆ — image baseline now has explicit capture hashes and anti-stale source caveats; D — inspect actual game frames → deterministic pixel samples on 10 screenshots → replayable script + JSON receipt → manual design review; Ω — 95% for observed artifacts, not for future visual quality; Λ — same-build multi-size capture and contrast audit.
