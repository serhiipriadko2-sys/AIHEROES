param([switch]$Visual)
$ErrorActionPreference='Stop'
$Root=Split-Path -Parent $PSScriptRoot
$Engine='C:\Program Files\Epic Games\UE_5.8\Engine'
$Busy=@(Get-CimInstance Win32_Process | Where-Object {$_.Name -in @('UnrealEditor.exe','UnrealEditor-Cmd.exe','cl.exe') -or ($_.Name -eq 'dotnet.exe' -and $_.CommandLine -match 'UnrealBuildTool')})
if($Busy.Count){throw 'Existing engine/build active. No process terminated; verification not started.'}
$Run=[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$Evidence=Join-Path $Root ('Saved\QA\ModelRequestValidation_'+$Run)
New-Item -ItemType Directory -Path $Evidence | Out-Null
Write-Output "MODEL_REQUEST_EVIDENCE=$Evidence"
$Receipt=[ordered]@{schema='aiheroes.model-request-validation.v1';status='IN_PROGRESS';evidence=$Evidence;observed_utc=[DateTime]::UtcNow.ToString('o');real_model_loaded=$false;network_calls=0;android_verified=$false}
try {
    $ArgsForMap=@('-NoProfile','-File',"$Root\Tools\verify_map_readability.ps1")
    if($Visual){$ArgsForMap+='-Visual'}
    & powershell.exe @ArgsForMap *> "$Evidence\map-validation.log"
    if($LASTEXITCODE -ne 0){throw "Base/map verification failed: $LASTEXITCODE"}
    $Match=@(Select-String -LiteralPath "$Evidence\map-validation.log" -Pattern '^MAP_READABILITY_EVIDENCE=(.+)$')
    if($Match.Count -ne 1){throw 'Exactly one fresh validation directory required'}
    $MapEvidence=$Match[0].Matches[0].Groups[1].Value.Trim()
    if(-not $MapEvidence.StartsWith((Join-Path $Root 'Saved\QA\MapReadability_'))){throw 'Unexpected validation root'}
    $Map=Get-Content -LiteralPath "$MapEvidence\receipt.json" -Raw -Encoding UTF8 | ConvertFrom-Json
    if($Map.status -ne 'PASS_TESTED_WINDOWS_SNAPSHOT'){throw 'Base verification is not stable PASS'}
    $Report=Get-Content -LiteralPath "$MapEvidence\report\index.json" -Raw -Encoding UTF8 | ConvertFrom-Json
    $Expected=@('VisibleAllowlist','RoadGuardAndBudgetParity','SeedBattleAndArmyLocks','MalformedInputsClearOutput','StableOrderAndReadOnly','LegacyGoalsAndSlots') | ForEach-Object {'AIHeroes.ModelRequest.'+$_}
    $Scoped=@($Report.tests | Where-Object {$_.fullTestPath -like 'AIHeroes.ModelRequest.*'})
    if($Scoped.Count -ne 6 -or @($Scoped | Where-Object {$_.state -ne 'Success'}).Count -ne 0 -or @(Compare-Object ($Expected|Sort-Object) ($Scoped.fullTestPath|Sort-Object)).Count -ne 0){throw 'All six model export tests must pass'}
    $Receipt.map_evidence=$MapEvidence
    $Receipt.all_tests=@($Report.tests).Count
    $Receipt.export_tests=$Scoped.Count
    $Receipt.map_visual_runs=$Map.visual_runs
    if(Get-Process UnrealEditor,UnrealEditor-Cmd,cl -ErrorAction SilentlyContinue){throw 'Another session appeared; smoke not started'}
    $Info=New-Object System.Diagnostics.ProcessStartInfo
    $Info.FileName="$Engine\Binaries\Win64\UnrealEditor.exe"
    $Info.Arguments='"'+(Join-Path $Root 'AIHeroes.uproject')+'" -game -windowed -ResX=1280 -ResY=720 -ForceRes -NoSplash -NoP4 -NoSound -AIHeroesModelRequestSmoke "-abslog='+$Evidence+'\export.log"'
    $Info.WorkingDirectory=$Root; $Info.UseShellExecute=$false
    $Child=New-Object System.Diagnostics.Process; $Child.StartInfo=$Info
    if(-not $Child.Start()){throw 'Smoke launch failed'}
    if(-not $Child.WaitForExit(150000)){throw 'Owned smoke exceeded bound; not forcibly terminated'}
    $Receipt.export_exit=$Child.ExitCode
    if($Child.ExitCode -ne 0){throw 'Model request smoke exit failure'}
    $Pass=@(Select-String -LiteralPath "$Evidence\export.log" -Pattern '\[MODEL REQUEST SMOKE\] PASS exports=4 .*directory=(.+)$')
    if($Pass.Count -ne 1){throw 'Fresh complete model smoke PASS required'}
    $Directory=$Pass[0].Matches[0].Groups[1].Value.Trim()
    if(-not $Directory.StartsWith(($Root -replace '\\','/')+'/Saved/QA/ModelRequestSmoke_') -and -not $Directory.StartsWith((Join-Path $Root 'Saved\QA\ModelRequestSmoke_'))){throw 'Unexpected fixture root'}
    & python "$Root\Tools\verify_model_request_contract.py" --directory $Directory --report "$Evidence\interop.json" *> "$Evidence\interop.log"
    if($LASTEXITCODE -ne 0){throw 'Python bridge contract rejected Unreal exports'}
    $Receipt.interop=Get-Content "$Evidence\interop.json" -Raw -Encoding UTF8 | ConvertFrom-Json
    $Receipt.export_directory=$Directory
    $Frozen=Get-Content "$MapEvidence\source-manifest.json" -Raw -Encoding UTF8 | ConvertFrom-Json
    $Drift=@($Frozen | Where-Object {-not (Test-Path (Join-Path $Root $_.path)) -or (Get-FileHash (Join-Path $Root $_.path) -Algorithm SHA256).Hash -ne $_.sha256})
    $BeforeSaves=@((Get-Content "$MapEvidence\player-saves.json" -Raw -Encoding UTF8 | ConvertFrom-Json))
    $AfterSaves=@(Get-ChildItem "$Root\Saved\SaveGames" -Filter '*.sav' -File -ErrorAction SilentlyContinue | Sort-Object Name | ForEach-Object {[pscustomobject]@{name=$_.Name;bytes=$_.Length;sha256=(Get-FileHash $_.FullName -Algorithm SHA256).Hash}})
    $SaveDrift=@(Compare-Object $BeforeSaves $AfterSaves -Property name,bytes,sha256)
    $Receipt.source_drift=$Drift.path; $Receipt.save_drift=$SaveDrift
    $Receipt.saves_checked=$AfterSaves.Count
    $Receipt.dll_stable=(Get-FileHash "$Root\Binaries\Win64\UnrealEditor-AIHeroes.dll" -Algorithm SHA256).Hash -eq $Map.dll_sha256
    if($Drift.Count -or $SaveDrift.Count -or -not $Receipt.dll_stable){throw 'State drift; scope must be reconciled'}
    $Receipt.status='PASS_WINDOWS_EXPORT_AND_SCHEMA'
} catch {
    $Receipt.status='PARTIAL_OR_FAIL'
    $Receipt.error=$_.Exception.Message
} finally {
    $Receipt.observed_utc=[DateTime]::UtcNow.ToString('o')
    $Receipt | ConvertTo-Json -Depth 10 | Set-Content "$Evidence\receipt.json" -Encoding UTF8
    $Receipt | ConvertTo-Json -Depth 10
}
if($Receipt.status -ne 'PASS_WINDOWS_EXPORT_AND_SCHEMA'){exit 2}
