param(
    [Parameter(Mandatory = $true)][int]$ExpectedPid,
    [string]$LogTag = 'AIHeroesFieldDwellingQA.log'
)
# A bounded 1280x720 exact-window smoke. No global mouse/keyboard events;
# all storage writes are test-owned screenshot/log receipts, NEVER SaveGame.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$processInfo = Get-CimInstance Win32_Process -Filter "ProcessId=$ExpectedPid"
if(-not $processInfo -or $processInfo.Name -ne 'UnrealEditor.exe' -or
    $processInfo.CommandLine -notlike '*AIHeroes.uproject*' -or
    $processInfo.CommandLine -notlike '*-game*' -or
    $processInfo.CommandLine -notlike "*-log=$LogTag*"){
    throw 'FIELD_QA_BLOCKED: PID is not the dedicated AIHEROES field QA game'
}
$game = Get-Process -Id $ExpectedPid -ErrorAction Stop
$hwnd = $game.MainWindowHandle
if($hwnd -eq [IntPtr]::Zero){throw 'FIELD_QA_BLOCKED: no game HWND'}
$log = Join-Path $root ('Saved\Logs\' + $LogTag)
if(-not(Test-Path -LiteralPath $log)){throw 'FIELD_QA_BLOCKED: dedicated log missing'}
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct FieldWindowRect {public int Left,Top,Right,Bottom;}
public static class AIHFieldWindow {
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hwnd, out FieldWindowRect rect);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hwnd, uint msg, IntPtr w, IntPtr l);
}
'@
$rect = New-Object FieldWindowRect
if(-not [AIHFieldWindow]::GetClientRect($hwnd,[ref]$rect) -or
   $rect.Right -ne 1280 -or $rect.Bottom -ne 720){
    throw "FIELD_QA_BLOCKED: viewport changed: $($rect.Right)x$($rect.Bottom)"
}
function SaveInventory {
    $dir=Join-Path $root 'Saved\SaveGames'
    if(-not (Test-Path -LiteralPath $dir)){return @()}
    return @(Get-ChildItem -LiteralPath $dir -File|Sort-Object Name|ForEach-Object {
        $_.Name+'|'+$_.Length+'|'+$_.LastWriteTimeUtc.Ticks+'|'+
        (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    })
}
$before=@(SaveInventory)
function Key([int]$key){
    if(-not[AIHFieldWindow]::PostMessage($hwnd,0x0100,[IntPtr]$key,[IntPtr]0)){
        throw ('FIELD_QA_FAIL: key down refused '+$key)
    }
    Start-Sleep -Milliseconds 150
    if(-not[AIHFieldWindow]::PostMessage($hwnd,0x0101,[IntPtr]$key,[IntPtr]0)){
        throw ('FIELD_QA_FAIL: key up refused '+$key)
    }
    Start-Sleep -Milliseconds 440
}
function Click([int]$x,[int]$y){
    $pt=[IntPtr](($y -shl 16) -bor $x)
    foreach($msg in @(0x0200,0x0201,0x0202)){
        $flags=if($msg -eq 0x0201){[IntPtr]1}else{[IntPtr]0}
        if(-not[AIHFieldWindow]::PostMessage($hwnd,$msg,$flags,$pt)){
            throw 'FIELD_QA_FAIL: targeted click rejected'
        }
        Start-Sleep -Milliseconds 110
    }
    Start-Sleep -Milliseconds 440
}
function Until([string]$pattern,[int]$seconds=20) {
    $end=(Get-Date).AddSeconds($seconds)
    while((Get-Date) -lt $end) {
        $x=Select-String -LiteralPath $log -Pattern $pattern -ErrorAction SilentlyContinue |
            Select-Object -Last 1
        if($x){Write-Output $x.Line;return}
        if(-not(Get-Process -Id $ExpectedPid -ErrorAction SilentlyContinue)){
            throw 'FIELD_QA_FAIL: isolated game crashed'
        }
        Start-Sleep -Milliseconds 250
    }
    throw ('FIELD_QA_FAIL: expected record not found: '+$pattern)
}
function Capture([string]$name){
    $p=Join-Path $root ('Saved\QA\FieldDwellings\' + $name)
    & (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $LogTag -OutputPath $p
    if(-not(Test-Path -LiteralPath $p) -or (Get-Item $p).Length -lt 10000){
        throw ('FIELD_QA_FAIL: empty or missing screenshot: '+$name)
    }
}
Write-Output "FIELD_QA_PINNED pid=$ExpectedPid size=1280x720"
# Main HUD FIELD 3 button should be clickable but cannot claim at HQ.
Click 913 475
Until '\[AI HEROES UI\] Button action=204'
Until 'FIELD: select an existing FOREST or ORE site first. Nothing spent.'
# Click the only revealed nearby FOREST node, as in previous exact-window QA.
Click 435 377
Until '\[AI HEROES\] FOREST selected'
Key 0x0D # explicit human battle authorization
Until '\[AI HEROES LIVE\] BEGIN node=FOREST'
Key 0x42 # fast finish only the already-authorized battle
Until '\[AI HEROES LIVE\] COMMIT node=FOREST'
Key 0x0D # return from result -> original map
Until '\[AI HEROES\] Returned to map'

# Claim the separate field camp by clicking the new shared mouse/touch button.
Click 913 475
Until '\[AI HEROES UI\] Button action=204'
Until '\[AI HEROES FIELD\] CLAIM site=FOREST CAMP stock=5 player_order=1'
Capture 'FIELD_FOREST_CLAIMED.png'
Key 0x33 # separate player authorization buys one pack from local stock
Until '\[AI HEROES FIELD\] RECRUIT site=FOREST CAMP remaining=0'
Key 0x33
Until 'FIELD RECRUIT BLOCKED: only 0/5 units at this site'
$commits=@(Select-String -LiteralPath $log -Pattern '\[AI HEROES LIVE\] COMMIT node=FOREST').Count
if($commits -ne 1){throw ('FIELD_QA_FAIL: unexpected combat commit count '+$commits)}
for($day=2;$day -le 8;$day++){
    Key 0x20 # player-authorized NEXT DAY
    Until ("\[AI HEROES\] Day "+$day+": income collected")
}
Until '\[AI HEROES FIELD\] WEEK day=8 forest_plus=5 forest_stock=5 forest_claimed=1 ore_plus=0 ore_stock=2 ore_claimed=0'
Capture 'FIELD_FOREST_WEEK2.png'
Key 0x33
Until '\[AI HEROES FIELD\] RECRUIT site=FOREST CAMP remaining=0'
if(@(Select-String -LiteralPath $log -Pattern '\[AI HEROES LIVE\] COMMIT node=FOREST').Count -ne 1){
    throw 'FIELD_QA_FAIL: local recruiting replayed real battle'
}
$after=@(SaveInventory)
if(Compare-Object -ReferenceObject $before -DifferenceObject $after){
    throw 'FIELD_QA_FAIL: pre-existing player saves changed'
}
Write-Output 'FIELD_QA_PASS: guarded forest -> human capture -> local pack -> stock exhaustion -> day8 accrual -> local pack'
Write-Output 'FIELD_SAVES_UNCHANGED_PASS'
Write-Output "FIELD_QA_LOG=$log"
