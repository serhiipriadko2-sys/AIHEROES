param([switch]$BuildOnly)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$engine = 'C:\Program Files\Epic Games\UE_5.8\Engine'
$tag = 'RecruitQA_' + (Get-Date -Format 'yyyyMMdd_HHmmss')
$stage = Join-Path $root ('Saved\' + $tag)
$evidence = Join-Path $root ('Saved\QA\' + $tag)
$marker = Join-Path $root 'Saved\QA\recruitment-latest.json'
if (Test-Path $stage) { throw 'Refusing to overwrite a previous snapshot.' }
if (Get-Process UnrealEditor-Cmd -ErrorAction SilentlyContinue) {
    throw 'Another Unreal automation process is running; do not share its caches.'
}
New-Item -ItemType Directory -Path $stage,$evidence -Force | Out-Null
$manifest = @()
$sourceFiles = @('AIHeroes.uproject')
foreach ($dir in @('Source','Config','Content')) {
    Get-ChildItem (Join-Path $root $dir) -Recurse -File | ForEach-Object {
        $sourceFiles += $_.FullName.Substring($root.Length + 1)
    }
}
foreach ($rel in $sourceFiles) {
    $source = Join-Path $root $rel
    $target = Join-Path $stage $rel
    if ((Get-Item -LiteralPath $source).Attributes -band [IO.FileAttributes]::ReparsePoint) {
        throw "Reparse point refused: $rel"
    }
    $before = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
    New-Item -ItemType Directory -Path (Split-Path $target) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $target
    $copied = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash
    if ($before -ne $copied) { throw "Concurrent source edit during snapshot: $rel" }
    $manifest += [pscustomobject]@{path=$rel;bytes=(Get-Item $target).Length;sha256=$copied}
}
foreach ($entry in $manifest) {
    if ((Get-FileHash (Join-Path $root $entry.path) -Algorithm SHA256).Hash -ne $entry.sha256) {
        throw "Source changed before snapshot freeze: $($entry.path)"
    }
}
$manifest | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $evidence 'source-manifest.json') -Encoding UTF8
@{stage=$stage;evidence=$evidence;status='BUILDING';source_files=$manifest.Count} |
    ConvertTo-Json | Set-Content $marker -Encoding UTF8
$project = Join-Path $stage 'AIHeroes.uproject'
Write-Host "SNAPSHOT: $project"
Write-Host "EVIDENCE: $evidence"
Write-Host 'Original game process and original SaveGames are not touched.'
& 'C:\Program Files\dotnet\dotnet.exe' (Join-Path $engine 'Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll') `
    AIHeroesEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReload -MaxParallelActions=1 2>&1 |
    Tee-Object -FilePath (Join-Path $evidence 'build.log')
$buildCode = $LASTEXITCODE
if ($buildCode -ne 0) { throw "Unreal build failed: $buildCode. See build.log" }
$testCode = $null
if (-not $BuildOnly) {
    if (Get-Process UnrealEditor-Cmd -ErrorAction SilentlyContinue) {
        throw 'Another automation process appeared; build succeeded but tests are not run.'
    }
    & (Join-Path $engine 'Binaries\Win64\UnrealEditor-Cmd.exe') $project `
        -unattended -nop4 -nullrhi -nosplash '-ExecCmds=Automation RunTests AIHeroes' `
        '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$evidence\report" `
        '-log=RecruitmentAutomation.log' -stdout 2>&1 |
        Tee-Object -FilePath (Join-Path $evidence 'tests.log')
    $testCode = $LASTEXITCODE
    if ($testCode -ne 0) { throw "Unreal tests failed: $testCode. See tests.log" }
    $reportPath = Join-Path $evidence 'report\index.json'
    if (-not (Test-Path $reportPath)) { throw 'No JSON report; a zero exit code is insufficient.' }
    $report = Get-Content $reportPath -Raw | ConvertFrom-Json
    $recruitment = @($report.tests | Where-Object { $_.fullTestPath -like 'AIHeroes.Recruitment.*' })
    if ($recruitment.Count -ne 6 -or
        @($recruitment | Where-Object { $_.state -ne 'Success' }).Count -ne 0) {
        throw 'All six recruitment groups must execute and pass; missing/skipped is not PASS.'
    }
    if (@($report.tests | Where-Object { $_.state -ne 'Success' }).Count -ne 0) {
        throw 'A gameplay regression is failed, skipped or not run. Review the report.'
    }
    Write-Host ('VERIFIED: recruitment=6/6; all gameplay tests=' + $report.tests.Count)
}
@{stage=$stage;evidence=$evidence;status='COMPLETED';build_exit=$buildCode;
  test_exit=$testCode;source_files=$manifest.Count} | ConvertTo-Json |
    Set-Content $marker -Encoding UTF8
Write-Host 'Snapshot verification completed. Review report counts; no deployment or push performed.'
