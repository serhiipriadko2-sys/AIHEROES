param(
    [Parameter(Mandatory = $true)]
    [string]$EditorCompiledSnapshot,
    [Parameter(Mandatory = $true)]
    [string]$AutomationReport,
    [ValidateRange(1, 100000)]
    [int]$MinTests = 188
)
# Build actual AIHeroes.exe from EXACT Editor-tested inputs. Never overwrite an
# earlier evidence directory, terminate another process, or touch SaveGames.
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'aiheroes_build_evidence.ps1')
$Mutex = Enter-AIHeroesNativeWork

try {
    function ResolveOwned([string]$RelativeOrAbsolute) {
        $Path = if ([IO.Path]::IsPathRooted($RelativeOrAbsolute)) {
            $RelativeOrAbsolute
        } else {
            Join-Path $Root $RelativeOrAbsolute
        }
        return (Resolve-Path -LiteralPath $Path -ErrorAction Stop).Path
    }

    $OtherUBT = @(Get-CimInstance Win32_Process -Filter "Name='dotnet.exe' OR Name='UnrealEditor-Cmd.exe'" |
        Where-Object {
            $_.Name -eq 'UnrealEditor-Cmd.exe' -or
            ($_.CommandLine -match 'UnrealBuildTool' -and
             $_.CommandLine -match 'AIHeroes')
        })
    if ($OtherUBT.Count -gt 0) {
        throw "GAME_BUILD_BLOCKED: $($OtherUBT.Count) another native compiler/test process(es) active."
    }
    if (Get-Process -Name UnrealEditor,LiveCodingConsole -ErrorAction SilentlyContinue) {
        throw 'GAME_BUILD_BLOCKED: Unreal Editor/game or Live Coding is running. No process stopped.'
    }

    $EditorPath = ResolveOwned $EditorCompiledSnapshot
    $TestsPath = ResolveOwned $AutomationReport
    $Expected = Get-Content -LiteralPath $EditorPath -Raw -Encoding UTF8 | ConvertFrom-Json
    Assert-AIHeroesCompiledSnapshot -Manifest $Expected -Root $Root
    $Tests = Get-Content -LiteralPath $TestsPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $Entries = @($Tests.tests)
    if ($Entries.Count -eq 0 -or $Entries.Count -lt $MinTests -or $Tests.succeeded -ne $Entries.Count -or
        $Tests.failed -ne 0 -or $Tests.notRun -ne 0 -or
        $Tests.inProcess -gt 0 -or $Tests.succeededWithWarnings -ne 0 -or
        @($Entries | Where-Object { $_.state -ne 'Success' }).Count -ne 0) {
        throw "GAME_BUILD_BLOCKED: editor test report not fully green: tests=$($Entries.Count), succeeded=$($Tests.succeeded)."
    }

    # A green JSON result is not sufficient: require its runner-generated,
    # source-hashed provenance to match the EXACT Editor-compiled snapshot.
    $ProvenancePath = Join-Path (Split-Path -Parent $TestsPath) 'provenance.json'
    $Provenance = Get-Content -LiteralPath $ProvenancePath -Raw -Encoding UTF8 | ConvertFrom-Json
    $ReportHash = (Get-FileHash -LiteralPath $TestsPath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($Provenance.schema -ne 1 -or $Provenance.status -cne 'passed' -or
        $Provenance.build_verified -ne $true -or $Provenance.filter -cne 'AIHeroes' -or
        $Provenance.tests -ne $Entries.Count -or $Provenance.warnings -ne 0 -or
        $Provenance.report_sha256 -cne $ReportHash -or
        $null -eq $Provenance.inputs -or $null -eq $Provenance.binary) {
        throw 'GAME_BUILD_BLOCKED: automation provenance missing, stale or not build-verified.'
    }
    Assert-AIHeroesEntriesEqual -Expected @($Expected.inputs) -Actual @($Provenance.inputs) -Label 'test inputs'
    Assert-AIHeroesEntriesEqual -Expected @($Expected.binary) -Actual @($Provenance.binary) -Label 'tested Editor binary'

    $InputsBefore = Get-AIHeroesInputSnapshot -Root $Root
    $EditorBefore = Get-AIHeroesBinarySnapshot -Root $Root
    $RunId = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
    $Directory = Join-Path $Root ('Docs\BuildRuns\' + $RunId + '_GameWin64')
    New-Item -ItemType Directory -Path $Directory -ErrorAction Stop | Out-Null
    $Dotnet = 'C:\Program Files\dotnet\dotnet.exe'
    $UBT = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
    if (-not (Test-Path -LiteralPath $Dotnet) -or -not (Test-Path -LiteralPath $UBT)) {
        throw 'GAME_BUILD_BLOCKED: toolchain missing.'
    }

    Write-Output "GAME_BUILD_START run=$RunId input_count=$($InputsBefore.Count) tests=$($Entries.Count)"
    & $Dotnet $UBT AIHeroes Win64 Development "-Project=$Root\AIHeroes.uproject" -WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1 2>&1 |
        Tee-Object -FilePath (Join-Path $Directory 'build.log')
    if ($LASTEXITCODE -ne 0) {
        throw "GAME_BUILD_FAIL native_exit=$LASTEXITCODE log=$Directory\build.log"
    }
    $BuildLog = Join-Path $Directory 'build.log'
    if (-not @(Select-String -LiteralPath $BuildLog -SimpleMatch 'Result: Succeeded').Count) {
        throw 'GAME_BUILD_FAIL: UBT result success line missing.'
    }

    $InputsAfter = Get-AIHeroesInputSnapshot -Root $Root
    $EditorAfter = Get-AIHeroesBinarySnapshot -Root $Root
    Assert-AIHeroesEntriesEqual -Expected $InputsBefore -Actual $InputsAfter
    Assert-AIHeroesEntriesEqual -Expected @($EditorBefore) -Actual @($EditorAfter) -Label 'editor DLL'
    Assert-AIHeroesCompiledSnapshot -Manifest $Expected -Root $Root

    $GamePath = Join-Path $Root 'Binaries\Win64\AIHeroes.exe'
    $GameFile = Get-Item -LiteralPath $GamePath -ErrorAction Stop
    if ($GameFile.PSIsContainer -or $GameFile.Length -lt 100000 -or
        ($GameFile.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw 'GAME_BUILD_FAIL: nonexistent, empty or linked executable.'
    }
    $NewestSource = Get-ChildItem -LiteralPath (Join-Path $Root 'Source') -File -Recurse |
        Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
    if ($GameFile.LastWriteTimeUtc -lt $NewestSource.LastWriteTimeUtc) {
        throw "GAME_BUILD_FAIL: exe predates newest source: $($NewestSource.FullName)"
    }
    $Game = [pscustomobject][ordered]@{
        path = 'Binaries/Win64/AIHeroes.exe'
        bytes = [long]$GameFile.Length
        sha256 = (Get-FileHash -LiteralPath $GamePath -Algorithm SHA256).Hash.ToLowerInvariant()
        modified_utc = $GameFile.LastWriteTimeUtc.ToString('o')
    }
    $Receipt = [pscustomobject][ordered]@{
        schema = 1
        status = 'compiled'
        build_verified = $true
        lifecycle = 'local Win64 Development executable only; not packaged, signed or device-tested'
        created_utc = [DateTime]::UtcNow.ToString('o')
        target = 'AIHeroes Win64 Development'
        git_head = (& git -C $Root rev-parse HEAD).Trim()
        scope = 'AIHeroes.uproject and all Source/Config/ bytes match the tested Editor snapshot'
        editor_manifest = $EditorCompiledSnapshot
        editor_manifest_sha256 = (Get-FileHash -LiteralPath $EditorPath -Algorithm SHA256).Hash.ToLowerInvariant()
        editor_binary = $EditorAfter
        automation_report = $AutomationReport
        automation_report_sha256 = $ReportHash
        automation_provenance_sha256 = (Get-FileHash -LiteralPath $ProvenancePath -Algorithm SHA256).Hash.ToLowerInvariant()
        automation_tests = $Entries.Count
        inputs = $InputsAfter
        game_binary = $Game
        build_log_sha256 = (Get-FileHash -LiteralPath $BuildLog -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    $ReceiptPath = Join-Path $Directory 'game_compiled.json'
    [IO.File]::WriteAllText($ReceiptPath,
        ($Receipt | ConvertTo-Json -Depth 9),
        (New-Object Text.UTF8Encoding($false)))
    $ReadBack = Get-Content -LiteralPath $ReceiptPath -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($ReadBack.game_binary.sha256 -cne $Game.sha256 -or
        $ReadBack.automation_tests -ne $Entries.Count -or
        $ReadBack.game_binary.bytes -ne $Game.bytes) {
        throw 'GAME_BUILD_FAIL: receipt readback mismatch.'
    }
    Write-Output "AIHEROES_GAME_BUILD_PASS exe=$GamePath bytes=$($Game.bytes) sha256=$($Game.sha256) tests=$($Entries.Count) receipt=$ReceiptPath"
} finally {
    Exit-AIHeroesNativeWork -Mutex $Mutex
}
