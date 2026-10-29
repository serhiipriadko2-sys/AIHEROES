param([switch]$Visual, [switch]$Snapshot)
$ErrorActionPreference='Stop'
$Root=Split-Path -Parent $PSScriptRoot
$Engine='C:\Program Files\Epic Games\UE_5.8\Engine'
if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl -ErrorAction SilentlyContinue){throw 'Another engine/build is active. No process was terminated.'}
$Run=[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$Evidence=Join-Path $Root ('Saved\QA\MapReadability_'+$Run)
New-Item -ItemType Directory -Path $Evidence | Out-Null
$Project=Join-Path $Root 'AIHeroes.uproject'
function Sources {
    $Items=@(Get-ChildItem "$Root\Source","$Root\Config" -Recurse -File)+@(Get-Item $Project)
    @($Items | Sort-Object FullName | ForEach-Object {
        [PSCustomObject]@{path=$_.FullName.Substring($Root.Length+1);bytes=$_.Length;sha256=(Get-FileHash $_.FullName -Algorithm SHA256).Hash}
    })
}
function Saves {
    @(Get-ChildItem "$Root\Saved\SaveGames" -Filter '*.sav' -File -ErrorAction SilentlyContinue | Sort-Object Name | ForEach-Object {
        [PSCustomObject]@{name=$_.Name;bytes=$_.Length;sha256=(Get-FileHash $_.FullName -Algorithm SHA256).Hash}
    })
}
$Before=Sources; $SaveBefore=Saves
$Before | ConvertTo-Json -Depth 4 | Set-Content "$Evidence\source-manifest.json" -Encoding UTF8
$SaveBefore | ConvertTo-Json -Depth 4 | Set-Content "$Evidence\player-saves.json" -Encoding UTF8
Write-Output "MAP_READABILITY_EVIDENCE=$Evidence"
$BuildRoot=$Root
if($Snapshot){
    $BuildRoot=Join-Path $Root ('Saved\MapQA_'+$Run)
    New-Item -ItemType Directory -Path $BuildRoot | Out-Null
    foreach($F in $Before){
        $Source=Join-Path $Root $F.path; $Target=Join-Path $BuildRoot $F.path
        $Info=Get-Item -LiteralPath $Source
        if(($Info.Attributes -band [IO.FileAttributes]::ReparsePoint) -or $Info.Length -le 0){throw "Linked/empty snapshot input: $($F.path)"}
        New-Item -ItemType Directory -Path (Split-Path $Target) -Force | Out-Null
        Copy-Item -LiteralPath $Source -Destination $Target
        if((Get-FileHash $Target -Algorithm SHA256).Hash -ne $F.sha256){throw "Copy drift: $($F.path)"}
    }
    if(@(Compare-Object $Before (Sources) -Property path,bytes,sha256).Count -gt 0){throw 'Sources changed during snapshot creation. No build started.'}
    $Project=Join-Path $BuildRoot 'AIHeroes.uproject'
    Write-Output "FROZEN_BUILD_ROOT=$BuildRoot"
}
& 'C:\Program Files\dotnet\dotnet.exe' "$Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" `
    AIHeroesEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1 *> "$Evidence\build.log"
$BuildExit=$LASTEXITCODE
if($BuildExit -ne 0){throw "Build failed: $BuildExit. Evidence: $Evidence"}
$Dll="$BuildRoot\Binaries\Win64\UnrealEditor-AIHeroes.dll"
$DllHash=(Get-FileHash $Dll -Algorithm SHA256).Hash
if(Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue){throw 'Engine appeared after build; tests not started.'}
& "$Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $Project -unattended -nop4 -nullrhi -nosplash -nosound `
    '-ExecCmds=Automation RunTests AIHeroes' '-TestExit=Automation Test Queue Empty' -stdout `
    "-abslog=$Evidence\tests.log" "-ReportExportPath=$Evidence\report" *> "$Evidence\stdout.log"
$TestExit=$LASTEXITCODE
if($TestExit -ne 0){throw "Test process failed: $TestExit. Evidence: $Evidence"}
$R=Get-Content "$Evidence\report\index.json" -Raw -Encoding UTF8 | ConvertFrom-Json
$Tests=@($R.tests); $Scoped=@($Tests | Where-Object {$_.fullTestPath -like 'AIHeroes.Readability.*'})
$Expected=@('SafeMapBounds','LabelsDoNotOverlapPanelsOrEachOther','HiddenAndInvalidInputs',
    'StableSelectedPriority','ResolutionAndCrowdingSweep','RepeatedChevronGeometry') | ForEach-Object {'AIHeroes.Readability.'+$_}
if($Tests.Count -eq 0 -or $Scoped.Count -ne 6 -or
    @(Compare-Object ($Expected | Sort-Object) ($Scoped.fullTestPath | Sort-Object)).Count -ne 0 -or
    @($Tests | Where-Object {$_.state -ne 'Success'}).Count -gt 0 -or $R.failed -gt 0 -or
    $R.notRun -gt 0 -or $R.inProcess -gt 0 -or ($R.succeeded+$R.succeededWithWarnings) -ne $Tests.Count){throw 'Fresh complete passing report required.'}
$Screens=@(); $VisualRuns=@()
if($Visual){
    foreach($Width in @(1280,960,640)){
        if(Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue){throw 'Another game appeared; visual run not started.'}
        $Dir=Join-Path $Evidence ("viewport_"+$Width)
        New-Item -ItemType Directory -Path $Dir | Out-Null
        $Started=[DateTime]::UtcNow
        $Arguments=@(('"'+$Project+'"'),'-game','-windowed',('-ResX='+$Width),'-ResY=720',
            '-NoSplash','-NoP4','-NoSound','-AIHeroesTravelViewSmoke',('"-abslog='+$Dir+'\visual.log"'))
        $Child=Start-Process "$Engine\Binaries\Win64\UnrealEditor.exe" -ArgumentList $Arguments -PassThru
        $PinnedHandle=$Child.Handle
        if(-not $Child.WaitForExit(150000)){throw "Owned process $($Child.Id) exceeded bound; not forcibly terminated."}
        $VisualExit=$Child.ExitCode
        if($null -eq $VisualExit -or $VisualExit -ne 0 -or
            -not (Select-String "$Dir\visual.log" -SimpleMatch '[TRAVEL VIEW SMOKE] PASS' -Quiet)){throw "Visual run failed: $Dir"}
        foreach($Name in @('VIEW_READY.png','VIEW_MOVING.png','VIEW_OVER_BUDGET.png')){
            $InputFile=Get-Item "$BuildRoot\Saved\QA\TravelPresentation\$Name"
            if($InputFile.Length -le 0 -or $InputFile.LastWriteTimeUtc -lt $Started){throw "Stale/missing screenshot: $Name"}
            $OutputFile=Join-Path $Dir $Name
            Copy-Item -LiteralPath $InputFile.FullName -Destination $OutputFile
            $Hash=(Get-FileHash $OutputFile -Algorithm SHA256).Hash
            if($Hash -ne (Get-FileHash $InputFile.FullName -Algorithm SHA256).Hash){throw 'Screenshot copy mismatch'}
            $Screens+=[PSCustomObject]@{path=$OutputFile.Substring($Root.Length+1);bytes=$InputFile.Length;sha256=$Hash;width_requested=$Width;height_requested=720}
        }
        $VisualRuns+=[PSCustomObject]@{width=$Width;height=720;pid=$Child.Id;exit=$VisualExit;status='SCRIPTED_PASS_REQUIRES_IMAGE_INSPECTION'}
        Write-Output "READABILITY_VISUAL_PASS width=$Width pid=$($Child.Id)"
    }
}
$SourceDrift=@(Compare-Object $Before (Sources) -Property path,bytes,sha256)
$SaveDrift=@(Compare-Object $SaveBefore (Saves) -Property name,bytes,sha256)
$DllStable=(Get-FileHash $Dll -Algorithm SHA256).Hash -eq $DllHash
$FrozenDrift=@($Before | Where-Object {
    $P=Join-Path $BuildRoot $_.path
    -not (Test-Path -LiteralPath $P) -or (Get-FileHash $P -Algorithm SHA256).Hash -ne $_.sha256
})
$Receipt=[ordered]@{status=if($FrozenDrift.Count -eq 0 -and $SaveDrift.Count -eq 0 -and $DllStable){'PASS_TESTED_WINDOWS_SNAPSHOT'}else{'PARTIAL_DRIFT'};
    snapshot_mode=[bool]$Snapshot;build_root=$BuildRoot;frozen_source_drift=$FrozenDrift;
    observed_utc=[DateTime]::UtcNow.ToString('o');evidence=$Evidence;build_exit=$BuildExit;test_exit=$TestExit;
    all_tests=$Tests.Count;readability_tests=$Scoped.Count;viewport_test_cases=60;failed=$R.failed;not_run=$R.notRun;
    warnings=$R.succeededWithWarnings;visual_runs=$VisualRuns;screenshots=$Screens;source_files=$Before.Count;source_drift=$SourceDrift;
    player_saves=$SaveBefore.Count;save_drift=$SaveDrift;dll_stable=$DllStable;dll_sha256=$DllHash;
    android_verified=$false;full_ui_accessibility_verified=$false;real_model_connected=$false;published=$false}
$Receipt | ConvertTo-Json -Depth 8 | Set-Content "$Evidence\receipt.json" -Encoding UTF8
$Receipt | ConvertTo-Json -Depth 8
if($Receipt.status -ne 'PASS_TESTED_WINDOWS_SNAPSHOT'){exit 2}
