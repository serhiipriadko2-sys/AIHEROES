# Tests the actual shared C++ kernel, not a rewritten model of its algorithm.
# Does not build/load an Unreal module, close a game, or touch player saves.
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Run = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$Output = Join-Path $Root ('Saved\QA\AdventureNative_' + $Run)
New-Item -ItemType Directory -Path $Output | Out-Null
$Candidates = @('C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
$VC = $Candidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $VC) { throw 'Known installed MSVC environment unavailable; nothing installed.' }
$InputFiles = @('Source/AIHeroes/Public/AIHeroesAdventurePathCore.h','Tests/adventure_path_native.cpp')
$Manifest = @($InputFiles | ForEach-Object {
    $P = Join-Path $Root $_
    [PSCustomObject]@{path=$_;bytes=(Get-Item $P).Length;sha256=(Get-FileHash $P -Algorithm SHA256).Hash}
})
$Source = Join-Path $Root 'Tests\adventure_path_native.cpp'
$Include = Join-Path $Root 'Source\AIHeroes\Public'
$Exe = Join-Path $Output 'adventure_native.exe'
$Obj = Join-Path $Output 'adventure_native.obj'
$Command = "call `"$VC`" >nul && cl.exe /nologo /std:c++17 /EHsc /W4 /WX /I`"$Include`" `"$Source`" /Fo`"$Obj`" /Fe`"$Exe`""
& cmd.exe /d /c $Command *> (Join-Path $Output 'compile.log')
if ($LASTEXITCODE -ne 0) { throw "Native C++ build failed; see $Output\compile.log" }
& $Exe *> (Join-Path $Output 'tests.log')
$TestExit = $LASTEXITCODE
$Text = Get-Content (Join-Path $Output 'tests.log') -Raw
if ($TestExit -ne 0 -or $Text -notmatch 'ADVENTURE_NATIVE_PASS checks=(\d+) generated_graphs=200 generated_queries=1600') {
    throw "Native tests failed; see $Output\tests.log"
}
$Checks = [int]$Matches[1]
foreach ($F in $Manifest) {
    if ((Get-FileHash (Join-Path $Root $F.path) -Algorithm SHA256).Hash -ne $F.sha256) {
        throw 'Source changed during native verification; no PASS receipt.'
    }
}
@{status='PASS_NATIVE_CPP_KERNEL_ONLY';checks=$Checks;generated_graphs=200;generated_queries=1600;
    files=$Manifest;engine_adapter_verified=$false;campaign_integration=$false;player_save_writes=0;
    executable_sha256=(Get-FileHash $Exe -Algorithm SHA256).Hash;observed_utc=[DateTime]::UtcNow.ToString('o')} |
    ConvertTo-Json -Depth 5 | Set-Content (Join-Path $Output 'receipt.json') -Encoding UTF8
Write-Output $Text.Trim()
Write-Output "NATIVE_RECEIPT=$Output\receipt.json"
