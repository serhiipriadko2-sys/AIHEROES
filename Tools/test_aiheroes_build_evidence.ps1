# Dependency-free PowerShell 5.1 fixture tests: no Unreal process or user save access.
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'aiheroes_build_evidence.ps1')
$script:Checks = 0
function Assert-Rejected {
    param([string]$Name, [scriptblock]$Action)
    $Rejected = $false
    try { & $Action } catch { $Rejected = $true }
    if (-not $Rejected) { throw "Expected rejection: $Name" }
    $script:Checks++
}
function Copy-Manifest {
    param([object]$Manifest)
    return ($Manifest | ConvertTo-Json -Depth 8 | ConvertFrom-Json)
}
$TempParent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/')
$Fixture = Join-Path $TempParent ('aiheroes-evidence-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $Fixture | Out-Null
try {
    foreach ($Directory in @('Source', 'Config', 'Binaries\Win64', 'Docs', '.iskra')) {
        New-Item -ItemType Directory -Path (Join-Path $Fixture $Directory) -Force | Out-Null
    }
    $Project = Join-Path $Fixture 'AIHeroes.uproject'
    $Cpp = Join-Path $Fixture 'Source\Fixture.cpp'
    $Dll = Join-Path $Fixture 'Binaries\Win64\UnrealEditor-AIHeroes.dll'
    [IO.File]::WriteAllText($Project, '{}')
    [IO.File]::WriteAllText($Cpp, 'fixture source')
    [IO.File]::WriteAllText((Join-Path $Fixture 'Config\DefaultGame.ini'), '[fixture]')
    [IO.File]::WriteAllText($Dll, 'fixture binary')
    $Inputs = Get-AIHeroesInputSnapshot -Root $Fixture
    if ($Inputs.Count -ne 3) { throw "Unexpected input array shape/count: $($Inputs.Count)" }
    $script:Checks++
    $Manifest = [pscustomobject]@{
        schema = 1; status = 'compiled'; build_verified = $true
        target = 'AIHeroesEditor Win64 Development'
        inputs = $Inputs; binary = Get-AIHeroesBinarySnapshot -Root $Fixture
    }
    Assert-AIHeroesCompiledSnapshot -Manifest $Manifest -Root $Fixture
    $script:Checks++
    Assert-AIHeroesCompiledSnapshot -Manifest (Copy-Manifest $Manifest) -Root $Fixture
    $script:Checks++
    [IO.File]::WriteAllText((Join-Path $Fixture '.iskra\private.txt'), 'excluded canon fixture')
    [IO.File]::WriteAllText((Join-Path $Fixture 'Docs\receipt.txt'), 'excluded output')
    Assert-AIHeroesEntriesEqual -Expected $Inputs -Actual (Get-AIHeroesInputSnapshot -Root $Fixture)
    $script:Checks++

    [IO.File]::WriteAllText($Cpp, 'changed source')
    Assert-Rejected 'source drift' { Assert-AIHeroesCompiledSnapshot -Manifest $Manifest -Root $Fixture }
    [IO.File]::WriteAllText($Cpp, 'fixture source')
    $Extra = Join-Path $Fixture 'Source\New.h'
    [IO.File]::WriteAllText($Extra, '// new input')
    Assert-Rejected 'new source file' { Assert-AIHeroesCompiledSnapshot -Manifest $Manifest -Root $Fixture }
    Remove-Item -LiteralPath $Extra
    [IO.File]::WriteAllText($Dll, 'different binary')
    Assert-Rejected 'binary drift' { Assert-AIHeroesCompiledSnapshot -Manifest $Manifest -Root $Fixture }
    [IO.File]::WriteAllText($Dll, 'fixture binary')

    $Bad = Copy-Manifest $Manifest; $Bad.schema = 99
    Assert-Rejected 'unknown schema' { Assert-AIHeroesCompiledSnapshot -Manifest $Bad -Root $Fixture }
    $Bad = Copy-Manifest $Manifest; $Bad.status = 'building'
    Assert-Rejected 'unfinished build' { Assert-AIHeroesCompiledSnapshot -Manifest $Bad -Root $Fixture }
    $Bad = Copy-Manifest $Manifest; $Bad.build_verified = 'true'
    Assert-Rejected 'untyped build flag' { Assert-AIHeroesCompiledSnapshot -Manifest $Bad -Root $Fixture }
    $Bad = Copy-Manifest $Manifest; $Bad.inputs[0].path = '..\outside.txt'
    Assert-Rejected 'untrusted source path' { Assert-AIHeroesCompiledSnapshot -Manifest $Bad -Root $Fixture }
    $Bad = Copy-Manifest $Manifest; $Bad.binary.path = '..\outside.dll'
    Assert-Rejected 'untrusted binary path' { Assert-AIHeroesCompiledSnapshot -Manifest $Bad -Root $Fixture }
    $Bad = Copy-Manifest $Manifest; $Bad.inputs[1] = $Bad.inputs[0]
    Assert-Rejected 'duplicate input' { Assert-AIHeroesCompiledSnapshot -Manifest $Bad -Root $Fixture }
    $Bad = Copy-Manifest $Manifest; $Bad.inputs[0].sha256 = 'not a hash'
    Assert-Rejected 'malformed hash' { Assert-AIHeroesCompiledSnapshot -Manifest $Bad -Root $Fixture }
    $Bad = Copy-Manifest $Manifest; $Bad.inputs[0].bytes = -1
    Assert-Rejected 'malformed bytes' { Assert-AIHeroesCompiledSnapshot -Manifest $Bad -Root $Fixture }
    $Bad = Copy-Manifest $Manifest; $Bad.inputs[0].bytes = [string]$Bad.inputs[0].bytes
    Assert-Rejected 'untyped bytes' { Assert-AIHeroesCompiledSnapshot -Manifest $Bad -Root $Fixture }
    $Bad = Copy-Manifest $Manifest; $Bad.schema = '1'
    Assert-Rejected 'untyped schema' { Assert-AIHeroesCompiledSnapshot -Manifest $Bad -Root $Fixture }
    Assert-Rejected 'missing fields' { Assert-AIHeroesCompiledSnapshot -Manifest ([pscustomobject]@{schema = 1}) -Root $Fixture }
    Assert-Rejected 'malformed JSON' { '{bad' | ConvertFrom-Json }
    New-Item -ItemType Directory -Path (Join-Path $Fixture 'Plugins') | Out-Null
    Assert-Rejected 'uncovered plugin inputs' { Get-AIHeroesInputSnapshot -Root $Fixture }
    Remove-Item -LiteralPath (Join-Path $Fixture 'Plugins')
    Remove-Item -LiteralPath $Dll
    Assert-Rejected 'missing binary' { Assert-AIHeroesCompiledSnapshot -Manifest $Manifest -Root $Fixture }

    $Mutex = Enter-AIHeroesNativeWork
    try {
        # Another OS process, not recursive acquisition by this thread.
        $Helper = (Join-Path $PSScriptRoot 'aiheroes_build_evidence.ps1').Replace("'", "''")
        $Child = ". '$Helper'; try { `$m = Enter-AIHeroesNativeWork; Exit-AIHeroesNativeWork `$m; exit 7 } catch { exit 0 }"
        $Encoded = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($Child))
        & powershell.exe -NoProfile -EncodedCommand $Encoded
        if ($LASTEXITCODE -ne 0) { throw 'Concurrent process acquired an already held native-work mutex.' }
        $script:Checks++
    } finally { Exit-AIHeroesNativeWork -Mutex $Mutex }
    $Mutex = Enter-AIHeroesNativeWork
    Exit-AIHeroesNativeWork -Mutex $Mutex
    $script:Checks++
    Write-Output "AIHEROES_BUILD_EVIDENCE_TEST_PASS checks=$script:Checks powershell=$($PSVersionTable.PSVersion)"
} finally {
    # Verified absolute target is only the random fixture directory directly under temp.
    $Resolved = (Resolve-Path -LiteralPath $Fixture).Path
    if ([IO.Path]::GetDirectoryName($Resolved) -ne $TempParent -or
        [IO.Path]::GetFileName($Resolved) -notmatch '^aiheroes-evidence-[a-f0-9]{32}$') {
        throw 'Refusing cleanup outside the verified test fixture.'
    }
    Remove-Item -LiteralPath $Resolved -Recurse -Force
}
