# Playback only: no SaveGame writes or global keyboard input.
$ErrorActionPreference = 'Stop'
$root = 'C:\github\AIHEROES'
$log = Join-Path $root 'Saved\Logs\AIHeroesHotkeyQA.log'
$save = Join-Path $root 'Saved\SaveGames\AIHEROES_Campaign_Resilient_v1_Player2_A.sav'
if (-not(Test-Path $save)) {throw 'QA_BLOCKED: last-battle slot 2 is not present'}
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class LitReplayQA {
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
}
'@
$c=@(Get-CimInstance Win32_Process | Where-Object {
    $_.Name -eq 'UnrealEditor.exe' -and $_.CommandLine -like '*AIHeroes.uproject*' -and
    $_.CommandLine -like '*-log=AIHeroesHotkeyQA.log*'
})
if ($c.Count -ne 1) {throw "QA_BLOCKED: expected one game"}
$g=Get-Process -Id $c[0].ProcessId -ErrorAction Stop
if ($g.MainWindowHandle -eq [IntPtr]::Zero) {throw 'Game has no window'}
function Hit([int]$key) {
    $down=[LitReplayQA]::PostMessage($g.MainWindowHandle,0x100,[IntPtr]$key,[IntPtr]0)
    Start-Sleep -Milliseconds 120
    $up=[LitReplayQA]::PostMessage($g.MainWindowHandle,0x101,[IntPtr]$key,[IntPtr]0)
    if (-not ($down -and $up)) { throw "Input error" }
    Start-Sleep -Milliseconds 340
}
function Expect([string]$pattern) {
    $line = Select-String -LiteralPath $log -Pattern $pattern | Select-Object -Last 1
    if(-not $line){ throw "QA_FAIL: missing $pattern" }
    Write-Output $line.Line
}
Hit 0x43  # C: current slot 3 -> slot 1
Hit 0x43  # C: slot 1 -> slot 2
Expect 'SAVE SLOT 2/3 selected'
Hit 0x4C # L loads verified battle
Expect 'GAME LOADED SLOT 2: day 2'
Hit 0x56 # V starts verified 3D replay
Expect '\[AI HEROES 3D\] START'
Start-Sleep -Milliseconds 800
Expect '\[AI HEROES REPLAY\] 1/'
& (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag 'AIHeroesHotkeyQA.log' -OutputPath (Join-Path $root 'Saved\Screenshots\AIHeroes3DReplayLitQA.png')
Hit 0x1B # ESC returns to map
Expect '\[AI HEROES 3D\] SKIP'
Start-Sleep -Milliseconds 450
Expect '\[AI HEROES 3D\] FINISH'
$debug = @(Select-String -LiteralPath $log -Pattern 'Set new viewmode: ShaderComplexity')
if ($debug.Count -ne 0){ throw "QA_FAIL: debug render mode enabled" }
Write-Output '3D_REPLAY_LIT_MODE_AND_CAMERA_RESTORE_PASS'
