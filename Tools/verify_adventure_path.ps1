# Isolated Unreal verification of the new pure strategic-path module ONLY.
# Never replaces the loaded game DLL or copies player saves/private configuration.
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Engine = 'C:\Program Files\Epic Games\UE_5.8\Engine'
if (Get-Process cl,UnrealEditor-Cmd -ErrorAction SilentlyContinue) {
    throw 'Another compile/automation is active. No verification started.'
}
$Run = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$Stage = Join-Path $Root ('Saved\AdventurePathQA_' + $Run)
$Evidence = Join-Path $Root ('Saved\QA\AdventurePath_' + $Run)
New-Item -ItemType Directory -Path $Stage,$Evidence | Out-Null
$Files = @('AIHeroes.uproject','Source/AIHeroesEditor.Target.cs',
    'Source/AIHeroes/AIHeroes.Build.cs','Source/AIHeroes/Private/AIHeroes.cpp',
    'Source/AIHeroes/Public/AIHeroesAdventurePath.h',
    'Source/AIHeroes/Public/AIHeroesAdventurePathCore.h',
    'Source/AIHeroes/Private/AIHeroesAdventurePath.cpp',
    'Source/AIHeroes/Private/Tests/AIHeroesAdventurePathTests.cpp')
$Manifest = @()
foreach ($Relative in $Files) {
    $Original = Join-Path $Root $Relative
    $Target = Join-Path $Stage $Relative
    $Info = Get-Item -LiteralPath $Original
    if ($Info.Length -eq 0 -or ($Info.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Empty or linked input rejected: $Relative"
    }
    New-Item -ItemType Directory -Path (Split-Path $Target) -Force | Out-Null
    Copy-Item -LiteralPath $Original -Destination $Target
    $Hash = (Get-FileHash -LiteralPath $Original -Algorithm SHA256).Hash
    if ($Hash -ne (Get-FileHash -LiteralPath $Target -Algorithm SHA256).Hash) {
        throw "Snapshot mismatch: $Relative"
    }
    $Manifest += [PSCustomObject]@{path=$Relative;bytes=$Info.Length;sha256=$Hash}
}
$Manifest | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $Evidence 'source-manifest.json') -Encoding UTF8
Write-Output "ADVENTURE_PATH_STAGE=$Stage"
Write-Output "ADVENTURE_PATH_EVIDENCE=$Evidence"
Write-Output 'Scope: pure path module only; NOT integrated campaign acceptance.'
$Project = Join-Path $Stage 'AIHeroes.uproject'
& 'C:\Program Files\dotnet\dotnet.exe' (Join-Path $Engine 'Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll') `
    AIHeroesEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1 `
    *> (Join-Path $Evidence 'build.log')
$BuildExit = $LASTEXITCODE
if ($BuildExit -ne 0) { throw "Build failed: $BuildExit. See $Evidence\build.log" }
if (Get-Process UnrealEditor-Cmd -ErrorAction SilentlyContinue) {
    throw 'Another automation is active; build passed, tests not run.'
}
$ReportDir = Join-Path $Evidence 'report'
& (Join-Path $Engine 'Binaries\Win64\UnrealEditor-Cmd.exe') $Project `
    -unattended -nop4 -nullrhi -nosplash -nosound '-ExecCmds=Automation RunTests AIHeroes.AdventurePath' `
    '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$ReportDir" "-abslog=$Evidence\tests.log" `
    -stdout *> (Join-Path $Evidence 'stdout.log')
$TestExit = $LASTEXITCODE
if ($TestExit -ne 0) { throw "Test process failed: $TestExit" }
$Report = Get-Content (Join-Path $ReportDir 'index.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$Tests = @($Report.tests)
if ($Tests.Count -ne 8 -or @($Tests | Where-Object {$_.state -ne 'Success'}).Count -ne 0 -or
    $Report.failed -gt 0 -or $Report.notRun -gt 0 -or $Report.inProcess -gt 0) {
    throw 'All eight path test groups must run and pass. No missing/skipped test is accepted.'
}
foreach ($File in $Manifest) {
    if ((Get-FileHash (Join-Path $Root $File.path) -Algorithm SHA256).Hash -ne $File.sha256) {
        throw "SOURCE_DRIFT: $($File.path). Results attest snapshot only."
    }
}
$Receipt = @{status='PASS_ISOLATED_UNREAL_MODULE';stage=$Stage;evidence=$Evidence;
    build_exit=$BuildExit;test_exit=$TestExit;tests=$Tests.Count;source_files=$Manifest.Count;
    files=$Manifest;campaign_integration=$false;android_verified=$false;player_save_writes=0;
    observed_utc=[DateTime]::UtcNow.ToString('o')}
$Receipt | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $Evidence 'receipt.json') -Encoding UTF8
Write-Output "ADVENTURE_PATH_PASS tests=$($Tests.Count) source_files=$($Manifest.Count) receipt=$Evidence\receipt.json"
