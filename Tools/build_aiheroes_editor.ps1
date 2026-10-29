$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'aiheroes_build_evidence.ps1')
$Mutex = Enter-AIHeroesNativeWork
try {
    $LatestPath = Join-Path $Root 'Docs\AIHEROES_BUILD_LATEST.json'
    # Fail closed: a failed/stale attempt cannot leave the previous receipt current.
    if (Test-Path -LiteralPath $LatestPath) { Remove-Item -LiteralPath $LatestPath }
    if (Get-Process -Name UnrealEditor,UnrealEditor-Cmd,LiveCodingConsole -ErrorAction SilentlyContinue) {
        throw 'Unreal/Live Coding process is running. Build not started; no process was closed. Save and close the relevant editor/game before building.'
    }
    $InputsBefore = Get-AIHeroesInputSnapshot -Root $Root
    $RunId = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
    $Directory = Join-Path $Root "Docs\BuildRuns\$RunId"
    New-Item -ItemType Directory -Path $Directory -ErrorAction Stop | Out-Null
    $Dotnet = 'C:\Program Files\dotnet\dotnet.exe'
    $UBT = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
    if (-not (Test-Path -LiteralPath $Dotnet) -or -not (Test-Path -LiteralPath $UBT)) { throw 'Unreal build tools are unavailable.' }
    & $Dotnet $UBT AIHeroesEditor Win64 Development "-Project=$Root\AIHeroes.uproject" `
        -WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1 2>&1 |
        Tee-Object -FilePath (Join-Path $Directory 'build.log')
    if ($LASTEXITCODE -ne 0) { throw "AIHEROES_BUILD_FAIL exit=$LASTEXITCODE log=$Directory\build.log" }
    $InputsAfter = Get-AIHeroesInputSnapshot -Root $Root
    Assert-AIHeroesEntriesEqual -Expected $InputsBefore -Actual $InputsAfter
    $Manifest = [pscustomobject][ordered]@{
        schema = 1
        status = 'compiled'
        build_verified = $true
        created_utc = [DateTime]::UtcNow.ToString('o')
        target = 'AIHeroesEditor Win64 Development'
        scope = 'AIHeroes.uproject + all Source/ and Config/ files; engine and Content excluded'
        inputs = $InputsAfter
        binary = Get-AIHeroesBinarySnapshot -Root $Root
    }
    Assert-AIHeroesCompiledSnapshot -Manifest $Manifest -Root $Root
    $Json = $Manifest | ConvertTo-Json -Depth 8
    $CompiledPath = Join-Path $Directory 'compiled.json'
    [IO.File]::WriteAllText($CompiledPath, $Json, (New-Object Text.UTF8Encoding($false)))
    # Same-directory move publishes only a complete JSON file.
    $StagingPath = "$LatestPath.$RunId.tmp"
    [IO.File]::WriteAllText($StagingPath, $Json, (New-Object Text.UTF8Encoding($false)))
    Move-Item -LiteralPath $StagingPath -Destination $LatestPath -Force
    Write-Output "AIHEROES_BUILD_PASS inputs=$($InputsAfter.Count) manifest=$CompiledPath latest=$LatestPath"
} finally {
    Exit-AIHeroesNativeWork -Mutex $Mutex
}
