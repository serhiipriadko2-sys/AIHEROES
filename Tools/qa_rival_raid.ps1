# Manual-playthrough smoke for player-authorized capture followed by a rival raid.
# No real save writes. A separate UnrealEditor.exe must be launched with
# -log=AIHeroesRivalRaidQA.log. Do not run against another window.
$ErrorActionPreference='Stop'
$root='C:\github\AIHEROES'
$tag='AIHeroesRivalRaidQA.log'
$log=Join-Path $root 'Saved\Logs\AIHeroesRivalRaidQA.log'
$dir=Join-Path $root 'Saved\SaveGames'
$before=@{}
if(Test-Path $dir){
    Get-ChildItem -LiteralPath $dir -File | ForEach-Object {
        $before[$_.FullName]=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }
}
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class AIRivalRaidKeys {
  [DllImport("user32.dll")]
  public static extern bool PostMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
}
'@
$c=@(Get-CimInstance Win32_Process | Where-Object {
    $_.Name -eq 'UnrealEditor.exe' -and $_.CommandLine -like '*AIHeroes.uproject*' -and
    $_.CommandLine -like '*-log=AIHeroesRivalRaidQA.log*'
})
if($c.Count -ne 1){throw "QA_BOUNDARY: Expected one private game, got $($c.Count)"}
$game=Get-Process -Id $c[0].ProcessId -ErrorAction Stop
if($game.MainWindowHandle -eq [IntPtr]::Zero -or -not(Test-Path $log)){
    throw 'QA_BOUNDARY: window and log required'
}
Write-Output ("RIVAL_RAID_QA_TARGET_PID="+$game.Id)
function Post([uint32]$m,[int]$w,[int]$l){
    if(-not [AIRivalRaidKeys]::PostMessage($game.MainWindowHandle,$m,[IntPtr]$w,[IntPtr]$l)){
        throw ('PostMessage failed: '+$m)
    }
}
function Press([int]$vk){
    Post 0x100 $vk 0
    Start-Sleep -Milliseconds 180
    Post 0x101 $vk 0
    Start-Sleep -Milliseconds 330
}
function Click([int]$x,[int]$y){
    $param=($y -shl 16) -bor $x
    Post 0x200 0 $param
    Start-Sleep -Milliseconds 170
    Post 0x201 1 $param
    Start-Sleep -Milliseconds 190
    Post 0x202 0 $param
    Start-Sleep -Milliseconds 450
}
function Expect([string]$re){
    $line=Select-String -LiteralPath $log -Pattern $re | Select-Object -Last 1
    if(-not $line){throw ('QA_FAIL missing '+$re)}
    Write-Output $line.Line
}
function AwaitLog([string]$re){
    for($i=0;$i -lt 75;$i++){
        $found=Select-String -LiteralPath $log -Pattern $re | Select-Object -Last 1
        if($found){Write-Output $found.Line; return}
        Start-Sleep -Milliseconds 240
    }
    throw ('QA_TIMEOUT no '+$re)
}
# Explicitly click the map object and authorize a battle.
Click 435 377
Expect '\[AI HEROES\] FOREST selected'
Press 0x0D
AwaitLog '\[AI HEROES LIVE\] BEGIN node=FOREST'
Press 0x42 # B: speed through the same deterministic session.
AwaitLog '\[AI HEROES LIVE\] COMMIT node=FOREST'
Expect '\[AI HEROES\] LIVE RESULT: VICTORY'
Press 0x0D # Enter returns from the battle result to the map.
AwaitLog '\[AI HEROES\] Returned to map'
& (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $tag -OutputPath (Join-Path $root 'Docs\RIVAL_FOREST_CAPTURE_QA_2026-10-28.png')
# Day 2: captured forest pays +15 wood, next-day forecast shows raid.
Press 0x20
Expect '\[AI HEROES\] Day 2: income collected'
& (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $tag -OutputPath (Join-Path $root 'Docs\RIVAL_RAID_FORECAST_QA_2026-10-28.png')
# Day 3: only Forest is player-owned. NPC must raid forest for 6 wood.
Press 0x20
Expect '\[AI HEROES RIVAL\] day=3 action=2 amount=6 wood=59 ore=30 player_order=0'
Expect '\[AI HEROES\] Day 3: income collected'
& (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $tag -OutputPath (Join-Path $root 'Docs\RIVAL_RAID_APPLIED_QA_2026-10-28.png')
$after=@{}
if(Test-Path $dir){
    Get-ChildItem -LiteralPath $dir -File | ForEach-Object {
        $after[$_.FullName]=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }
}
if($before.Count -ne $after.Count){throw 'QA_FAIL: save file count changed'}
foreach($path in $before.Keys){
    if(-not $after.ContainsKey($path) -or $after[$path] -ne $before[$path]){
        throw ('QA_FAIL: save changed '+$path)
    }
}
Write-Output 'RIVAL_AUTHORIZED_CAPTURE_AND_RAID_PASS day3_wood=59 no_save_writes=1'
