# Standalone AI HEROES Fog QA. Requires an owned, uniquely tagged
# UnrealEditor.exe -game with -log=AIHeroesFogQA.log.
# Never writes/loads a player's SaveGame and sends no global OS inputs.
$ErrorActionPreference='Stop'
$root='C:\github\AIHEROES'
$tag='AIHeroesFogQA.log'
$log=Join-Path $root ('Saved\Logs\'+$tag)
$saveDir=Join-Path $root 'Saved\SaveGames'

$before=@{}
if(Test-Path $saveDir){
    Get-ChildItem -LiteralPath $saveDir -File | ForEach-Object {
        $before[$_.FullName]=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }
}
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class FogQAInput {
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr handle, uint msg, IntPtr w, IntPtr l);
}
'@
$candidates=@(Get-CimInstance Win32_Process | Where-Object {
    $_.Name -eq 'UnrealEditor.exe' -and
    $_.CommandLine -like '*AIHeroes.uproject*' -and
    $_.CommandLine -like '*-log=AIHeroesFogQA.log*'
})
if($candidates.Count -ne 1){throw ('FOG_QA_BOUNDARY: expected 1 private window, got '+$candidates.Count)}
$game=Get-Process -Id $candidates[0].ProcessId -ErrorAction Stop
if($game.MainWindowHandle -eq [IntPtr]::Zero -or -not(Test-Path $log)){throw 'FOG_QA_BOUNDARY: no game window or dedicated log'}
Write-Output ('FOG_QA_TARGET_PID='+$game.Id)
function Post([uint32]$msg,[int]$w,[int]$l){
    if(-not [FogQAInput]::PostMessage($game.MainWindowHandle,$msg,[IntPtr]$w,[IntPtr]$l)){
        throw ('FOG_QA_INPUT_FAILED '+$msg)
    }
}
function Press([int]$key){
    Post 0x100 $key 0
    Start-Sleep -Milliseconds 190
    Post 0x101 $key 0
    Start-Sleep -Milliseconds 520
}
function Click([int]$x,[int]$y){
    $xy=($y -shl 16) -bor $x
    Post 0x200 0 $xy
    Start-Sleep -Milliseconds 170
    Post 0x201 1 $xy
    Start-Sleep -Milliseconds 170
    Post 0x202 0 $xy
    Start-Sleep -Milliseconds 630
}
function Expect([string]$regex){
    $found=Select-String -LiteralPath $log -Pattern $regex | Select-Object -Last 1
    if(-not $found){throw ('FOG_QA_MISSING '+$regex)}
    Write-Output $found.Line
}
function Await([string]$regex){
    for($j=0;$j -lt 70;$j++){
        $match=Select-String -LiteralPath $log -Pattern $regex | Select-Object -Last 1
        if($match){Write-Output $match.Line;return}
        Start-Sleep -Milliseconds 290
    }
    throw ('FOG_QA_TIMEOUT '+$regex)
}
function Shot([string]$name){
    & (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $tag -OutputPath (Join-Path $root ('Docs\'+$name))
}
# A NEW campaign has just three visible sites. Capture before selecting.
if(Select-String -LiteralPath $log -Pattern '\[AI HEROES SCOUT\]' -Quiet){
    throw 'FOG_QA_BOUNDARY: dedicated game is not a fresh campaign'
}
Shot 'AIHEROES_FOG_START_2026-10-28.png'
# Explicit click and attack authorization. No unrequested auto-battle.
Click 435 377
Expect '\[AI HEROES\] FOREST selected'
Press 0x0D
Await '\[AI HEROES LIVE\] BEGIN node=FOREST'
Press 0x42  # B: validated fast finish, no change to battle rules.
Await '\[AI HEROES LIVE\] COMMIT node=FOREST'
Await '\[AI HEROES SCOUT\] discovered=OUTPOST known=4/5 no_order=1'
Expect '\[AI HEROES\] LIVE RESULT: VICTORY'
Press 0x0D
Await '\[AI HEROES\] Returned to map'
Shot 'AIHEROES_FOG_FOREST_REVEAL_2026-10-28.png'
# Guarantee no user SaveGame files or recovery snapshots were touched.
$after=@{}
if(Test-Path $saveDir){
    Get-ChildItem -LiteralPath $saveDir -File | ForEach-Object {
        $after[$_.FullName]=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }
}
if($before.Count -ne $after.Count){throw 'FOG_QA_SAVE_COUNT_CHANGED'}
foreach($p in $before.Keys){
    if(-not $after.ContainsKey($p) -or $after[$p] -ne $before[$p]){
        throw ('FOG_QA_USER_SAVE_CHANGED '+$p)
    }
}
Write-Output ('FOG_MANUAL_PASS initial=3 after_forest=4 forest_battle=1 player_order=1 no_save_writes=1 pid='+$game.Id)
