# Compiles production C++17 generator + 5,120 deterministic generated worlds.
# Writes only under Saved/QA, not the active UE campaign or human save slots.
$ErrorActionPreference='Stop'
$Root=Split-Path -Parent $PSScriptRoot
$Stamp=[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$Output=Join-Path $Root ('Saved\QA\ZoneTemplateNative_'+$Stamp)
New-Item -ItemType Directory -Path $Output -ErrorAction Stop|Out-Null
$Candidates=@(
'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat',
'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
$VC=$Candidates|Where-Object{Test-Path -LiteralPath $_}|Select-Object -First 1
if(!$VC){throw 'No installed MSVC vcvars64.bat; nothing installed.'}
$Inputs=@('Source/AIHeroes/Public/AIHeroesZoneTemplateCore.h',
          'Source/AIHeroes/Public/AIHeroesAdventurePathCore.h',
          'Tests/zone_template_native.cpp')
$Manifest=@($Inputs|ForEach-Object{
    $F=Join-Path $Root $_
    if(!(Test-Path -LiteralPath $F)){throw "Missing source: $F"}
    [PSCustomObject]@{path=$_;bytes=(Get-Item $F).Length;sha256=(Get-FileHash $F -Algorithm SHA256).Hash.ToLower()}
})
$Source=Join-Path $Root 'Tests\zone_template_native.cpp'
$Include=Join-Path $Root 'Source\AIHeroes\Public'
$Exe=Join-Path $Output 'zone_template_native.exe'
$Obj=Join-Path $Output 'zone_template_native.obj'
$Command='call "'+$VC+'" >nul && cl.exe /nologo /std:c++17 /EHsc /W4 /WX /I"'+$Include+'" "'+$Source+'" /Fo"'+$Obj+'" /Fe"'+$Exe+'"'
& cmd.exe /d /c $Command *> (Join-Path $Output 'compile.log')
if($LASTEXITCODE -ne0){throw "NATIVE_COMPILE_FAILED: $Output\compile.log"}
& $Exe *> (Join-Path $Output 'tests.log')
$Code=$LASTEXITCODE
$Text=Get-Content (Join-Path $Output 'tests.log') -Raw
if($Code -ne0 -or $Text -notmatch 'ZONE_TEMPLATE_NATIVE_PASS seeds=1024 variants=5 worlds=5120 checks=(\d+) schema=1'){
    throw "NATIVE_TEST_FAILED: $Output\tests.log"
}
$Checks=[int]$Matches[1]
foreach($F in $Manifest){
    $Current=(Get-FileHash (Join-Path $Root $F.path) -Algorithm SHA256).Hash.ToLower()
    if($Current -ne $F.sha256){throw 'SOURCE_DRIFT_DURING_NATIVE_QA'}
}
@{status='PASS_NATIVE_CXX17_GENERATOR';generated_worlds=5120;
    assertions=$Checks;schema=1;files=$Manifest;build_standard='c++17';
    executable_sha256=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToLower();
    player_save_writes=0;active_campaign_writes=0;unreal_adapter_tested=$false;
    observed_utc=[DateTime]::UtcNow.ToString('o')
} |ConvertTo-Json -Depth 6 |Set-Content -Path (Join-Path $Output 'receipt.json') -Encoding UTF8
Write-Output $Text.Trim()
Write-Output ('ZONE_NATIVE_RECEIPT='+ (Join-Path $Output 'receipt.json'))
