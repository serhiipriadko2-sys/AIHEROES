param([switch]$Visual)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Engine = 'C:\Program Files\Epic Games\UE_5.8\Engine'
if (Get-Process UnrealEditor,UnrealEditor-Cmd,cl -ErrorAction SilentlyContinue) {
    throw 'Existing editor/test/compiler is active. No process is terminated.'
}
$Run = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$Evidence = Join-Path $Root ('Saved\QA\TravelViewValidation_' + $Run)
New-Item -ItemType Directory -Path $Evidence | Out-Null
$Project = Join-Path $Root 'AIHeroes.uproject'
function SourceManifest {
    @(Get-ChildItem (Join-Path $Root 'Source') -Recurse -File | Sort-Object FullName | ForEach-Object {
        [PSCustomObject]@{path=$_.FullName.Substring($Root.Length+1);bytes=$_.Length;sha256=(Get-FileHash $_.FullName -Algorithm SHA256).Hash}
    })
}
function SaveManifest {
    @(Get-ChildItem (Join-Path $Root 'Saved\SaveGames') -Filter '*.sav' -File -ErrorAction SilentlyContinue | Sort-Object Name | ForEach-Object {
        [PSCustomObject]@{name=$_.Name;bytes=$_.Length;sha256=(Get-FileHash $_.FullName -Algorithm SHA256).Hash}
    })
}
$Sources = SourceManifest
$Saves = SaveManifest
$Sources | ConvertTo-Json -Depth 5 | Set-Content "$Evidence\source-manifest.json" -Encoding UTF8
$Saves | ConvertTo-Json -Depth 5 | Set-Content "$Evidence\player-saves-before.json" -Encoding UTF8
Write-Output "TRAVEL_VIEW_EVIDENCE=$Evidence"
& 'C:\Program Files\dotnet\dotnet.exe' (Join-Path $Engine 'Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll') `
    AIHeroesEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1 `
    *> "$Evidence\build.log"
$BuildExit = $LASTEXITCODE
if ($BuildExit -ne 0) { throw "Build failed: $BuildExit. Evidence: $Evidence" }
$Dll = Join-Path $Root 'Binaries\Win64\UnrealEditor-AIHeroes.dll'
$DllHash = (Get-FileHash $Dll -Algorithm SHA256).Hash
if (Get-Process UnrealEditor-Cmd -ErrorAction SilentlyContinue) { throw 'Another test session appeared; tests not started.' }
& (Join-Path $Engine 'Binaries\Win64\UnrealEditor-Cmd.exe') $Project -unattended -nop4 -nullrhi -nosplash -nosound `
    '-ExecCmds=Automation RunTests AIHeroes' '-TestExit=Automation Test Queue Empty' -stdout `
    "-abslog=$Evidence\tests.log" "-ReportExportPath=$Evidence\report" *> "$Evidence\stdout.log"
$TestExit = $LASTEXITCODE
if ($TestExit -ne 0) { throw "Tests exited $TestExit. Evidence: $Evidence" }
$Report = Get-Content "$Evidence\report\index.json" -Raw -Encoding UTF8 | ConvertFrom-Json
$Tests = @($Report.tests)
$ViewTests = @($Tests | Where-Object {$_.fullTestPath -like 'AIHeroes.TravelView.*'})
if ($Tests.Count -eq 0 -or $ViewTests.Count -ne 8 -or
    @($Tests | Where-Object {$_.state -ne 'Success'}).Count -ne 0 -or
    $Report.failed -gt 0 -or $Report.notRun -gt 0 -or $Report.inProcess -gt 0 -or
    ($Report.succeeded + $Report.succeededWithWarnings) -ne $Tests.Count) { throw "Missing/failed tests: $Evidence" }
$VisualResult = 'NOT_RUN'
if ($Visual) {
    if (Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue) { throw 'Another engine session appeared; visual check not started.' }
    $Args = @(('"'+$Project+'"'),'-game','-windowed','-ResX=1280','-ResY=720',
        '-NoSplash','-NoP4','-NoSound','-AIHeroesTravelViewSmoke',('"-abslog='+$Evidence+'\visual.log"'))
    $Child = Start-Process (Join-Path $Engine 'Binaries\Win64\UnrealEditor.exe') -ArgumentList $Args -PassThru
    $PinnedHandle = $Child.Handle
    if (-not $Child.WaitForExit(150000)) { throw "Owned visual process $($Child.Id) exceeded bound; not forcibly terminated." }
    $VisualExit = $Child.ExitCode
    Write-Output "VISUAL_PID=$($Child.Id) EXIT=$VisualExit"
    if ($null -eq $VisualExit -or $VisualExit -ne 0 -or -not (Test-Path "$Evidence\visual.log") -or
        -not (Select-String -LiteralPath "$Evidence\visual.log" -SimpleMatch '[TRAVEL VIEW SMOKE] PASS' -Quiet)) { throw "Visual check failed: $Evidence" }
    $VisualResult = 'SCRIPTED_PASS_IMAGE_INSPECTION_PENDING'
}
$SourceDrift = @(Compare-Object $Sources (SourceManifest) -Property path,bytes,sha256)
$SaveDrift = @(Compare-Object $Saves (SaveManifest) -Property name,bytes,sha256)
$DllStable = (Get-FileHash $Dll -Algorithm SHA256).Hash -eq $DllHash
$Receipt = [ordered]@{status=if($SourceDrift.Count -eq 0 -and $SaveDrift.Count -eq 0 -and $DllStable){'PASS_WINDOWS_TESTED_SNAPSHOT'}else{'PARTIAL_DRIFT'};
    observed_utc=[DateTime]::UtcNow.ToString('o');evidence=$Evidence;build_exit=$BuildExit;test_exit=$TestExit;
    tests=$Tests.Count;view_tests=$ViewTests.Count;failed=$Report.failed;not_run=$Report.notRun;visual=$VisualResult;
    source_files=$Sources.Count;source_drift=$SourceDrift;player_saves=$Saves.Count;save_drift=$SaveDrift;dll_stable=$DllStable;dll_sha256=$DllHash;
    android_verified=$false;real_model_connected=$false;published=$false}
$Receipt | ConvertTo-Json -Depth 6 | Set-Content "$Evidence\receipt.json" -Encoding UTF8
$Receipt | ConvertTo-Json -Depth 6
if ($Receipt.status -ne 'PASS_WINDOWS_TESTED_SNAPSHOT') { exit 2 }
