param(
    [Parameter(Mandatory = $true)][int]$ExpectedPid,
    [string]$LogTag = 'AIHeroesStockQA.log'
)
# Exact-window player-protocol smoke. Never broadcast OS input, save a
# human campaign, or close an unverified Unreal/VS Code process.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$target = Get-CimInstance Win32_Process -Filter "ProcessId=$ExpectedPid"
if(-not $target -or $target.Name -ne 'UnrealEditor.exe' -or
   $target.CommandLine -notlike '*AIHeroes.uproject*' -or
   $target.CommandLine -notlike '*-game*' -or
   $target.CommandLine -notlike "*-log=$LogTag*") {
    throw 'STOCK_QA_BLOCKED: the expected PID is not our isolated QA game'
}
$game = Get-Process -Id $ExpectedPid -ErrorAction Stop
if($game.MainWindowHandle -eq [IntPtr]::Zero){
    throw 'STOCK_QA_BLOCKED: game has no input window'
}
$log = Join-Path $root ('Saved\Logs\' + $LogTag)
if(-not (Test-Path -LiteralPath $log)){
    throw 'STOCK_QA_BLOCKED: dedicated log is missing'
}
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct RecruitClientRect {public int Left,Top,Right,Bottom;}
public static class RecruitQAInput {
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr window,out RecruitClientRect rect);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window,uint msg,IntPtr w,IntPtr l);
}
'@
$dimensions=New-Object RecruitClientRect
if(-not [RecruitQAInput]::GetClientRect(
        $game.MainWindowHandle,[ref]$dimensions) -or
    $dimensions.Right -ne 1280 -or $dimensions.Bottom -ne 720) {
    throw "STOCK_QA_BLOCKED: unexpected viewport $($dimensions.Right)x$($dimensions.Bottom)"
}
function Inventory {
    $dir=Join-Path $root 'Saved\SaveGames'
    if(-not (Test-Path $dir)){return @()}
    return @(Get-ChildItem -LiteralPath $dir -File|Sort-Object Name|ForEach-Object {
        $_.Name+'|'+$_.Length+'|'+$_.LastWriteTimeUtc.Ticks+'|'+
        (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    })
}
$before=@(Inventory)
function Key([int]$vk) {
    if(-not [RecruitQAInput]::PostMessage(
            $game.MainWindowHandle,0x0100,[IntPtr]$vk,[IntPtr]0)) {
        throw "STOCK_QA_FAIL: WM_KEYDOWN $vk was refused"
    }
    Start-Sleep -Milliseconds 150
    if(-not [RecruitQAInput]::PostMessage(
            $game.MainWindowHandle,0x0101,[IntPtr]$vk,[IntPtr]0)) {
        throw "STOCK_QA_FAIL: WM_KEYUP $vk was refused"
    }
    Start-Sleep -Milliseconds 360
}
function ExpectLog([string]$pattern,[int]$seconds=17) {
    $end=(Get-Date).AddSeconds($seconds)
    while((Get-Date) -lt $end){
        $found=Select-String -LiteralPath $log -Pattern $pattern -ErrorAction SilentlyContinue |
            Select-Object -Last 1
        if($found){Write-Output $found.Line;return}
        if(-not (Get-Process -Id $ExpectedPid -ErrorAction SilentlyContinue)){
            throw 'STOCK_QA_FAIL: test window crashed'
        }
        Start-Sleep -Milliseconds 280
    }
    throw ('STOCK_QA_FAIL: expected game event missing: '+$pattern)
}
function Capture([string]$name){
    $full=Join-Path $root ('Saved\QA\RecruitmentStock\' + $name)
    & (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $LogTag -OutputPath $full
    if(-not(Test-Path -LiteralPath $full) -or (Get-Item $full).Length -lt 10000){
        throw ('STOCK_QA_FAIL: missing screenshot '+$name)
    }
}
Write-Output "STOCK_QA_PINNED pid=$ExpectedPid viewport=1280x720"
# Direct player keyboard commands, same handlers as UI/touch BASE cards.
Key 0x31
ExpectLog 'Recruited 5 militia at HQ\. Unpurchased militia: 5'
Key 0x31
ExpectLog 'Recruited 5 militia at HQ\. Unpurchased militia: 0'
Key 0x31
ExpectLog 'RECRUIT BLOCKED: only 0 units available, need 5'
Key 0x32
ExpectLog 'Recruited 2 scouts at HQ\. Unpurchased scouts: 2'
Key 0x32
ExpectLog 'Recruited 2 scouts at HQ\. Unpurchased scouts: 0'
Key 0x32
ExpectLog 'RECRUIT BLOCKED: only 0 units available, need 2'
# BASE must expose both unavailable offers without changing resources.
Key 0x47
ExpectLog '\[AI HEROES BASE PANEL\] OPEN no_order=1'
Capture 'STOCK_EXHAUSTED_DAY1.png'
Key 0x47
ExpectLog '\[AI HEROES BASE PANEL\] CLOSE pending_discarded=1'

# Seven player-authorized day advances. Weekly growth only at 7 -> 8.
for($i=2;$i -le 8;$i++) {
    Key 0x20
    ExpectLog ("\[AI HEROES\] Day "+$i+": income collected")
}
ExpectLog '\[AI HEROES WEEK\] week=2 day=8 neutral=[0-9]+ militia_stock_plus=5 scout_stock_plus=2 stock=5/2'
Key 0x47
ExpectLog '\[AI HEROES BASE PANEL\] OPEN no_order=1'
Capture 'STOCK_NEW_WEEK_DAY8.png'
Key 0x47
ExpectLog '\[AI HEROES BASE PANEL\] CLOSE pending_discarded=1'
Key 0x31
ExpectLog 'Recruited 5 militia at HQ\. Unpurchased militia: 0'
Key 0x32
ExpectLog 'Recruited 2 scouts at HQ\. Unpurchased scouts: 0'
if(@(Select-String -LiteralPath $log -Pattern '\[AI HEROES LIVE\] COMMIT').Count -ne 0) {
    throw 'STOCK_QA_FAIL: recruiting/day cycle must never commit a battle'
}
if(Compare-Object -ReferenceObject $before -DifferenceObject @(Inventory)){
    throw 'STOCK_QA_FAIL: human SaveGame file changed during this test'
}
Write-Output 'STOCK_QA_PASS: two packs each -> stock refused -> day8 growth -> buy once more'
Write-Output 'STOCK_SAVES_UNCHANGED_PASS'
Write-Output "STOCK_QA_LOG=$log"
