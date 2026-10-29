$ErrorActionPreference='Stop'
$Root=Split-Path -Parent $PSScriptRoot
$Expected='$BeforeSaves=@((Get-Content "$MapEvidence\player-saves.json" -Raw -Encoding UTF8 | ConvertFrom-Json))'
$Line=@(Get-Content "$Root\Tools\verify_model_request.ps1" | Where-Object {$_.Trim().StartsWith('$BeforeSaves=')})
if($Line.Count -ne 1 -or $Line[0].Trim() -cne $Expected){throw 'Production parse expression changed; review required'}
# Execute only the exact inspected assignment, never arbitrary file content.
$Parse=[scriptblock]::Create('param($MapEvidence) '+$Expected+'; return ,$BeforeSaves')
$Dir=Join-Path $Root ('Saved\QA\ManifestArrayRegression_'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $Dir | Out-Null
$Checks=0
function Require($Condition,[string]$Reason){if(-not $Condition){throw $Reason}; $script:Checks++}
foreach($Count in @(0,1,4)){
    $Rows=@(for($I=0;$I -lt $Count;$I++){
        [pscustomobject]@{name="fixture_$I.sav";bytes=100+$I;sha256=('a'*64)}
    })
    ConvertTo-Json -InputObject $Rows -Depth 4 | Set-Content "$Dir\player-saves.json" -Encoding UTF8
    $Parsed=& $Parse $Dir
    Require ($Parsed.Count -eq $Count) "Wrong parsed count: $Count"
    for($I=0;$I -lt $Count;$I++){
        Require ($Parsed[$I].name -eq $Rows[$I].name -and $Parsed[$I].bytes -eq $Rows[$I].bytes -and $Parsed[$I].sha256 -eq $Rows[$I].sha256) 'Row did not round-trip'
    }
}
$Baseline=& $Parse $Dir
$Same=@($Baseline | ForEach-Object {[pscustomobject]@{name=$_.name;bytes=$_.bytes;sha256=$_.sha256}})
Require (@(Compare-Object $Baseline $Same -Property name,bytes,sha256).Count -eq 0) 'Equal saves marked as changed'
$Same[0].sha256='b'*64
Require (@(Compare-Object $Baseline $Same -Property name,bytes,sha256).Count -eq 2) 'Changed hash missed'
$Removed=@($Baseline | Select-Object -Skip 1)
Require (@(Compare-Object $Baseline $Removed -Property name,bytes,sha256).Count -eq 1) 'Missing save missed'
$Broken=@(Get-Content "$Dir\player-saves.json" -Raw -Encoding UTF8 | ConvertFrom-Json)
if($PSVersionTable.PSVersion.Major -eq 5){Require ($Broken.Count -eq 1 -and $Broken[0] -is [array]) 'Historical regression not reproduced on Windows PowerShell 5'}
$Receipt=[ordered]@{status='PASS_MANIFEST_ARRAY_REGRESSION';checks=$Checks;powershell=$PSVersionTable.PSVersion.ToString();evidence=$Dir;player_save_writes=0;unreal_launches=0}
$Receipt | ConvertTo-Json | Set-Content "$Dir\receipt.json" -Encoding UTF8
$Receipt | ConvertTo-Json
