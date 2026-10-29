param()
$ErrorActionPreference='Stop'
$Root=Split-Path -Parent $PSScriptRoot
Set-Location $Root
$Run=[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$Evidence=Join-Path $Root ('Saved\QA\LocalExchangeValidation_'+$Run)
New-Item -ItemType Directory -Path $Evidence | Out-Null
Write-Output "LOCAL_EXCHANGE_VALIDATION=$Evidence"
$Receipt=[ordered]@{schema='aiheroes.local-exchange-validation.v1';status='PARTIAL_OR_FAIL';real_model_loaded=$false;android_verified=$false;evidence=$Evidence}
try {
    & powershell.exe -NoProfile -File "$PSScriptRoot\verify_model_request.ps1" *> "$Evidence\base.log"
    if($LASTEXITCODE -ne 0){throw 'Base build/test/export verification did not pass'}
    $MatchesFound=@(Select-String -LiteralPath "$Evidence\base.log" -Pattern '^MODEL_REQUEST_EVIDENCE=(.+)$')
    if($MatchesFound.Count -ne 1){throw 'Expected one fresh base evidence directory'}
    $BaseDir=$MatchesFound[0].Matches[0].Groups[1].Value.Trim()
    if(-not $BaseDir.StartsWith((Join-Path $Root 'Saved\QA\ModelRequestValidation_'))){throw 'Invalid base evidence root'}
    $Base=Get-Content "$BaseDir\receipt.json" -Raw -Encoding UTF8 | ConvertFrom-Json
    if($Base.status -ne 'PASS_WINDOWS_EXPORT_AND_SCHEMA'){throw 'Base verification not stable PASS'}
    $Report=Get-Content "$($Base.map_evidence)\report\index.json" -Raw -Encoding UTF8 | ConvertFrom-Json
    $Expected=@('EndpointAndOptIn','RequestMenuAndFogParity','FragmentedHTTPFrame','RejectsAmbiguousHTTP','EnvelopeIdentityAndFinish','DuplicatesAndJSONBounds','CancelAndClockPreflight') | ForEach-Object {'AIHeroes.LocalExchange.'+$_}
    $Scoped=@($Report.tests | Where-Object {$_.fullTestPath -like 'AIHeroes.LocalExchange.*'})
    if($Scoped.Count -ne 7 -or @($Scoped | Where-Object {$_.state -ne 'Success'}).Count -or @(Compare-Object ($Expected|Sort-Object) ($Scoped.fullTestPath|Sort-Object)).Count){throw 'All seven new automation groups required'}
    & python "$PSScriptRoot\qa_local_exchange.py" --root $Root *> "$Evidence\loopback.log"
    if($LASTEXITCODE -ne 0){throw 'Loopback mock scenario did not pass'}
    $Q=@(Select-String -LiteralPath "$Evidence\loopback.log" -Pattern '^LOCAL_EXCHANGE_EVIDENCE=(.+)$')
    if($Q.Count -ne 1){throw 'Expected one fresh loopback evidence directory'}
    $LocalDir=$Q[0].Matches[0].Groups[1].Value.Trim()
    if(-not $LocalDir.StartsWith((Join-Path $Root 'Saved\QA\LocalExchange_'))){throw 'Invalid loopback evidence root'}
    $Local=Get-Content "$LocalDir\receipt.json" -Raw -Encoding UTF8 | ConvertFrom-Json
    if($Local.status -ne 'PASS_WINDOWS_ASYNC_LOOPBACK_MOCK_ONLY'){throw 'Loopback evidence is not stable PASS'}
    $Frozen=@((Get-Content "$($Base.map_evidence)\source-manifest.json" -Raw -Encoding UTF8 | ConvertFrom-Json))
    foreach($F in $Frozen){if(-not (Test-Path -LiteralPath (Join-Path $Root $F.path)) -or (Get-FileHash (Join-Path $Root $F.path) -Algorithm SHA256).Hash -ne $F.sha256){throw ('Build/source drift: '+$F.path)}}
    $Map=Get-Content "$($Base.map_evidence)\receipt.json" -Raw -Encoding UTF8 | ConvertFrom-Json
    if((Get-FileHash "$Root\Binaries\Win64\UnrealEditor-AIHeroes.dll" -Algorithm SHA256).Hash -ne $Map.dll_sha256){throw 'Build binary changed'}
    $Receipt.status='PASS_WINDOWS_ASYNC_LOOPBACK_MOCK_ONLY'
    $Receipt.base_evidence=$BaseDir; $Receipt.loopback_evidence=$LocalDir
    $Receipt.all_tests=@($Report.tests).Count; $Receipt.local_exchange_tests=7
    $Receipt.synthetic_network_cases=@($Local.cases).Count
    $Receipt.source_files=$Frozen.Count; $Receipt.source_unchanged=$true
    $Receipt.player_saves=$Local.player_saves; $Receipt.saves_unchanged=$Local.saves_unchanged
    $Receipt.dll_sha256=$Map.dll_sha256
} catch { $Receipt.error=$_.Exception.Message }
finally {
    $Receipt.observed_utc=[DateTime]::UtcNow.ToString('o')
    $Receipt | ConvertTo-Json -Depth 8 | Set-Content "$Evidence\receipt.json" -Encoding UTF8
    $Receipt | ConvertTo-Json -Depth 8
}
if($Receipt.status -ne 'PASS_WINDOWS_ASYNC_LOOPBACK_MOCK_ONLY'){exit 2}
