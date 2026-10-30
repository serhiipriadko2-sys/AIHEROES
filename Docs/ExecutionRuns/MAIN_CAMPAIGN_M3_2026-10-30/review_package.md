# M3 final fresh read-only review
Plan: Docs/MAIN_CAMPAIGN_VERTICAL_SLICE_2026-10-29.md ? M3 and all five Review Focus classes.
Spec: Docs/GAME_DESIGN.md, Docs/ARCHITECTURE.md. Ledger: Docs/MAIN_CAMPAIGN_EXECUTION_LEDGER_2026-10-29.md ? M3 rulings.
Review the WHOLE integrated M3 candidate, not merely root-authored hunks:
- integration_changes.patch + integration_scope.json (relative to HEAD 1ff88bb)
- own_changes.patch + own_changes.json (only our post-pause integration relative to handoff snapshot)
- PARALLEL_HANDOFF.md explains external authorship; do not delete or overwrite it.

Current immutable build: Docs/BuildRuns/20261030T133221466Z/compiled.json (255 inputs).
Full native group: Docs/AutomationRuns/20261030T133531511Z/{index,provenance}.json (291 successes, zero warnings/failures).
True regression RED: Docs/AutomationRuns/20261030T132137682Z/index.json (corrupt trace and same-metadata borrowed receipt; positive controls passed).
First 131839045Z was an invalid test-fixture run, NOT RED. Static verify_hud_cache.py failed before the HUD binding fix and now passes; not a performance benchmark.

Check actual already-computed reward plan vs UI deltas; full record binding, legacy/corrupt/draw behavior; duplicate group identity and wounded ceil counts; caches vs replay-each-frame; candidate preparation before irreversible commit; exactly-once/lifecycle reset on start/abort/return/load; no schema, balance, V1/V2 or permission mutation. Review surrounding code and explicit boundary assumptions, not just reports. Native ruins transaction test is a declared weak-guard fixture, not campaign balance proof.
No writes, builds, tests, processes, UI or subdelegation. Return Critical/Important/Minor findings with file:line and concrete trigger; explicitly none and list declined/unverified areas. Keep concise.
