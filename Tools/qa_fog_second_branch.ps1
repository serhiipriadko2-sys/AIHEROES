# Continue a uniquely tagged, disposable AIHeroesFogQA campaign.
# Assumes authorized FOREST battle finished, at 4/5 discovered sites.
# Never touches campaign saves or untagged Unreal windows.
$ErrorActionPreference = 'Stop'
$root = 'C:\github\AIHEROES'
$tag = 'AIHeroesFogQA.log'
$log = Join-Path $root ('Saved\Logs\' + $tag)
$saveDir = Join-Path $root 'Saved\SaveGames'
$before = @{}
if(Test-Path $saveDir){
    Get-ChildItem $saveDir -File | ForEach-Object{
        $before[$_.FullName]=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }
}
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class FogBranchQA {
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
}
'@
$candidates=@(Get-CimInstance Win32_Process|Where-Object{
    $_.Name -eq 'UnrealEditor.exe' -and
    $_.CommandLine -like '*AIHeroes.uproject*' -and
    $_.CommandLine -like '*-log=AIHeroesFogQA.log*'
})
if($candidates.Count -ne 1){throw 'FOG_BRANCH_BOUNDARY: unique tagged game required'}
$g=Get-Process -Id $candidates[0].ProcessId -ErrorAction Stop
if($g.MainWindowHandle -eq [IntPtr]::Zero -or -not(Test-Path $log)){throw 'FOG_BRANCH_BOUNDARY: game or log missing'}
if(-not(Select-String -LiteralPath $log -Pattern '\[AI HEROES SCOUT\] discovered=OUTPOST known=4/5' -Quiet)){
    throw 'Expected previous forest reconnaissance did not complete'
}
function Post([uint32]$msg,[int]$w,[int]$l) {
    if(-not [FogBranchQA]::PostMessage($g.MainWindowHandle,$msg,[IntPtr]$w,[IntPtr]$l)){
        throw "FOG_BRANCH_INPUT_REJECTED $msg"
    }
}
function Press([int]$code){
    Post 0x100 $code 0
    Start-Sleep -Milliseconds 220
    Post 0x101 $code 0
    Start-Sleep -Milliseconds 530
}
function Click([int]$x,[int]$y){
    $p=($y -shl 16) -bor $x
    Post 0x200 0 $p
    Start-Sleep -Milliseconds 160
    Post 0x201 1 $p
    Start-Sleep -Milliseconds 160
    Post 0x202 0 $p
    Start-Sleep -Milliseconds 610
}
function Need([string]$pattern){
    $m=Select-String -LiteralPath $log -Pattern $pattern | Select-Object -Last 1
    if(-not $m){throw ('FOG_BRANCH_NO_EVENT '+$pattern)}
    Write-Output $m.Line
}
function Await([string]$pattern){
    for($i=0;$i -lt 80;$i++){
        $m=Select-String -LiteralPath $log -Pattern $pattern | Select-Object -Last 1
        if($m){Write-Output $m.Line;return}
        Start-Sleep -Milliseconds 260
    }
    throw ('FOG_BRANCH_TIMEOUT '+$pattern)
}
# Travel FOREST -> HQ -> ORE uses 4 moves, so replenish budget first.
Press 0x20
Await '\[AI HEROES\] Day 2: income collected'
# Existing Ore actor is a known destination. A click alone is free.
Click 808 350
Await '\[AI HEROES\] ORE selected'
Press 0x0D
Await '\[AI HEROES LIVE\] BEGIN node=ORE'
Press 0x42
Await '\[AI HEROES LIVE\] COMMIT node=ORE'
Need '\[AI HEROES SCOUT\] discovered=RUINS known=5/5 no_order=1'
Need '\[AI HEROES\] LIVE RESULT: VICTORY'
Press 0x0D
Await '\[AI HEROES\] Returned to map'
& (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $tag -OutputPath (Join-Path $root 'Docs\AIHEROES_FOG_ALL_REVEALED_2026-10-28.png')
$after=@{}
if(Test-Path $saveDir){
    Get-ChildItem $saveDir -File|ForEach-Object{
        $after[$_.FullName]=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }
}
if($before.Count -ne $after.Count){throw 'FOG_BRANCH_SAVE_COUNT_CHANGED'}
foreach($p in $before.Keys){
    if(-not $after.ContainsKey($p) -or $after[$p] -ne $before[$p]){
        throw ('FOG_BRANCH_USER_SAVE_CHANGED '+$p)
    }
}
Write-Output ('FOG_ALL_DISCOVERED_PASS sites=5/5 battle_player_authorized=1 save_writes=0 pid='+$g.Id)
