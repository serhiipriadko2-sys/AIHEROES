param([switch]$Visual)
$ErrorActionPreference='Stop'
$Root=Split-Path -Parent $PSScriptRoot
$Qa=Join-Path $Root 'Saved\QA'
$Busy=@(Get-Process UnrealEditor,UnrealEditor-Cmd,cl -ErrorAction SilentlyContinue)
if($Busy.Count -gt 0){
  Write-Output 'RELIEF_V5_BLOCKED_EDITOR_OR_BUILD_ACTIVE: no processes terminated'
  exit 2
}
$Started=Get-Date
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$PSScriptRoot\verify_map_readability.ps1" -Visual
$Exit=$LASTEXITCODE
if($Exit -ne 0){throw "Map readability verification failed ($Exit)"}
$Dir=Get-ChildItem $Qa -Directory |
  Where-Object {$_.Name -like 'MapReadability_*' -and $_.LastWriteTime -ge $Started.AddSeconds(-2)} |
  Sort-Object LastWriteTime -Descending | Select-Object -First 1
if(!$Dir){throw 'No fresh run directory'}
$Receipt=Get-Content (Join-Path $Dir.FullName 'receipt.json') -Raw|ConvertFrom-Json
$Report=Get-Content (Join-Path $Dir.FullName 'report\index.json') -Raw|ConvertFrom-Json
$New=@(
 'AIHeroes.Visual.ReliefV5.RiverDepthAndSitePads',
 'AIHeroes.Visual.ReliefV5.DesktopMobileAssets',
 'AIHeroes.Visual.ReliefV5.DecorativeIslandNonColliding'
)
foreach($Id in $New){
  $Match=@($Report.tests|Where-Object {$_.fullTestPath -eq $Id -and $_.state -eq 'Success'})
  if($Match.Count -ne 1){throw "Missing passing Relief V5 test $Id"}
}
if($Receipt.status -ne 'PASS_TESTED_WINDOWS_SNAPSHOT' -or
   $Report.failed -ne 0 -or $Report.notRun -ne 0 -or
   $Report.succeededWithWarnings -ne 0 -or
   $Receipt.failed -ne 0 -or $Receipt.not_run -ne 0 -or
   @($Receipt.visual_runs).Count -ne 3 -or
   @($Receipt.screenshots).Count -ne 9 -or
   @($Receipt.source_drift).Count -ne 0 -or
   @($Receipt.save_drift).Count -ne 0 -or
   !$Receipt.dll_stable){
  throw 'Game verification incomplete or parallel source changed'
}
$Manifest=Get-Content (Join-Path $Root 'ArtSource\AmberMarch\ReliefV5\asset_manifest.json') -Raw|ConvertFrom-Json
if($Manifest.assets.Count -ne 2){throw 'Art inventory drift'}
foreach($Asset in $Manifest.assets){
  $File=Join-Path $Root ('ArtSource\AmberMarch\ReliefV5\'+$Asset.name+'.fbx')
  if((Get-FileHash $File -Algorithm SHA256).Hash.ToLowerInvariant() -ne $Asset.sha256){
    throw "Art asset hash drift $($Asset.name)"
  }
}
$Result=[ordered]@{
  status='PASS_TESTED_WINDOWS_RELIEF_V5'
  observed_at=(Get-Date).ToString('o')
  full_test_count=$Report.succeeded
  new_test_count=3
  tested_resolutions=@('1280x720','960x720','640x720')
  screenshots=9
  source_drift=0
  save_drift=0
  android_verified=$false
  actual_game_screenshot_inspection_required=$true
  underlying_report=$Dir.FullName
  report_sha256=(Get-FileHash (Join-Path $Dir.FullName 'report\index.json') -Algorithm SHA256).Hash.ToLowerInvariant()
}
$Out=Join-Path $Root 'Docs\AMBER_RELIEF_V5_WINDOWS_TEST_RECEIPT_2026-10-29.json'
if(Test-Path $Out){throw 'Final receipt already exists; inspect before repeating'}
$Result|ConvertTo-Json -Depth 5|Set-Content $Out -Encoding UTF8
$Check=Get-Content $Out -Raw|ConvertFrom-Json
if($Check.status -ne 'PASS_TESTED_WINDOWS_RELIEF_V5'){throw 'Readback mismatch'}
'RELIEF_V5_WINDOWS_TEST_PASS'
'RECEIPT='+$Out
'SHA256='+(Get-FileHash $Out -Algorithm SHA256).Hash.ToLowerInvariant()