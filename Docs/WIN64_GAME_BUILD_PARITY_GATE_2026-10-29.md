# AIHEROES — Windows Game Build Parity Gate (2026-10-29)

Status: **TOOLS IMPLEMENTED; BLOCKING PATH TESTED; GAME BUILD PENDING**. Do not equate an Editor DLL, source commit, or a green automation JSON with a current usable Windows Game binary.

## Why this gate exists

On 2026-10-29 the current `AIHeroes.exe` was still compiled 2026-10-28 21:22:48 UTC, while the newer Win64 Editor DLL, model reply logic, tactical camera and the 188/188 and subsequent 197/197 automation suites had changed. Unreal's own build pipeline treats compiling code, cooking data, staging assets and packaging for players as distinct operations. Thus a successful Win64 Development build does **not** mean cooked or released gameplay; see https://dev.epicgames.com/documentation/unreal-engine/packaging-your-project .

The official HotA docs at https://h3hota.com/en/documentation confirm owned field dwellings accumulate creatures weekly; https://h3hota.com/en/templates distinguishes roads and guards from zone contents. AIHEROES's human-authorized convoy is an **original** mechanic; no assets or AI/model implementations are copied.

## New guarded command

`Tools/build_aiheroes_game.ps1` (PowerShell 5 compatible) takes:

- `-EditorCompiledSnapshot <Docs/BuildRuns/.../compiled.json>`: original `Tools/build_aiheroes_editor.ps1` manifest, created only after the Editor UBT reports success and source hashes remain unchanged.
- `-AutomationReport <Docs/AutomationRuns/.../index.json>`: a complete Unreal Automation report. The directory must contain a sibling `provenance.json` created by `Tools/run_aiheroes_automation.ps1`.
- `-MinTests 188` (positive-only, can increase for a growing suite): the number of successful tests must be at least this count. One failed, omitted or warned test invalidates readiness.

Before UBT work, the gate requires:
1. No competing UBT, Editor-Cmd, Unreal Editor or Live Coding process. It never terminates other processes.
2. Exact current `AIHeroes.uproject`, complete `Source/`, `Config/` and Editor DLL **SHA-256 parity** with the provided compiled manifest. New files, deleted files, changed bytes or different DLL fail closed.
3. An independent fresh test report that is wholly successful and nonempty, with all tests reporting `Success`.
4. The runner-generated `provenance.json` must be `status=passed, build_verified=true, filter=AIHeroes`. Its input-file inventory, Editor DLL bytes and hash must match the compiled manifest **and** its report hash must match the actual full report. Merely copying a successful JSON from another build is not enough.

Then the script builds `AIHeroes Win64 Development` through the actual UE5.8 UnrealBuildTool with `-NoUBA -MaxParallelActions=1 -NoHotReload -WaitMutex`; checks both native exit code and `Result: Succeeded`. After build it verifies unchanged source inputs/Editor DLL, executable existence/size/SHA-256, and that the executable is not older than the newest source. A unique new `Docs/BuildRuns/<UTC>_GameWin64/build.log` and `game_compiled.json` are created **only after the required postconditions**. The read-back receipt includes Game exe hash, Editor DLL hash, frozen source list, successful automation report hash and provenance hash.

### Observed defensive tests

- New tool parsed with **0 PowerShell syntax errors**.
- Valid older 188/188 automation and original Editor compiled manifest have equal source inventory for **185 files** and equal Editor DLL SHA-256; the runner-generated test `provenance.json` reports the same full report hash.
- With another UBT active, the gate returned nonzero `GAME_BUILD_BLOCKED` and an exact before/after SHA-256 comparison showed **no change** to `Binaries/Win64/AIHeroes.exe`.
- After other agents added the ModelReply feature, invoking the gate with the old 188-test snapshot returned **nonzero** `AIHEROES_PROVENANCE_MISMATCH inputs count changed`, before any build directory or new binary was written; Game exe SHA unchanged.
- Fresh 197/197 Automation for the separately developed new ModelReply snapshot was independently inspected: `Saved/QA/MapReadability_20261029T125050007Z/report/index.json`, SHA-256 `91fd10ac944733337018eea458bee0cdbd24bc0070f9903a05a7f5d2e29d89d7`. Its test report **does not substitute** for the required same-snapshot `Docs/AutomationRuns` runner provenance until that linkage is established.

### Explicit memory and authority boundaries

A concurrently running C++ Editor build failed with `C3859`/`C1076` and Windows error **1455**, in an environment with about 32GB physical RAM and a 16GB pagefile. Microsoft documents that for PCHs not using `#pragma hdrstop` these are commonly virtual-memory-pressure failures, and recommends reducing simultaneous compilers rather than blindly adding `/Zm`. See https://learn.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/fatal-error-c1076 and https://learn.microsoft.com/en-us/cpp/build/reference/zm-specify-precompiled-header-memory-allocation-limit .

This run **did not** adjust OS pagefile, compiler policy, system-wide toolchain, other agent processes, SaveGames, project Knowledge, branch pointers, commits or deployed packages. Any virtual-memory policy change needs separate authorization.

## Next acceptance sequence

After the active source changes settle and all other UBT processes are absent:

1. Run `Tools/build_aiheroes_editor.ps1` on the new complete source and note its **new unique** compiled manifest path.
2. Run `Tools/run_aiheroes_automation.ps1 -Filter AIHeroes -CompiledSnapshot <new manifest path>`; demand all green and runner `provenance.json`.
3. Run the new Game gate using those **matching** paths and `-MinTests` equal to the current expected test count.
4. Run a dedicated Game executable GUI smoke, compare player SaveGame inventory before/after and create screenshot/log receipts.
5. Only then consider Game Development **compiled + tested**; cooking, staging, packaging, shipping and physical Android device validation are independent later gates.

**Outcome as of the observed QA:** editor 188/188 PASS with strong compiled provenance; a later 197/197 separate source snapshot PASS; the Game build gate itself is present and tested for fail-closed behavior, but a new Game exe for the latest uncommitted source is **NOT VERIFIED**.

`∆DΩΛ`: ∆ — Game binary build pipeline now demands same-source test provenance; D — Epic/Microsoft official docs → Windows source inspection → test manifest comparison → nonzero negative gates and unchanged old exe; Ω — 95% for observed guard failures and frozen Editor test, lower for current unreleased Game; Λ — recompute after latest Source/Config/Editor hash is frozen and the new Game build succeeds.
