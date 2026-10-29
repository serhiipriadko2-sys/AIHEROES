# AI HEROES вЂ” Three-way zone adviser dossier: integration receipt

Date: 2026-10-29. Local Windows: DESKTOP-ITUAMNF.
Git HEAD observed: cae9f7b. No commit or push by this operator.

## Lifecycle status

PASS: experimental read-only D panel; Win64 Editor compiled snapshot;
216/216 full source-pinned Unreal Automation, original 3D-window
H/Shift+H/D/Esc/Enter/Space QA, five GPU screenshots, and
local AIHeroes.exe Win64 Development compile tied to the same 216 tests.
PARTIAL: the mutable shared working tree may be newer than this
frozen 204-input build. Rebuilding is required if other agents edit
Source or Config after the build. This receipt is a snapshot,
not an automatic claim of parity with later shared-file changes.
NOT CLAIMED: packaged/signed Windows release, Android APK or
physical-device touch QA, cloud deployment, real neural model weights.

## Design and source evidence

- HotA template-format zones/guards/roads: https://h3hota.com/en/template-format
- Official Unreal automation report JSON and CLI:
  https://dev.epicgames.com/documentation/unreal-engine/review-test-results-in-unreal-engine
- No assets, names, factions, code or map data were copied from HotA.
- This slice only changes the opt-in ZoneExpedition adviser,
  mode/HUD integration, its dedicated tests, QA script and docs.
- Legacy fixed five-site campaign, SaveGame classes and prefixes
  are not modified by this slice.

## D. Changes

- D opens an at-most-three-choice ranking from validated PlayerView.
- Each known target shows exact path cost, heuristic score, no-guard
  or guarded-unknown risk and a short economic/resource rationale.
- Unknown node IDs, unseen road costs, hidden enemies and
  uncalibrated victory percentages are never shown.
- D/Esc closes without movement, battle, spending or save.
- Enter/Space, H, R/P, Tab and Save/Load all blocked while open.
- H and Shift+H existing behavior remains unchanged.

## E. Verification

- Editor input entries: 204. Frozen DLL bytes=2589696, SHA256 e36d684c6b446e3f291698076164e3e93eff6b2a83488c900770cfc0ac362383.
- Targeted dossier tests: 4/4 PASS.
- Whole suite: 216/216 PASS; warnings=0 failed=0 notRun=0; build_verified=true.
- Full test report SHA256 ea263bea192cd0910908be3cab0b8579362bbd67c4d811425838aca243356886.
- Win64 Development executable compiled: bytes=333519872 SHA256 ea16e4582b602be79a2a1fd31821a1421f70b5d4fa886277543c4acd56b7ed3c.
- Win64 executable is NOT a cooked or signed distribution.
- Two separate real 3D window runs completed; the second used
  the final Editor-compiled DLL, and is listed in the manifest.
- D opened three choices, ignored Enter and Space while open,
  then closed with Esc. The user then confirmed two independent
  battles via Enter, with one explicit human next-day command.
- Windows QA took five native GPU frames at 1280x720; each copied
  image was byte- and hash-checked against its own engine QA receipt.
- Original player SaveGame filenames and hashes did not change.
- QA-owned game windows closed gracefully after log/screenshot readback.
- PowerShell QA syntax and generic credential-pattern scan passed.

## Concrete verified file manifest

| Project-relative path | Bytes | SHA256 |
|---|---:|---|
| Source/AIHeroes/Public/AIHeroesZoneExpeditionAdvisor.h | 2684 | b104f01dab2d090e88eaf830ee4d30710dfad1dbfe2cee541d7214830069019b |
| Source/AIHeroes/Private/AIHeroesZoneExpeditionAdvisor.cpp | 12588 | f4d37fb6e279b471bc12a155a946d74adcb4a3dc4968937e355bc8cfb08cb469 |
| Source/AIHeroes/Public/AIHeroesZoneExpeditionGameMode.h | 2603 | bf96252dd4a54de95d4fe5ec8758ef1ee5af12ed69d4a4107b4c3437f0c132f7 |
| Source/AIHeroes/Private/AIHeroesZoneExpeditionGameMode.cpp | 20822 | 47dd4cd14aefc3de60ebd13ebd83f67edbf8ec06d4cf394016a9b832a9d06f7e |
| Source/AIHeroes/Private/AIHeroesZoneExpeditionHUD.cpp | 4066 | d44daa159f9ee140055bc8b0700a3c983e4cef4adfde985453462cb1ffd10c02 |
| Source/AIHeroes/Private/Tests/AIHeroesZoneExpeditionDossierTests.cpp | 9291 | 63d8666d832f0d38a366d38492f251d77e23000d90cbe959a69c2e4c46377216 |
| Tools/qa_zone_expedition_dossier.ps1 | 11477 | 03b28abc2d2034a949cb5e89e089e28f36fd3794499b99ee6dab5192d69ce96e |
| Docs/ZONE_EXPEDITION_DOSSIER_V1.md | 7870 | f5550454e3112ab3aa610411f1d9f0b2a3b251b89de2a536119d30131635a503 |
| Docs/BuildRuns/20261029T141711156Z/compiled.json | 56320 | ee94083f86c611f6c680b60270ec64f8e6474dd8afbadaa7cf857b0b0c1124df |
| Docs/AutomationRuns/20261029T141959618Z/index.json | 85709 | ea263bea192cd0910908be3cab0b8579362bbd67c4d811425838aca243356886 |
| Docs/AutomationRuns/20261029T141959618Z/provenance.json | 56440 | 5ecd5020b81056c6d3e78724cdf57d245b4027bbb1c8830a28830c4b8d7bdc8d |
| Docs/BuildRuns/20261029T142136983Z_GameWin64/game_compiled.json | 57489 | 269dc360de5d14323f1fb1dd3a690c2d15fdde47b10c051e545d8044ad414028 |
| Docs/VisualRuns/ZoneExpeditionDossier_Verified_20261029T142547483Z/initial.png | 370897 | e4c56c1ff159c95fb97b41cec6e038add6508db09f3ad2103a50922afdd7ab70 |
| Docs/VisualRuns/ZoneExpeditionDossier_Verified_20261029T142547483Z/dossier.png | 222821 | c4f9b35710feec800de1b330584a823421c6bbfa7cd726ae03477449f127111d |
| Docs/VisualRuns/ZoneExpeditionDossier_Verified_20261029T142547483Z/forest.png | 403022 | 6b4c7c752e1bcb91279618adb7d7faee15e10b532d00363393e3de1c98579af7 |
| Docs/VisualRuns/ZoneExpeditionDossier_Verified_20261029T142547483Z/day2.png | 397594 | c94e0a86ee3e4e6fc90fd6c5914a88890cf5296e381c92063ae4a4925661a682 |
| Docs/VisualRuns/ZoneExpeditionDossier_Verified_20261029T142547483Z/ore.png | 404817 | 2fc499173c7047d63586a9c936f4b1bfb9e9fa2365032ca994631b1f8cf36261 |
| Docs/VisualRuns/ZoneExpeditionDossier_Verified_20261029T142547483Z/receipt.json | 4159 | 1ba84ce3efab355bfaeb98e1350b23672f0417db5789a6dbd2dfbfe6bc5a606b |
| Saved/QA/ZoneDossierVerified_20261029T141711156Z/UnrealEditor-AIHeroes.dll | 2589696 | e36d684c6b446e3f291698076164e3e93eff6b2a83488c900770cfc0ac362383 |
| Binaries/Win64/AIHeroes.exe | 333519872 | ea16e4582b602be79a2a1fd31821a1421f70b5d4fa886277543c4acd56b7ed3c |
| Docs/AutomationRuns/20261029T140903700Z/index.json | 2366 | 70ff8c410d32b9e39e8dc008247634976299536a12a49d33f63b1d5a7c1c9e9c |

## Snapshot boundary and rollback

Earlier UBT successfully compiled one intermediate state, but
a source-drift gate rejected its provenance while other agents
were adding artwork. A later build+full suite+Win64 Game compile
passed under the frozen 204-input manifest. Any later CameraPawn
or other source change must NOT be treated as covered by these tests.

Rollback only the five advisor/GameMode/HUD files listed here;
their unchanged copies are under Saved/QA/ZoneDossierBefore_20261029.
Remove the dedicated dossier test/script/docs after dependency check.
Do not reset the separate ArtDirection or Explorer code contributed
by other agents. No user SaveGame migrations were performed.

Delta D Omega Lambda: delta transparent three-way offline advisor;
D read-only known graph => 216 regression tests => window key QA =>
SHA evidence; Omega at most 95 percent for the frozen local build;
Lambda revisit after subsequent source changes, human UX, mobile QA.
