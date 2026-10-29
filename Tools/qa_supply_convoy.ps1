param(
    [Parameter(Mandatory = $true)][int]$ExpectedPid,
    [string]$LogTag = 'AIHeroesSupplyQA.log'
)
# Exact-game-window smoke. No global keyboard/mouse, SaveGame writes, or
# third-party process interaction. A dev-only startup flag creates a disposable
# secured map snapshot; it is NOT normal player campaign progress.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$procInfo = Get-CimInstance Win32_Process -Filter "ProcessId=$ExpectedPid"
if(-not $procInfo -or $procInfo.Name -ne 'UnrealEditor.exe' -or
    $procInfo.CommandLine -notlike '*AIHeroes.uproject*' -or
    $procInfo.CommandLine -notlike '*-game*' -or
    $procInfo.CommandLine -notlike '*-AIHeroesSupplySmoke*' -or
    $procInfo.CommandLine -notlike "*-log=$LogTag*"){
    throw 'SUPPLY_QA_BLOCKED: PID not the dedicated AIHEROES supply fixture'
}
$game = Get-Process -Id $ExpectedPid -ErrorAction Stop
$hwnd = $game.MainWindowHandle
if($hwnd -eq [IntPtr]::Zero){throw 'SUPPLY_QA_BLOCKED: window not ready'}
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct SupplyWindowRect { public int Left,Top,Right,Bottom; }
public static class AIHSupplyWindow {
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hwnd, out SupplyWindowRect r);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hwnd, uint m, IntPtr w, IntPtr l);
}
'@
$rect=New-Object SupplyWindowRect
if(-not[AIHSupplyWindow]::GetClientRect($hwnd,[ref]$rect) -or
   $rect.Right -ne 1280 -or $rect.Bottom -ne 720){
    throw "SUPPLY_QA_BLOCKED: viewport is not 1280x720"
}
$log=Join-Path $root ('Saved\Logs\' + $LogTag)
if(-not(Test-Path -LiteralPath $log)){throw 'SUPPLY_QA_BLOCKED: game log missing'}
function SaveInventory {
    $dir=Join-Path $root 'Saved\SaveGames'
    if(-not(Test-Path -LiteralPath $dir)){return @()}
    return @(Get-ChildItem -LiteralPath $dir -File|Sort-Object Name|ForEach-Object {
        $_.Name+'|'+$_.Length+'|'+$_.LastWriteTimeUtc.Ticks+'|'+
        (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    })
}
function Until([string]$pattern,[int]$seconds=20){
    $deadline=(Get-Date).AddSeconds($seconds)
    while((Get-Date) -lt $deadline){
        $record=Select-String -LiteralPath $log -Pattern $pattern -ErrorAction SilentlyContinue |
            Select-Object -Last 1
        if($record){Write-Output $record.Line;return}
        if(-not(Get-Process -Id $ExpectedPid -ErrorAction SilentlyContinue)){
            throw 'SUPPLY_QA_FAIL: dedicated game crashed'
        }
        Start-Sleep -Milliseconds 260
    }
    throw ('SUPPLY_QA_FAIL: expected record not found: '+$pattern)
}
function Click([int]$x,[int]$y){
    $pt=[IntPtr](($y -shl 16) -bor $x)
    foreach($msg in @(0x0200,0x0201,0x0202)){
        $flags=if($msg -eq 0x0201){[IntPtr]1}else{[IntPtr]0}
        if(-not [AIHSupplyWindow]::PostMessage($hwnd,$msg,$flags,$pt)){
            throw 'SUPPLY_QA_FAIL: targeted click refused'
        }
        Start-Sleep -Milliseconds 90
    }
    Start-Sleep -Milliseconds 460
}
function Key([int]$virtualKey){
    if(-not [AIHSupplyWindow]::PostMessage($hwnd,0x0100,[IntPtr]$virtualKey,[IntPtr]0)){
        throw ('SUPPLY_QA_FAIL: targeted key down refused '+$virtualKey)
    }
    Start-Sleep -Milliseconds 155
    if(-not [AIHSupplyWindow]::PostMessage($hwnd,0x0101,[IntPtr]$virtualKey,[IntPtr]0)){
        throw ('SUPPLY_QA_FAIL: targeted key up refused '+$virtualKey)
    }
    Start-Sleep -Milliseconds 440
}
function Capture([string]$name){
    # Each dedicated log owns its own screenshot directory; preserve old QA images.
    $path=Join-Path $root ('Saved\QA\SupplyConvoy\' +
        [IO.Path]::GetFileNameWithoutExtension($LogTag) + '\' + $name)
    & (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $LogTag -OutputPath $path
    if(-not(Test-Path -LiteralPath $path) -or
       (Get-Item -LiteralPath $path).Length -lt 10000){
        throw ('SUPPLY_QA_FAIL: screenshot unavailable: '+$path)
    }
    $f=Get-Item -LiteralPath $path
    Write-Output ('SUPPLY_SCREENSHOT='+$f.FullName+' bytes='+$f.Length+
        ' sha256='+(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash)
}
Write-Output "SUPPLY_QA_PINNED pid=$ExpectedPid 1280x720 no_player_save_writes=1"
$saveBefore=@(SaveInventory)
Until '\[AI HEROES SUPPLY SMOKE\] READY source=FOREST stock=10 gold=240 moves=4'
Capture 'CONVOY_BEFORE.png'
# 205: the new shared touch/mouse control at desktop (x=716..844, y=461..490).
Key 0x58
# Keyboard-bound X does not create a HUD mouse-action log; verify the quote itself.
Until '\[AI HEROES SUPPLY\] QUOTE source=FOREST CAMP units=5 cost=65G/5W/0O moves=1 player_order=0 commit=0'
if(@(Select-String -LiteralPath $log -Pattern '\[AI HEROES SUPPLY\] DISPATCH').Count -ne 0){
    throw 'SUPPLY_QA_FAIL: first action spent resources'
}
Capture 'CONVOY_QUOTE.png'
# Esc cancels even a valid quote. This MUST NOT charge currency or send troops.
Key 0x1B
Until '\[AI HEROES SUPPLY\] CANCEL player_order=1 commit=0'
if(@(Select-String -LiteralPath $log -Pattern '\[AI HEROES SUPPLY\] DISPATCH').Count -ne 0){
    throw 'SUPPLY_QA_FAIL: Esc committed the quoted convoy'
}
Capture 'CONVOY_CANCELED.png'
# First X after Esc must quote again, not unexpectedly buy the prior offer.
Key 0x58
$firstRequote=@(Select-String -LiteralPath $log -Pattern '\[AI HEROES SUPPLY\] QUOTE source=FOREST CAMP').Count
if($firstRequote -ne 2){
    throw ('SUPPLY_QA_FAIL: canceled quote did not require fresh preview: '+$firstRequote)
}
if(@(Select-String -LiteralPath $log -Pattern '\[AI HEROES SUPPLY\] DISPATCH').Count -ne 0){
    throw 'SUPPLY_QA_FAIL: first X after Esc incorrectly committed'
}
Capture 'CONVOY_REQUOTED.png'
Key 0x58
Until '\[AI HEROES SUPPLY\] DISPATCH source=FOREST CAMP units=5 cost=65G/5W/0O moves=1 remaining=5 player_order=1 commit=1'
Capture 'CONVOY_DELIVERED.png'
# One more click should only create a NEW quote. Cancel the second quote too.
Key 0x58
$quotes=@(Select-String -LiteralPath $log -Pattern '\[AI HEROES SUPPLY\] QUOTE source=FOREST CAMP').Count
$commits=@(Select-String -LiteralPath $log -Pattern '\[AI HEROES SUPPLY\] DISPATCH source=FOREST CAMP').Count
if($quotes -ne 3 -or $commits -ne 1){
    throw ('SUPPLY_QA_FAIL: replay/confirmation errors quotes='+$quotes+' commits='+$commits)
}
Key 0x1B
$cancels=@(Select-String -LiteralPath $log -Pattern '\[AI HEROES SUPPLY\] CANCEL player_order=1 commit=0').Count
if($cancels -ne 2){
    throw ('SUPPLY_QA_FAIL: Esc cancellation count must be 2, actual='+$cancels)
}
if(@(Select-String -LiteralPath $log -Pattern '\[AI HEROES SUPPLY\] DISPATCH source=FOREST CAMP').Count -ne 1){
    throw 'SUPPLY_QA_FAIL: a second Esc duplicated the delivery'
}
$saveAfter=@(SaveInventory)
if(($saveBefore -join "`n") -ne ($saveAfter -join "`n")){
    throw 'SUPPLY_QA_FAIL: real SaveGame files mutated'
}
Write-Output 'SUPPLY_GUI_PASS: HQ -> quote read-only -> Esc cancels -> X requotes -> X delivers one pack -> X requotes -> Esc cancels'
Write-Output 'SUPPLY_SAVEGAMES_UNCHANGED_PASS'
Write-Output ('SUPPLY_QA_LOG='+$log)
