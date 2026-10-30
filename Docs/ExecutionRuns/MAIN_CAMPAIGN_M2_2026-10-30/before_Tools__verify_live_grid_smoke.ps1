param(
    [ValidateSet('Victory', 'Defeat', 'TwoGroups')]
    [string]$Scenario = 'Victory',
    [string]$CompiledSnapshot
)

$ErrorActionPreference = 'Stop'
# PowerShell ValidateSet accepts casing aliases; use the C++ selector's exact
# spelling so `-Scenario defeat` and the canonical VS Code task behave alike.
$Scenario = switch ($Scenario.ToLowerInvariant()) {
    'defeat' { 'Defeat' }
    'twogroups' { 'TwoGroups' }
    default { 'Victory' }
}
$Root = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($CompiledSnapshot)) {
    $CompiledSnapshot = Join-Path $Root 'Docs\AIHEROES_BUILD_LATEST.json'
}
. (Join-Path $PSScriptRoot 'aiheroes_build_evidence.ps1')

function Get-SmokeSaveHashes {
    $Path = Join-Path $Root 'Saved\SaveGames'
    if (-not (Test-Path -LiteralPath $Path)) { return }
    Get-ChildItem -LiteralPath $Path -File -Filter '*.sav' | Sort-Object Name |
        ForEach-Object {
            [pscustomobject]@{
                path = $_.Name
                bytes = [long]$_.Length
                sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
            }
        }
}

$Mutex = Enter-AIHeroesNativeWork
try {
    if (Get-Process -Name UnrealEditor,UnrealEditor-Cmd,LiveCodingConsole -ErrorAction SilentlyContinue) {
        throw 'Another Unreal process is running. No process was closed and no smoke was started.'
    }
    $Manifest = Get-Content -LiteralPath $CompiledSnapshot -Raw -Encoding UTF8 | ConvertFrom-Json
    $ManifestHash = (Get-FileHash -LiteralPath $CompiledSnapshot -Algorithm SHA256).Hash.ToLowerInvariant()
    Assert-AIHeroesCompiledSnapshot -Manifest $Manifest -Root $Root
    $App = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
    if (-not (Test-Path -LiteralPath $App)) { throw 'Unreal Editor executable is unavailable.' }
    $Id = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
    $Directory = Join-Path $Root "Docs\VisualRuns\${Id}_$Scenario"
    New-Item -ItemType Directory -Path (Join-Path $Directory 'previous') -Force | Out-Null
    $Prefix = switch ($Scenario) {
        'Defeat' { 'LIVE_GRID_V2_DEFEAT' }
        'TwoGroups' { 'LIVE_GRID_V2_TWO_GROUPS' }
        default { 'LIVE_GRID_V2' }
    }
    $Names = @('START', 'PAUSED', '2X', 'RESULT', 'RETURN', 'DOSSIER') |
        ForEach-Object { "${Prefix}_$_.png" }
    foreach ($Name in $Names) {
        $Path = Join-Path $Root "Docs\$Name"
        if (Test-Path -LiteralPath $Path) {
            Copy-Item -LiteralPath $Path -Destination (Join-Path $Directory "previous\$Name")
        }
    }
    $SavesBefore = @(Get-SmokeSaveHashes)
    $Started = [DateTime]::UtcNow
    $Arguments = @(
        ('"' + (Join-Path $Root 'AIHeroes.uproject') + '"'),
        '-game', '-windowed', '-ResX=1280', '-ResY=720', '-NoSplash', '-NoP4', '-NoSound',
        '-AIHeroesLiveSmoke', "-AIHeroesLiveSmokeScenario=$Scenario",
        ('-abslog="' + (Join-Path $Directory 'smoke.log') + '"')
    )
    # The user requested visible PC acceptance. This is our own disposable game,
    # not an Editor session. The harness exits itself; no process is terminated.
    # -Wait is deliberate: a prior observer saw null ExitCode with WaitForExit().
    $Process = Start-Process -FilePath $App -ArgumentList $Arguments -WorkingDirectory $Root `
        -WindowStyle Normal -Wait -PassThru `
        -RedirectStandardOutput (Join-Path $Directory 'stdout.log') `
        -RedirectStandardError (Join-Path $Directory 'stderr.log')
    $NativeExit = $Process.ExitCode
    if ($null -eq $NativeExit -or $NativeExit -ne 0) {
        throw "Graphic smoke exit=$NativeExit. Evidence directory: $Directory"
    }
    $Log = Get-Content -LiteralPath (Join-Path $Directory 'smoke.log') -Raw
    $ExpectedPass = '\[LIVE GRID SMOKE\] PASS scenario=' + [regex]::Escape($Scenario) + '\s'
    if ($Log -notmatch $ExpectedPass -or $Log -match '\[LIVE GRID SMOKE\] FAIL') {
        throw "Graphic smoke PASS absent or FAIL present: $Directory"
    }
    Assert-AIHeroesCompiledSnapshot -Manifest $Manifest -Root $Root
    if ((Get-FileHash -LiteralPath $CompiledSnapshot -Algorithm SHA256).Hash.ToLowerInvariant() -cne $ManifestHash) {
        throw 'Compiled manifest changed during the smoke.'
    }
    $SavesAfter = @(Get-SmokeSaveHashes)
    if ($SavesBefore.Count -ne $SavesAfter.Count) { throw 'Save file set changed during the smoke.' }
    if ($SavesBefore.Count) {
        Assert-AIHeroesEntriesEqual -Expected $SavesBefore -Actual $SavesAfter -Label 'save files'
    }
    $Screenshots = @(foreach ($Name in $Names) {
        $Path = Join-Path $Root "Docs\$Name"
        $File = Get-Item -LiteralPath $Path
        if ($File.LastWriteTimeUtc -lt $Started -or $File.Length -lt 1000) {
            throw "Screenshot is not fresh: $Name"
        }
        Copy-Item -LiteralPath $Path -Destination (Join-Path $Directory $Name)
        [pscustomobject]@{
            path = "Docs/VisualRuns/${Id}_$Scenario/$Name"
            bytes = [long]$File.Length
            sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    })
    $Evidence = [pscustomobject][ordered]@{
        schema = 1
        status = 'PASS'
        scenario = $Scenario
        mode = 'real-window scripted fixture; not manual input or balance acceptance'
        started_utc = $Started.ToString('o')
        finished_utc = [DateTime]::UtcNow.ToString('o')
        pid = $Process.Id
        native_exit = $NativeExit
        compiled_snapshot_sha256 = $ManifestHash
        binary = $Manifest.binary
        inputs_verified_before_and_after = $true
        save_files_checked = $SavesBefore.Count
        save_hashes_unchanged = $true
        screenshots = $Screenshots
        harness_pass_line = ($Log -split "`n" |
            Where-Object { $_ -match '\[LIVE GRID SMOKE\] PASS' } | Select-Object -Last 1).Trim()
    }
    $Path = Join-Path $Directory 'evidence.json'
    [IO.File]::WriteAllText($Path, ($Evidence | ConvertTo-Json -Depth 8), (New-Object Text.UTF8Encoding($false)))
    Write-Output "AIHEROES_GRAPHIC_SMOKE_PASS scenario=$Scenario screenshots=$($Screenshots.Count) native_exit=$NativeExit evidence=$Path"
} finally {
    Exit-AIHeroesNativeWork -Mutex $Mutex
}
