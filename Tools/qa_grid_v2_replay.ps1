param(
    [Parameter(Mandatory=$true)][int]$ExpectedPid,
    [string]$LogTag = 'AIHeroesGridReplayQA.log'
)
# Explicit 3D replay smoke. Window-targeted input ONLY; no foreground
# activation, real user saves or map writes. Refuses non-test game windows.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$proc = Get-CimInstance Win32_Process -Filter "ProcessId=$ExpectedPid"
if (-not $proc -or $proc.Name -ne 'UnrealEditor.exe' -or
    $proc.CommandLine -notlike '*AIHeroes.uproject*' -or
    $proc.CommandLine -notlike '*-game*' -or
    $proc.CommandLine -notlike "*-log=$LogTag*") {
    throw 'GRID_REPLAY_QA_BLOCKED: exact dedicated game identity mismatch'
}
$game = Get-Process -Id $ExpectedPid -ErrorAction Stop
if ($game.MainWindowHandle -eq [IntPtr]::Zero) {
    throw 'GRID_REPLAY_QA_BLOCKED: game HWND is not ready'
}
$log = Join-Path $root ('Saved\Logs\' + $LogTag)
if (-not (Test-Path -LiteralPath $log)) {
    throw 'GRID_REPLAY_QA_BLOCKED: expected dedicated game log missing'
}

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct V2ClientRect { public int Left,Top,Right,Bottom; }
public static class V2QAInput {
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hwnd,out V2ClientRect result);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hwnd,uint msg,IntPtr w,IntPtr l);
}
'@
$viewport = New-Object V2ClientRect
if (-not [V2QAInput]::GetClientRect($game.MainWindowHandle,[ref]$viewport) -or
    $viewport.Right -ne 1280 -or $viewport.Bottom -ne 720) {
    throw "GRID_REPLAY_QA_BLOCKED: unexpected viewport $($viewport.Right)x$($viewport.Bottom)"
}

function SaveInventory {
    $dir = Join-Path $root 'Saved\SaveGames'
    if (-not (Test-Path -LiteralPath $dir)) {return @()}
    return @(Get-ChildItem $dir -File | Sort-Object Name | ForEach-Object {
        $_.Name + '|' + $_.Length + '|' + $_.LastWriteTimeUtc.Ticks + '|' +
        (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    })
}
$originalSaves = @(SaveInventory)

function SendKey([int]$virtualKey) {
    $target = $game.MainWindowHandle
    if (-not [V2QAInput]::PostMessage($target,0x0100,[IntPtr]$virtualKey,[IntPtr]0)) {
        throw 'GRID_REPLAY_QA_FAIL: target rejected WM_KEYDOWN'
    }
    Start-Sleep -Milliseconds 170
    if (-not [V2QAInput]::PostMessage($target,0x0101,[IntPtr]$virtualKey,[IntPtr]0)) {
        throw 'GRID_REPLAY_QA_FAIL: target rejected WM_KEYUP'
    }
    Start-Sleep -Milliseconds 350
}

function Click([int]$x,[int]$y) {
    $xy=[IntPtr](($y -shl 16) -bor $x)
    if(-not [V2QAInput]::PostMessage($game.MainWindowHandle,0x0200,[IntPtr]::Zero,$xy)) {
        throw 'GRID_REPLAY_QA_FAIL: target rejected WM_MOUSEMOVE'
    }
    Start-Sleep -Milliseconds 120
    if(-not [V2QAInput]::PostMessage($game.MainWindowHandle,0x0201,[IntPtr]1,$xy)) {
        throw 'GRID_REPLAY_QA_FAIL: target rejected WM_LBUTTONDOWN'
    }
    Start-Sleep -Milliseconds 120
    if(-not [V2QAInput]::PostMessage($game.MainWindowHandle,0x0202,[IntPtr]::Zero,$xy)) {
        throw 'GRID_REPLAY_QA_FAIL: target rejected WM_LBUTTONUP'
    }
    Start-Sleep -Milliseconds 500
}

function WaitFor([string]$pattern,[int]$seconds=16) {
    $deadline=(Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        $found=Select-String -LiteralPath $log -Pattern $pattern | Select-Object -Last 1
        if ($found) {Write-Output $found.Line; return}
        Start-Sleep -Milliseconds 300
        if (-not (Get-Process -Id $ExpectedPid -ErrorAction SilentlyContinue)) {
            throw ('GRID_REPLAY_QA_FAIL: game process exited while waiting for '+$pattern)
        }
    }
    throw ('GRID_REPLAY_QA_FAIL: missing log evidence '+$pattern)
}

function Capture([string]$filename) {
    $path=Join-Path $root ('Saved\QA\GridV2Replay\' + $filename)
    & (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $LogTag -OutputPath $path
    if (-not (Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).Length -lt 10000) {
        throw ('GRID_REPLAY_QA_FAIL: empty screenshot '+$filename)
    }
}

Write-Output "GRID_REPLAY_QA_WINDOW_PINNED pid=$ExpectedPid viewport=1280x720"
Click 435 377
WaitFor '\[AI HEROES\] FOREST selected'
SendKey 0x0D # Enter: player expressly authorizes an actual grid battle.
WaitFor '\[AI HEROES LIVE\] BEGIN node=FOREST actions=0'
SendKey 0x42 # B: fast-finish the current real live session.
WaitFor '\[AI HEROES LIVE\] COMMIT node=FOREST'
SendKey 0x0D # Return to strategic map, preserving last battle record.
WaitFor '\[AI HEROES\] Returned to map\. Battle result already applied once'
$commitCountBefore = @(Select-String -LiteralPath $log -Pattern '\[AI HEROES LIVE\] COMMIT node=FOREST').Count
if ($commitCountBefore -ne 1) {throw "GRID_REPLAY_QA_FAIL: expected one commit, found $commitCountBefore"}

SendKey 0x56 # V: open recorded V2 scene, NOT a new battle.
WaitFor '\[AI HEROES GRID REPLAY\] START node=FOREST verified_actions=[1-9][0-9]* campaign_commit=0'
WaitFor '\[AI HEROES GRID REPLAY\] 1/'
Capture 'GRID_REPLAY_V2_START.png'
SendKey 0x20 # Space: replay pause, not end turn.
WaitFor '\[AI HEROES GRID REPLAY\] PAUSED'
SendKey 0x54 # T: playback speed, not combat simulation speed.
WaitFor '\[AI HEROES GRID REPLAY\] SPEED 2x'
Capture 'GRID_REPLAY_V2_PAUSED.png'
SendKey 0x20
WaitFor '\[AI HEROES GRID REPLAY\] RESUMED'
SendKey 0x1B # Escape: skip replay and restore camera.
WaitFor '\[AI HEROES GRID REPLAY\] FINISH camera_restored=1 campaign_unchanged=1'
Capture 'GRID_REPLAY_V2_RETURN.png'

$commitCountAfter = @(Select-String -LiteralPath $log -Pattern '\[AI HEROES LIVE\] COMMIT node=FOREST').Count
if ($commitCountAfter -ne 1) {
    throw ('GRID_REPLAY_QA_FAIL: replay committed the battle twice: '+$commitCountAfter)
}
$afterSaves = @(SaveInventory)
if (Compare-Object -ReferenceObject $originalSaves -DifferenceObject $afterSaves) {
    throw 'GRID_REPLAY_QA_FAIL: user save files changed without authorization'
}
Write-Output 'GRID_REPLAY_REAL_WINDOW_PASS: live battle -> commit once -> verified V2 replay -> pause/speed/skip -> camera restore'
Write-Output 'GRID_REPLAY_SAVES_UNCHANGED_PASS'
Write-Output "GRID_REPLAY_LOG=$log"
