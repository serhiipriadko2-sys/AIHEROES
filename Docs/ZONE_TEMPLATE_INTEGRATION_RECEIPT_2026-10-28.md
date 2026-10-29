# AI HEROES — Procedural zone template v1: development receipt

Date: 2026-10-28
Machine: DESKTOP-ITUAMNF / C:\github\AIHEROES / Unreal Engine 5.8
Git HEAD at readback: cc062ba (parallel authors may advance it).
Lifecycle state:
  SOURCE_CREATED, NATIVE_CXX17_BUILT, NATIVE_CXX17_TESTED,
  UNREAL_EDITOR_BUILT, UNREAL_AUTOMATION_TESTED, ARTIFACT_QC_VERIFIED
  ACTIVE_CAMPAIGN_WIRED=NO, LIVE_3D_MAP_SPAWN=NO,
  SAVE_MIGRATION=NO, ANDROID_TEST=NO, GIT_PUSH_BY_THIS_OPERATOR=NO.

## A. External research, with original design boundary

HotA's official Random Map Template Editor states that random maps
use templates to define the zone structure, legal zone-to-zone links,
guarded standard crossings, unguarded wide crossings and road policies.
The new code uses this abstract design principle, not HotA's code,
graph templates, art, factions, towns, monsters, music, or map files.
Sources observed on 2026-10-28:
https://h3hota.com/en/template-format
https://h3hota.com/en/templates

Epic Unreal Engine 5.8 PCG documentation describes points/attributes,
graphs, level/runtime generation, and editor previews; these do not
themselves establish a valid route to each game objective. An
engine-independent graph-validation layer is therefore required first.
https://dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-overview

UE supports SaveGame/custom serialization versioning. Changing the
fixed five-node v1 campaign without a migration would invalidate or
silently misinterpret existing player data; this change deliberately
does not do so.
https://dev.epicgames.com/documentation/en-us/unreal-engine/versioning-of-assets-and-packages-in-unreal-engine

## B. Original implementation and invariants

- Source/AIHeroes/Public/AIHeroesZoneTemplateCore.h:
  portable C++17 SplitMix32 seed -> 7..11 zone graphs, with two starting
  guarded objectives, checkpoint and two late guarded missions, and
  0..4 optional alternate routes.
- Source/AIHeroes/Public/AIHeroesZoneTemplate.h:
  versioned typed read-only Unreal representation (schema=1 is the
  NEW DRAFT schema, not a replacement for player-save schema).
- Source/AIHeroes/Private/AIHeroesZoneTemplate.cpp:
  engine wrapper that calls the EXACT portable production kernel and
  preserves node identity, integer map cells, danger, roads, guards
  and revision.
- Source/AIHeroes/Private/Tests/AIHeroesZoneTemplateTests.cpp:
  five Unreal Automation suites, including 640 adapter candidate maps.
- Tests/zone_template_native.cpp:
  the real portable core compiled and exercised by MSVC.
- Tools/verify_zone_template_native.ps1:
  isolated MSVC /std:c++17 /EHsc /W4 /WX build and read-back QA.
- Docs/ZONE_TEMPLATE_V1.md:
  architecture, stage gates, sources and explicit nonclaims.

A candidate's stable 32-bit revision is a CHANGE DETECTOR, not a
cryptographic security key. The existing portable Dijkstra path
implementation is reused; no second path algorithm was invented.
The validator rejects forged shortcut roads, duplicate edges/names,
missing reverse links, changed weights, collisions, invalid roles,
guard bypasses, resource progression bypass and missing essential
connections. Graphs contain NO game actor references, file paths,
network requests or gameplay-authorizing methods.

Full-world bDiscovered=true is intentionally a VALIDATION ORACLE.
A future fog-of-war adapter must not display undiscovered routes/costs
to a human player or an AI. The module does not invoke the current
GameMode, change a hero's position or spend player resources.

## C. Native verification

Script:
  powershell -NoProfile -File Tools\verify_zone_template_native.ps1

Printed receipt:
  ZONE_TEMPLATE_NATIVE_PASS seeds=1024 variants=5
  worlds=5120 checks=51223 schema=1
  exit code 0

Native output folder:
  Saved\QA\ZoneTemplateNative_20261028T204642110Z
  compile.log, tests.log, executable, receipt.json

Receipt copied with matching SHA256 to:
  Docs/ZONE_TEMPLATE_NATIVE_RECEIPT_2026-10-28.json
  status=PASS_NATIVE_CXX17_GENERATOR,
  generated_worlds=5120, assertions=51223,
  player_save_writes=0.

This is evidence about the actual portable C++17 generator, not
a substitute algorithm written in Python or documentation.

## D. Unreal build and automation

Shared Unreal editor build with newly written files was compiled on
Windows, and UnrealBuildTool reported "Result: Succeeded" in
C:\Users\gabra\AppData\Local\UnrealBuildTool\
Log-backup-2026.10.28-20.49.26.txt

Full automation was repeated after a concurrent rebuild. Immediately
before and after that second run, the editor DLL had the IDENTICAL SHA256:
 4bb15acbc4d7b36fdb0004b11f8513e1f7f83143b6c00ce6e08124a4412e449b
The script returned AIHEROES_AUTOMATION_PASS and exit code 0.
A byte-identical tested DLL was copied into Saved/QA under a unique
immutable snapshot path, so later concurrent builds cannot alter it.

Fresh full automation JSON:
 Docs/AutomationRuns/20261028T205454103Z/index.json
  total=117, succeeded=117, failed=0,
  warnings=0, notRun=0, inProcess=0

Five individual new test cases were Success:
 AIHeroes.ZoneTemplate.RepeatableSeedAndRoles
 AIHeroes.ZoneTemplate.GuardedProgressiveReachability
 AIHeroes.ZoneTemplate.TamperedGraphFailsClosed
 AIHeroes.ZoneTemplate.SeedSweepAndBounds
 AIHeroes.ZoneTemplate.V1SaveNotSilentlyExpanded

The last test shows the old five-node player-save schema REJECTS an
attempt to pack 10 new zones inside it. This is intentional; real
procedural gameplay must get a versioned save migration, with rollback
and tests, rather than writing into a human's old slots.
No player saves were created, modified, migrated or deleted by this
module. The DLL and JSON hashes are recorded below.

## E. Source-of-truth and integrity

SOT30 read-back:
 SOT30_INTEGRITY_PASS v5.5.7,
 30/30 Knowledge, 32/32 SHA256SUMS, members=33/33,
 pinned manifest 0decd118a6300324b757d2587ec315a569aaf5800299d2b33bf046feaf56aeff.
git diff --check: exit 0 at read-back; note that it does not
independently inspect new untracked files, which have SHA256 checks below.
Multiple agents concurrently modify other files in this repository.
This operator did not reset their work, create a Git commit, push,
merge, change global PC settings or close any user editor.
There is no claim that the shared tree has no other active changes.

## F. File manifest frozen after build/test

| Project-relative file | Bytes | SHA256 |
|---|---:|---|
| Source/AIHeroes/Public/AIHeroesZoneTemplateCore.h | 11414 | 4d2672532112f318fb656824bf2f3d4ac93a4a96796e71fac09f59128886ecbf |
| Source/AIHeroes/Public/AIHeroesZoneTemplate.h | 1401 | 53b27b8dd2509e0a7bc5ada69aa7a9a001266c060202fc718f7819dea00413bc |
| Source/AIHeroes/Private/AIHeroesZoneTemplate.cpp | 4163 | a9c6809c0e64f084bb65844b1c2e6e066ee37b1c046e70cb2d88078b41b53d8b |
| Source/AIHeroes/Private/Tests/AIHeroesZoneTemplateTests.cpp | 9452 | 769a593b830fbca24696f846851e3b1776821cbe2a0090ca914c6f7088c4ccfe |
| Tests/zone_template_native.cpp | 5864 | bb690f1753a291db757a91888148b2a3a2fac4c3cbad2440f79086f634cde898 |
| Tools/verify_zone_template_native.ps1 | 2694 | 9f45de4af7a6243c156fff07a668d834d33d6b746f921460736659438f814e8a |
| Docs/ZONE_TEMPLATE_V1.md | 6513 | ea82a842a55c6f6761e9afb27bcb731d7423235b621b460bc047fed6371a119e |
| Docs/ZONE_TEMPLATE_NATIVE_RECEIPT_2026-10-28.json | 1240 | 3f3d0f3bf678332062a263afe51765a99ab2fd9b7397f7321ad1faddadde34c1 |
| Saved/QA/ZoneTemplateVerified_20261028T205454103Z/UnrealEditor-AIHeroes.dll | 1683968 | 4bb15acbc4d7b36fdb0004b11f8513e1f7f83143b6c00ce6e08124a4412e449b |
| Docs/AutomationRuns/20261028T205454103Z/index.json | 46181 | b3dc3b09229856a47e4cf4bad157e22626cbfefbc9ee68b76b3449238408ce17 |

The manifest proves specific file bytes in the owner's Windows
workspace at readback. The tested DLL is a snapshot under Saved/QA;
the SHARED Binaries/Win64 DLL may change during concurrent development. It does NOT prove persistent cloud sync,
a Git commit, an installed mobile build, or full playable campaign
migration. The final receipt's own bytes and checksum are verified
separately after writing.

## G. Next gate (not yet performed)

1. Design a map schema v2 with stable zone IDs and per-seed metadata;
   create migration from all v1 campaign slots in a test-only namespace.
2. Extend the generator with resource budgets and balanced guard
   strengths before making it eligible for official campaign play.
3. Build PCG-based 3D zone/road previews and discovery UI, with an
   explicit opt-in playtest map separate from real saves.
4. Integrate authoritative movement and fight confirmation using
   the same AdventurePath graph and revision checks.
5. Run manual gameplay from seed -> explore -> fight -> capture ->
   save -> load -> replay across seeds and then Android toolchain QA.

Progress metrics for generated worlds and tests are supported by the
actual test outputs. Metrics for rendered terrain FPS, mobile battery,
real model inference or player retention: computation unavailable.

PASS: authored native generator and Unreal adapter, 117/117
automation, 5120 candidate maps, source/artifact checks.
PARTIAL: active procedural campaign, 3D PCG rendering and save
migration are not implemented; do not present them as completed.

DeltaD Omega Lambda:
Delta: seed-driven topology plus independent guarded-route proofs.
D: read existing path/save contracts -> official HotA/Epic docs ->
   portable generator -> native fuzz -> UE adapter -> full tests ->
   receipts and SHA256.
Omega: 95% for verified local build and deterministic invariants.
Lambda: reassess after migration, in-game 3D generation and player QA.
