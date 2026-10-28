param(
    [Parameter(Mandatory=$true)][int]$ExpectedPid,
    [ValidateSet('Frame','Exercise','Capture')][string]$Mode = 'Capture'
)
# Only the explicitly owned fresh QA window; never global input or saves.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$tag = 'AIHeroesHQQA.log'
$log = Join-Path $root ('Saved\Logs\' + $tag)
$proc = Get-CimInstance Win32_Process -Filter "ProcessId=$ExpectedPid"
if (-not $proc -or $proc.Name -ne 'UnrealEditor.exe' -or
    $proc.CommandLine -notlike '*C:\github\AIHEROES\AIHeroes.uproject*' -or
    $proc.CommandLine -notlike "*-log=$tag*") { throw 'QA target identity mismatch; no input sent' }
$game = Get-Process -Id $ExpectedPid
if ($game.MainWindowHandle -eq [IntPtr]::Zero -or -not (Test-Path $log)) { throw 'QA window/log unavailable' }
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct HQClientRect {public int Left,Top,Right,Bottom;}
public static class HQInput {
 [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h,out HQClientRect r);
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h,uint msg,IntPtr w,IntPtr l);
}
'@
$r = New-Object HQClientRect
if (-not [HQInput]::GetClientRect($game.MainWindowHandle,[ref]$r) -or
    $r.Right -ne 1280 -or $r.Bottom -ne 720) { throw 'Unexpected client size; no input sent' }
function Key([int]$Code,[int]$Hold=180) {
    if(-not [HQInput]::PostMessage($game.MainWindowHandle,0x100,[IntPtr]$Code,[IntPtr]0)){throw 'Key down failed'}
    try { Start-Sleep -Milliseconds $Hold }
    finally { if(-not [HQInput]::PostMessage($game.MainWindowHandle,0x101,[IntPtr]$Code,[IntPtr]0)){throw 'Key up failed'} }
    Start-Sleep -Milliseconds 400
}
function Click([int]$X,[int]$Y) {
    $at=[IntPtr](($Y -shl 16) -bor $X)
    foreach($m in @(0x200,0x201,0x202)) {
        $w=if($m -eq 0x201){[IntPtr]1}else{[IntPtr]0}
        if(-not [HQInput]::PostMessage($game.MainWindowHandle,$m,$w,$at)){throw 'Click failed'}
        Start-Sleep -Milliseconds 120
    }
    Start-Sleep -Milliseconds 400
}
function Expect([string]$Pattern) {
    $match=Select-String -LiteralPath $log -Pattern $Pattern | Select-Object -Last 1
    if(-not $match){throw ('Missing QA evidence: '+$Pattern)}
    Write-Output $match.Line
}
function Capture([string]$Name) {
    & (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $tag `
        -OutputPath (Join-Path $root ('Saved\QA\BaseDevelopment\'+$Name+'.png'))
}
if($Mode -eq 'Frame') {
    Key 0x53 1500 # S: pan backward to show headquarters above the lower HUD.
    Capture 'HQ_before'
} elseif($Mode -eq 'Exercise') {
    # Re-running purchases on an old scenario must fail instead of matching stale logs.
    if (Select-String -LiteralPath $log -Pattern 'HQ UPGRADED L[123]' -Quiet) {
        throw 'Exercise requires a fresh dedicated campaign; no keys sent.'
    }
    # Coordinates must be reviewed in HQ_before.png before invoking this mode.
    Click 640 360
    Expect '\[AI HEROES\] HQ selected'
    Key 0x55
    Expect 'HQ UPGRADED L1:.*60 gold'
    Capture 'HQ_level1'
    Key 0x20
    Expect 'Day 2: income collected \(\+60 gold\)'
    Click 860 681 # Independent HQ + button (action 201), not the model slot.
    Expect '\[AI HEROES UI\] Button action=201'
    Expect 'HQ UPGRADED L2:.*80 gold'
    Key 0x55
    Expect 'HQ needs 300 gold / 35 wood / 25 ore. Nothing spent.'
    Key 0x20
    Expect 'Day 3: income collected \(\+80 gold\)'
    Capture 'HQ_level2'
    Write-Output 'HQ_REAL_WINDOW_PASS keyboard+mouse+income+insufficient_funds; no save commands sent'
} else { Capture 'HQ_current' }
