# AIHEROES developer playbook

## Before edits
1. Read root AGENTS.md and verify the pinned .iskra/sot30 corpus.
2. Inspect git status; never overwrite concurrent/user changes.
3. Read Config/DefaultEngine.ini to choose the actual GameMode.
   AIHeroesGameMode is currently active; AIHGameMode is experimental.
4. Read Docs/GAME_DESIGN.md, Docs/ARCHITECTURE.md and
   Docs/ISKRA_AI_EVOLUTION_SPEC.md for intent and platform scope.

## Safety in the game
The player controls expansion, base, recruitment, turns and whether to
attack. Hero AI executes autonomous tactical battles. It may propose
actions, but Unreal enforces legal choices and calculations.
Use typed, bounded state, no access to PC files or arbitrary shell.
Never present fictional 1B/3B/8B upgrade tiers as real weight loading.
Android packaging and PC game interaction are different proof gates.

## Verification
Check no other Unreal-Cmd processes are using shader caches.
Use VS Code Terminal → Run Task → AIHEROES: Build Unreal Editor, then
AIHEROES: Test autonomous battles (4 tests), if available.
Inspect actual results; do not infer PASS from an invocation.

Final: include exact files, byte sizes, hashes, compile/test results,
unverified items and rollback.
