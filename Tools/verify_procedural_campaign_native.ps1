# MSVC C++17 contract tests: 5,120 seeded states and strict fog/guards.
# All outputs stay under Saved/QA. Never reads or writes player SaveGame.
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Stamp = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$Out = Join-Path $Root ('Saved\QA\ProceduralV2Native_' + $Stamp)
New-Item -ItemType Directory -Path $Out -ErrorAction Stop | Out-Null

$Candidates = @(
 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat',
 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
)
$VCTools = $Candidates | Where-Object { Test-Path -LiteralPath $_ } |
    Select-Object -First 1
if (!$VCTools) { throw 'Installed MSVC vcvars64.bat missing. No installation attempted.' }

$Files = @(
 'Source/AIHeroes/Public/AIHeroesAdventurePathCore.h',
 'Source/AIHeroes/Public/AIHeroesZoneTemplateCore.h',
 'Source/AIHeroes/Public/AIHeroesProceduralCampaignCore.h',
 'Tests/procedural_campaign_native.cpp'
)
$Manifest = @($Files | ForEach-Object {
    $File = Join-Path $Root $_
    if (!(Test-Path -LiteralPath $File)) { throw ('Missing production source ' + $File) }
    [pscustomobject]@{
        path = $_
        bytes = (Get-Item $File).Length
        sha256 = (Get-FileHash -LiteralPath $File -Algorithm SHA256).Hash.ToLower()
    }
})
$Source = Join-Path $Root 'Tests\procedural_campaign_native.cpp'
$Includes = Join-Path $Root 'Source\AIHeroes\Public'
$Exe = Join-Path $Out 'procedural_campaign_native.exe'
$Obj = Join-Path $Out 'procedural_campaign_native.obj'
$Command = 'call "' + $VCTools + '" >nul && cl.exe /nologo /std:c++17 /EHsc /W4 /WX /I"' + $Includes + '" "' + $Source + '" /Fo"' + $Obj + '" /Fe"' + $Exe + '"'
& cmd.exe /d /c $Command *> (Join-Path $Out 'compile.log')
if ($LASTEXITCODE -ne 0) {
    throw ('PROCEDURAL_V2_NATIVE_COMPILE_FAIL ' + (Join-Path $Out 'compile.log'))
}
& $Exe *> (Join-Path $Out 'tests.log')
$Code = $LASTEXITCODE
$Text = Get-Content (Join-Path $Out 'tests.log') -Raw
if ($Code -ne 0 -or
    $Text -notmatch 'PROCEDURAL_V2_NATIVE_PASS seeds=1024 variants=5 states=5120 checks=(\d+) schema=2') {
    throw ('PROCEDURAL_V2_NATIVE_TEST_FAIL ' + (Join-Path $Out 'tests.log') + ' ' + $Text)
}
$Assertions = [int]$Matches[1]
foreach ($File in $Manifest) {
    $Path = Join-Path $Root $File.path
    if ((Get-Item $Path).Length -ne $File.bytes -or
        (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLower() -ne $File.sha256) {
        throw ('PROCEDURAL_V2_NATIVE_SOURCE_DRIFT ' + $File.path)
    }
}
@{
    status = 'PASS_PORTABLE_CPP17_PROCEDURAL_V2'
    schemas = @(1,2)
    seeds = 1024
    variants_per_seed = 5
    validated_snapshots = 5120
    assertions = $Assertions
    compile_flags = '/std:c++17 /EHsc /W4 /WX'
    source_manifest = $Manifest
    executable_sha256 = (Get-FileHash $Exe -Algorithm SHA256).Hash.ToLower()
    user_save_writes = 0
    campaign_writes = 0
    timestamp_utc = [DateTime]::UtcNow.ToString('o')
} | ConvertTo-Json -Depth 7 | Set-Content (Join-Path $Out 'receipt.json') -Encoding UTF8
Write-Output $Text.Trim()
Write-Output ('PROCEDURAL_V2_NATIVE_QA_RECEIPT=' + (Join-Path $Out 'receipt.json'))
