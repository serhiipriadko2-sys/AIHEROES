param([switch]$SkipBuild,[switch]$Visual)
$ErrorActionPreference='Stop'
$Root=Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'aiheroes_build_evidence.ps1')
$Run=[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$Evidence=Join-Path $Root ('Saved\QA\AmberBattle_'+$Run)
New-Item -ItemType Directory -Path $Evidence | Out-Null
Write-Output "AMBER_BATTLE_EVIDENCE=$Evidence"
function FileEntry($File){[pscustomobject]@{path=$File.FullName.Substring($Root.Length+1);bytes=$File.Length;sha256=(Get-FileHash -LiteralPath $File.FullName -Algorithm SHA256).Hash}}
function Assets { @(Get-ChildItem (Join-Path $Root 'Content\AmberMarch\Units'),(Join-Path $Root 'ArtSource\AmberVanguard') -Recurse -File | Sort-Object FullName | ForEach-Object {FileEntry $_}) }
function Saves { @(Get-ChildItem (Join-Path $Root 'Saved\SaveGames') -Filter '*.sav' -File -ErrorAction SilentlyContinue | Sort-Object Name | ForEach-Object {FileEntry $_}) }
function Step($Name,$Script,$Extra=@()){
    $P=Start-Process powershell.exe -ArgumentList (@('-NoProfile','-File',(Join-Path $Root $Script))+$Extra) -Wait -PassThru -WorkingDirectory $Root -RedirectStandardOutput "$Evidence\$Name.stdout.log" -RedirectStandardError "$Evidence\$Name.stderr.log"
    if($null -eq $P.ExitCode -or $P.ExitCode -ne 0){throw "Step $Name failed exit=$($P.ExitCode); inspect evidence logs"}
    Write-Output "AMBER_STEP_PASS=$Name"
}
$Receipt=[ordered]@{schema='aiheroes.amber-battle-qa.v1';status='IN_PROGRESS';evidence=$Evidence;real_model_loaded=$false;android_verified=$false;manual_input_verified=$false}
try {
    $Before=Assets; $BeforeSaves=Saves
    $Before | ConvertTo-Json -Depth 5 | Set-Content "$Evidence\assets-before.json" -Encoding UTF8
    $BeforeSaves | ConvertTo-Json -Depth 5 | Set-Content "$Evidence\saves-before.json" -Encoding UTF8
    if(-not $SkipBuild){Step 'build' 'Tools\build_aiheroes_editor.ps1'}
    $Snapshot=Join-Path $Root 'Docs\AIHEROES_BUILD_LATEST.json'
    $Manifest=Get-Content $Snapshot -Raw -Encoding UTF8 | ConvertFrom-Json
    Assert-AIHeroesCompiledSnapshot -Manifest $Manifest -Root $Root
    $ManifestHash=(Get-FileHash $Snapshot -Algorithm SHA256).Hash
    Step 'automation' 'Tools\run_aiheroes_automation.ps1' @('-CompiledSnapshot',$Snapshot)
    $Match=Select-String -LiteralPath "$Evidence\automation.stdout.log" -Pattern ' report=(.+?index\.json) provenance='
    if(@($Match).Count -ne 1){throw 'One fresh automation report required'}
    $ReportPath=$Match.Matches[0].Groups[1].Value
    $Report=Get-Content $ReportPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $Scoped=@($Report.tests | Where-Object {$_.fullTestPath -like 'AIHeroes.Visual.Battle.*'})
    if($Scoped.Count -ne 5 -or @($Scoped|Where-Object {$_.state -ne 'Success'}).Count -ne 0){throw 'All five battle art test groups required'}
    $Receipt.automation_report=$ReportPath;$Receipt.all_tests=@($Report.tests).Count;$Receipt.battle_art_tests=$Scoped.Count
    $Receipt.graphics=@()
    if($Visual){
        foreach($Scenario in @('Victory','Defeat','TwoGroups')){
            Step ('visual_'+$Scenario) 'Tools\verify_live_grid_smoke.ps1' @('-Scenario',$Scenario,'-CompiledSnapshot',$Snapshot)
            $M=Select-String -LiteralPath "$Evidence\visual_$Scenario.stdout.log" -Pattern 'evidence=(.+\.json)$'
            if(@($M).Count -ne 1){throw 'Missing graphical receipt'}
            $Receipt.graphics+=Get-Content $M.Matches[0].Groups[1].Value -Raw -Encoding UTF8 | ConvertFrom-Json
        }
        # Compact viewport: same real TwoGroups scenario, no altered simulation.
        $Mutex=Enter-AIHeroesNativeWork
        try {
            if(Get-Process UnrealEditor,UnrealEditor-Cmd,LiveCodingConsole -ErrorAction SilentlyContinue){throw 'Another game active; compact smoke not started'}
            Assert-AIHeroesCompiledSnapshot -Manifest $Manifest -Root $Root
            $D=Join-Path $Evidence 'compact_640';New-Item -ItemType Directory "$D\previous" -Force | Out-Null
            $Names=@('START','PAUSED','2X','RESULT','RETURN','DOSSIER') | ForEach-Object {'LIVE_GRID_V2_TWO_GROUPS_'+$_+'.png'}
            foreach($Name in $Names){$Old=Join-Path $Root ('Docs\'+$Name);if(Test-Path $Old){Copy-Item $Old "$D\previous\$Name"}}
            $Started=[DateTime]::UtcNow
            $P=Start-Process 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList @((Join-Path $Root 'AIHeroes.uproject'),'-game','-windowed','-ResX=640','-ResY=720','-ForceRes','-NoP4','-NoSplash','-NoSound','-AIHeroesLiveSmoke','-AIHeroesLiveSmokeScenario=TwoGroups',('-abslog='+$D+'\smoke.log')) -Wait -PassThru -WorkingDirectory $Root -RedirectStandardOutput "$D\stdout.log" -RedirectStandardError "$D\stderr.log"
            if($null -eq $P.ExitCode -or $P.ExitCode -ne 0){throw 'Compact process did not exit cleanly'}
            $Log=Get-Content "$D\smoke.log" -Raw
            if($Log -notmatch '\[LIVE GRID SMOKE\] PASS scenario=TwoGroups\s' -or $Log -match '\[LIVE GRID SMOKE\] FAIL'){throw 'Compact scenario evidence missing'}
            Add-Type -AssemblyName System.Drawing
            $Screens=@(foreach($Name in $Names){
                $Source=Join-Path $Root ('Docs\'+$Name);$F=Get-Item $Source
                if($F.LastWriteTimeUtc -lt $Started -or $F.Length -lt 1000){throw 'Stale compact image'}
                Copy-Item $Source "$D\$Name";$Image=[Drawing.Image]::FromFile("$D\$Name")
                try{if($Image.Width -ne 640 -or $Image.Height -ne 720){throw 'Actual compact PNG dimensions differ'}}finally{$Image.Dispose()}
                FileEntry (Get-Item "$D\$Name")
            })
            $Receipt.graphics+=[pscustomobject]@{status='PASS_SCRIPTED_REQUIRES_IMAGE_REVIEW';scenario='TwoGroups_640';native_exit=$P.ExitCode;screenshots=$Screens}
        }finally{Exit-AIHeroesNativeWork -Mutex $Mutex}
    }
    Assert-AIHeroesCompiledSnapshot -Manifest $Manifest -Root $Root
    if((Get-FileHash $Snapshot -Algorithm SHA256).Hash -ne $ManifestHash){throw 'Manifest changed'}
    $After=Assets;$AfterSaves=Saves
    Assert-AIHeroesEntriesEqual -Expected $Before -Actual $After -Label 'unit art assets'
    Assert-AIHeroesEntriesEqual -Expected $BeforeSaves -Actual $AfterSaves -Label 'player saves'
    $Receipt.assets=$After;$Receipt.asset_count=$After.Count;$Receipt.player_saves=$AfterSaves.Count
    $Receipt.source_files=@($Manifest.inputs).Count;$Receipt.binary=$Manifest.binary
    $Receipt.inputs_unchanged=$true;$Receipt.saves_unchanged=$true;$Receipt.assets_unchanged=$true
    $Receipt.status=if($Visual){'PASS_WINDOWS_BATTLE_ART_SCRIPTED'}else{'PASS_WINDOWS_BATTLE_ART_AUTOMATION_ONLY'}
}catch{$Receipt.status='PARTIAL_OR_FAIL';$Receipt.error=$_.Exception.Message}
finally{
    $Receipt.observed_utc=[DateTime]::UtcNow.ToString('o')
    $Receipt | ConvertTo-Json -Depth 12 | Set-Content "$Evidence\receipt.json" -Encoding UTF8
    [pscustomobject]$Receipt | Select-Object status,error,all_tests,battle_art_tests,source_files,asset_count,player_saves,evidence | ConvertTo-Json -Depth 3
}
if($Receipt.status -like 'PARTIAL*'){exit 2}
