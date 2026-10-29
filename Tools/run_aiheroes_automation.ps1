param(
    [ValidatePattern('^AIHeroes(?:\.[A-Za-z0-9_]+)*$')]
    [string]$Filter = 'AIHeroes',
    [string]$CompiledSnapshot
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'aiheroes_build_evidence.ps1')
$Mutex = Enter-AIHeroesNativeWork
try {
$InputsBefore = Get-AIHeroesInputSnapshot -Root $Root
$BinaryBefore = Get-AIHeroesBinarySnapshot -Root $Root
$BuildVerified = -not [string]::IsNullOrWhiteSpace($CompiledSnapshot)
$Manifest = $null
if ($BuildVerified) {
    $Manifest = Get-Content -LiteralPath $CompiledSnapshot -Raw -Encoding UTF8 | ConvertFrom-Json
    Assert-AIHeroesCompiledSnapshot -Manifest $Manifest -Root $Root
}
$EditorCmd = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $EditorCmd)) { throw 'Unreal Editor-Cmd is unavailable.' }
if (Get-Process -Name UnrealEditor-Cmd -ErrorAction SilentlyContinue) {
    throw 'Another Editor-Cmd is running; shared shader-cache automation was not started.'
}
$RunId = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$ReportDirectory = Join-Path $Root "Docs\AutomationRuns\$RunId"
New-Item -ItemType Directory -Path $ReportDirectory | Out-Null
$ReportPath = Join-Path $ReportDirectory 'index.json'
& $EditorCmd (Join-Path $Root 'AIHeroes.uproject') -unattended -nop4 -nullrhi -nosplash -nosound `
    "-ExecCmds=Automation RunTests $Filter" '-TestExit=Automation Test Queue Empty' -stdout `
    "-abslog=$ReportDirectory\run.log" "-ReportExportPath=$ReportDirectory" `
    *> (Join-Path $ReportDirectory 'stdout.log')
$NativeExit = $LASTEXITCODE
$InputsAfter = Get-AIHeroesInputSnapshot -Root $Root
$BinaryAfter = Get-AIHeroesBinarySnapshot -Root $Root
Assert-AIHeroesEntriesEqual -Expected $InputsBefore -Actual $InputsAfter
Assert-AIHeroesEntriesEqual -Expected @($BinaryBefore) -Actual @($BinaryAfter) -Label 'binary'
if ($BuildVerified) { Assert-AIHeroesCompiledSnapshot -Manifest $Manifest -Root $Root }
if ($NativeExit -ne 0) { throw "Unreal exited $NativeExit; report directory: $ReportDirectory" }
if (-not (Test-Path -LiteralPath $ReportPath)) { throw "Fresh automation report is missing: $ReportPath" }
$Report = Get-Content -LiteralPath $ReportPath -Raw -Encoding UTF8 | ConvertFrom-Json
$Tests = @($Report.tests)
$Failures = @($Tests | Where-Object { $_.state -ne 'Success' })
# Unreal can exit 0 even when an assertion failed. Fresh report is authoritative.
if ($Tests.Count -eq 0 -or $Failures.Count -gt 0 -or $Report.failed -gt 0 -or
    $Report.notRun -gt 0 -or $Report.inProcess -gt 0 -or
    ($Report.succeeded + $Report.succeededWithWarnings) -ne $Tests.Count) {
    throw "AIHEROES_AUTOMATION_FAIL tests=$($Tests.Count) failed=$($Report.failed) report=$ReportPath"
}
$Evidence = [pscustomobject][ordered]@{
    schema = 1
    status = 'passed'
    build_verified = $BuildVerified
    scope = 'fixed Source/ + Config/ + project input hashes and fixed DLL; not manual gameplay acceptance'
    created_utc = [DateTime]::UtcNow.ToString('o')
    filter = $Filter
    tests = $Tests.Count
    warnings = $Report.succeededWithWarnings
    inputs = $InputsAfter
    binary = $BinaryAfter
    report_sha256 = (Get-FileHash -LiteralPath $ReportPath -Algorithm SHA256).Hash.ToLowerInvariant()
}
$EvidencePath = Join-Path $ReportDirectory 'provenance.json'
[IO.File]::WriteAllText($EvidencePath, ($Evidence | ConvertTo-Json -Depth 8), (New-Object Text.UTF8Encoding($false)))
Write-Output "AIHEROES_AUTOMATION_PASS tests=$($Tests.Count) warnings=$($Report.succeededWithWarnings) build_verified=$BuildVerified report=$ReportPath provenance=$EvidencePath"
} finally {
    Exit-AIHeroesNativeWork -Mutex $Mutex
}
