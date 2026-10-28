# AI HEROES — local build and verification receipt

Date: 2026-10-28
Surface: authorized Windows PC, C:\github\AIHEROES
Status: LOCAL_BUILD_PASS / AUTOMATION_PASS / GAMEPLAY_PARTIAL / ANDROID_BLOCKED
Repository: main has no commits yet; nothing has been pushed to GitHub by this work.

## Evidence

- Unreal Engine 5.8, target AIHeroesEditor Win64 Development.
- UnrealBuildTool output: Result: Succeeded; UnrealEditor-AIHeroes.dll linked.
- Engine automation log: 4 tests completed successfully:
  AIHeroes.Core.DeterministicBattle,
  AIHeroes.Core.NoArmyCannotWin,
  AIHeroes.Core.ProtocolChangesTactics,
  AIHeroes.Simulation.CombatPolicy.
- Runtime smoke log: "Game class is 'AIHeroesGameMode'" and
  "[AI HEROES] Welcome to AI HEROES..." from BeginPlay.
  This proves the active GameMode starts; it does NOT prove user-driven capture
  and recruitment work correctly in a visual session.
- .vscode/tasks.json parsed as JSON and contains three VS Code tasks.
- Config/DefaultEngine.ini: populated AndroidFileServer SecurityToken removed.
  Local rollback copy under Saved/LocalSecrets (Saved is Git-ignored).
- Unreal SDK validation: Win64 VALID; Android INVALID r27c.

## Important integration boundary

The Source/AIHeroes directory currently contains TWO experimental class families:
- AAIHeroesGameMode + Public/Private/AIHeroesBattleEngine (ACTIVE by config);
  multi-round deterministic combat with initiative and target selection.
- AAIHGameMode + AIHGameHUD + AIHPlayerController + AIHSimulation (ALTERNATIVE);
  strategic movement-point map and prototype mouse/touch HUD.
Both compiled, but the alternative is not active in the GameDefaultMap setting.
Their gameplay features have NOT been unified. Treat claims of a combined,
touch-enabled, multi-round playable game as UNVERIFIED until integration and QA.

## Verified artifact bytes and SHA256

| Relative path | Bytes | SHA256 |
|---|---:|---|
| .vscode/tasks.json | 2302 | 5873ea1867d30f086791bbac96313dcfb5effda5bcc897348fa88f6ac6d045e6 |
| Docs/AI_EVOLUTION_DESIGN.md | 11015 | d2d5e299a6ee1cf80246e69d36dfdd2eb756427b2beebe1c5d1a2e0f51850b4d |
| Source/AIHeroes/AIHGameMode.h | 1743 | 0acc3a0c2aadd382588773aa5b008cbd163f549a8a1c4d6596f1499dd09a95fb |
| Source/AIHeroes/AIHGameMode.cpp | 12414 | c98ec2e4a7f09a753bc5581c38996d6444b0f7b4e5e19b355ca437d6c1941625 |
| Source/AIHeroes/AIHGameHUD.cpp | 3278 | 0c2f9d6624ddf4789a69aee63cd39a66b024092d006aeb9c56aedef60fd6aa4b |
| Source/AIHeroes/AIHPlayerController.cpp | 1795 | fe5ba9198845225b4ccb6d2fca80ab559677bef6284df4bbf0018521737b3b07 |
| Config/DefaultEngine.ini | 698 | 23b86ae2e17e2f651c2084cea6efe8e4c56a9bfe6d93e93c63b71a9105de6701 |

## Acceptance and non-claims

PASS: C++ build; four deterministic tests; active GameMode starts; VS Code tasks.
PARTIAL: interactive 3D capture and HUD not personally verified with Play-in-Editor.
BLOCKED: Android native build due SDK; no Android device test.
NOT IMPLEMENTED: actual loaded 1B/3B/8B checkpoints, runtime training,
save/load, multiplayer, full world art, 3D animated tactical battles.
NOT DONE: branch commit, GitHub push, release, package, production deployment.

Next reversible action: use VS Code "AI HEROES: Launch PC prototype"
and manually validate five turns, one capture and one automatic battle.
Then consolidate both class families and test on Android hardware.
