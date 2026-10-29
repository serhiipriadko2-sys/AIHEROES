param([switch]$Visual)
$ErrorActionPreference='Stop'
$Root=Split-Path -Parent $PSScriptRoot
$Engine='C:\Program Files\Epic Games\UE_5.8\Engine'
if(Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue){throw 'Close or finish the existing editor/game first; no process is terminated by this tool.'}
$Run=[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$Evidence=Join-Path $Root "Saved\QA\TravelValidation_$Run"
New-Item -ItemType Directory -Path $Evidence | Out-Null
function SourceManifest {
    @(Get-ChildItem (Join-Path $Root 'Source') -Recurse -File | Sort-Object FullName | ForEach-Object {
        [pscustomobject]@{path=$_.FullName.Substring($Root.Length+1);bytes=$_.Length;sha256=(Get-FileHash $_.FullName -Algorithm SHA256).Hash}
    })
}
function SameSources($Before) {
    $After=SourceManifest
    return (($Before | ConvertTo-Json -Depth 3 -Compress) -ceq ($After | ConvertTo-Json -Depth 3 -Compress))
}
$Before=SourceManifest
$Before | ConvertTo-Json -Depth 3 | Set-Content (Join-Path $Evidence 'source-manifest.json') -Encoding UTF8
Write-Output "TRAVEL_EVIDENCE=$Evidence"
$Project=Join-Path $Root 'AIHeroes.uproject'
& 'C:\Program Files\dotnet\dotnet.exe' (Join-Path $Engine 'Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll') `
    AIHeroesEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1 `
    *> (Join-Path $Evidence 'build.log')
$BuildExit=$LASTEXITCODE
if($BuildExit -ne 0){throw "Build failed: $BuildExit; $Evidence\build.log"}
$BuildStable=SameSources $Before
if(-not $BuildStable){throw "SOURCE_DRIFT during build; repeat from a stable source snapshot. Evidence: $Evidence"}
if(Get-Process UnrealEditor-Cmd -ErrorAction SilentlyContinue){throw 'Another automation owns the shader cache; no parallel test run started.'}
$Dll=Join-Path $Root 'Binaries\Win64\UnrealEditor-AIHeroes.dll'
$DllHash=(Get-FileHash $Dll -Algorithm SHA256).Hash
$ReportDir=Join-Path $Evidence 'report'
& (Join-Path $Engine 'Binaries\Win64\UnrealEditor-Cmd.exe') $Project -unattended -nop4 -nullrhi -nosplash -nosound `
    '-ExecCmds=Automation RunTests AIHeroes' '-TestExit=Automation Test Queue Empty' `
    "-ReportExportPath=$ReportDir" "-abslog=$Evidence\tests.log" -stdout *> (Join-Path $Evidence 'stdout.log')
$TestExit=$LASTEXITCODE
if($TestExit -ne 0){throw "Automation exited $TestExit; see $Evidence"}
$ReportPath=Join-Path $ReportDir 'index.json'
if(-not (Test-Path $ReportPath)){throw 'Fresh JSON report missing; exit code alone is not acceptance.'}
$Report=Get-Content $ReportPath -Raw -Encoding UTF8 | ConvertFrom-Json
$Tests=@($Report.tests)
$Travel=@($Tests | Where-Object {$_.fullTestPath -like 'AIHeroes.TravelV2.*'})
$Path=@($Tests | Where-Object {$_.fullTestPath -like 'AIHeroes.AdventurePath.*'})
if($Travel.Count -ne 8 -or $Path.Count -ne 8 -or
    @($Tests | Where-Object {$_.state -ne 'Success'}).Count -ne 0 -or
    $Report.failed -gt 0 -or $Report.notRun -gt 0 -or $Report.inProcess -gt 0 -or
    ($Report.succeeded+$Report.succeededWithWarnings) -ne $Tests.Count){throw "Tests incomplete/failed; inspect $ReportPath"}
$VisualStatus='NOT_RUN'
if($Visual){
    if(Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue){throw 'Another editor/game appeared; visual check not started.'}
    # Windows GUI executables can return to PowerShell before their process exits.
    # Bind the wait and exit code to this exact child, never a stale LASTEXITCODE.
    $VisualArgs=@(('"'+$Project+'"'),'-game','-windowed','-ResX=1280','-ResY=720',
        '-NoSplash','-NoP4','-NoSound','-AIHeroesTravelSmoke',('"-abslog='+$Evidence+'\visual.log"'))
    $Child=Start-Process -FilePath (Join-Path $Engine 'Binaries\Win64\UnrealEditor.exe') `
        -ArgumentList $VisualArgs -PassThru -RedirectStandardOutput (Join-Path $Evidence 'visual-stdout.log') `
        -RedirectStandardError (Join-Path $Evidence 'visual-stderr.log')
    # Pin the process handle: Windows PowerShell 5.1 can otherwise lose ExitCode.
    $ChildHandle=$Child.Handle
    if(-not $Child.WaitForExit(150000)){throw "Owned visual process $($Child.Id) exceeded its bound; not terminated. Evidence: $Evidence"}
    $VisualExit=$Child.ExitCode
    Write-Output "VISUAL_CHILD_PID=$($Child.Id) EXIT=$VisualExit"
    if($null -eq $VisualExit -or $VisualExit -ne 0 -or -not (Test-Path "$Evidence\visual.log") -or
        -not (Select-String -LiteralPath "$Evidence\visual.log" -SimpleMatch '[TRAVEL V2 SMOKE] PASS' -Quiet)){
        throw "Visual smoke failed or did not attest completion: $Evidence"
    }
    $VisualStatus='SCRIPTED_RENDER_PASS_REQUIRES_IMAGE_INSPECTION'
}
$Stable=SameSources $Before
$SameDll=(Get-FileHash $Dll -Algorithm SHA256).Hash -eq $DllHash
$Receipt=[ordered]@{status=if($Stable -and $SameDll){'PASS_WINDOWS_TESTED_SNAPSHOT'}else{'PARTIAL_SOURCE_OR_DLL_DRIFT'};
    observed_utc=[DateTime]::UtcNow.ToString('o');root=$Root;evidence=$Evidence;build_exit=$BuildExit;test_exit=$TestExit;
    all_tests=$Tests.Count;travel_tests=$Travel.Count;path_tests=$Path.Count;failed=$Report.failed;not_run=$Report.notRun;
    visual=$VisualStatus;source_files=$Before.Count;source_unchanged=$Stable;dll_unchanged=$SameDll;dll_sha256=$DllHash;
    android_verified=$false;real_model_connected=$false;published=$false}
$Receipt | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $Evidence 'receipt.json') -Encoding UTF8
$Receipt | ConvertTo-Json -Depth 4 | Write-Output
if(-not $Stable -or -not $SameDll){exit 2}
exit 0
