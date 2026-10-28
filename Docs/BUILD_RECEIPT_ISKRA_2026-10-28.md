# AIHEROES — local verification receipt — 2026-10-28

## Confirmed environment
- Surface: authorized Windows device DESKTOP-ITUAMNF; working tree C:\github\AIHEROES
- Unreal Engine 5.8.4 (editor reports engine version 5.8.4)
- Blender 5.2 and VS Code available
- Repository initially had no commits; changes remain local, uncommitted, unpushed.

## Build
- Command: UnrealBuildTool AIHeroesEditor Win64 Development -Project=C:\github\AIHEROES\AIHeroes.uproject
- Result: SUCCEEDED; 22/22 actions completed, UnrealEditor-AIHeroes.dll linked.
- File: C:\github\AIHEROES\Binaries\Win64\UnrealEditor-AIHeroes.dll
- Bytes: 349184
- SHA256: F8C6AEBF78CB8ED4D0781EF3D7035CEFA159BC1063E84B080E1E466A57D9853B

## Automated verification
- AIHeroes.Core.DeterministicBattle — PASS
- AIHeroes.Core.NoArmyCannotWin — PASS
- AIHeroes.Core.ProtocolChangesTactics — PASS
- AIHeroes.Simulation.CombatPolicy — PASS
- UnrealEditor-Cmd test run: 4 performed, all Success, exit code 0.

## Runtime evidence
- A game process was independently observed running (not launched by this receipt).
- Log: "Game class is 'AIHeroesGameMode'".
- Log: "Welcome to AI HEROES. Recruit with 1 / 2, then click a red node to attack."
- Active mode (Config/DefaultEngine.ini): /Script/AIHeroes.AIHeroesGameMode.
- Rendered visual appearance and physical mouse/touch interactions NOT directly verified.

## Project and design artifact
- C:\github\AIHEROES\AIHeroes.uproject — 318 bytes
- SHA256: 16E606F3DD237EF5E686401A72B2E29766C7F45048309E9B74C86630E98BECEE
- C:\github\AIHEROES\Docs\ISKRA_AI_EVOLUTION_SPEC.md — 7501 bytes
- SHA256: A4313C6EDF3B46472FF273EB42C067626C392C9D6F7E425E1CC9F3578BF35432
- Source inventory at verification: 31 files / 66206 bytes.
- Credential-pattern scan of Source, Docs and Config: zero matches for the targeted private-key/token patterns.

## Important boundaries
- Two GameMode/AI prototype implementations coexist; the active one is AIHeroesGameMode.
- Other work was observed being added in the same folder concurrently; no claims of sole authorship.
- No Android APK packaged or tested.
- No actual LLM checkpoint (1B, 3B or 8B) installed or tested in the game.
- No game-playthrough QA, UI screenshots, mobile performance or persistence tests.
- No Git commit, push, PR, deployment or store release.
- LOCAL_BUILD_PASS + TESTS_PASS does not equal RELEASE_READY.

## Safe next integration
1. Preserve both implementations, select ONE authoritative GameMode, document why.
2. Run manual PC interaction: select node -> recruit -> attack -> capture -> income -> protocol.
3. Separate real LLM adapter from deterministic game rules, test failure fallback.
4. Set up Android toolchain and touch/power budgets before packaging.

Status: PARTIAL (working native PC scaffold verified, full game not accepted).
