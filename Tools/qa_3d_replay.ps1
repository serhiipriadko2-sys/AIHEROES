param([Parameter(Mandatory=$true)][int]$TargetProcessId)

# Exact PID / window-specific QA. No foreground focus and no global mouse events.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct ReplayClientRect { public int Left; public int Top; public int Right; public int Bottom; }
public static class ReplayWindowMessages {
 [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out ReplayClientRect rect);
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out ReplayClientRect rect);
 [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
}
'@
$game=Get-Process -Id $TargetProcessId -ErrorAction Stop
$cmd=(Get-CimInstance Win32_Process -Filter "ProcessId=$TargetProcessId").CommandLine
if ($game.MainWindowHandle -eq [IntPtr]::Zero -or
    $game.MainWindowTitle -notlike '*AIHeroes*' -or
    $cmd -notmatch 'AIHeroes[.]uproject' -or
    $cmd -notmatch '-game' -or
    $cmd -notmatch 'AIHeroesReplayQA[.]log') {
 throw 'QA_BLOCKED: exact PID does not match the selected QA game'
}
$rect=New-Object ReplayClientRect
if (-not [ReplayWindowMessages]::GetClientRect($game.MainWindowHandle,[ref]$rect) -or
    $rect.Right -ne 1280 -or $rect.Bottom -ne 720) {
 throw 'QA_BLOCKED: exact QA window has wrong dimensions'
}
$log='C:\github\AIHEROES\Saved\Logs\AIHeroesReplayQA.log'
if (-not (Test-Path $log)) {throw 'QA_BLOCKED: game log missing'}

function ClickGame([int]$x,[int]$y) {
 $point=[IntPtr](($y -shl 16) -bor $x)
 $a=[ReplayWindowMessages]::PostMessage($game.MainWindowHandle,0x0200,[IntPtr]::Zero,$point)
 Start-Sleep -Milliseconds 110
 $b=[ReplayWindowMessages]::PostMessage($game.MainWindowHandle,0x0201,[IntPtr]1,$point)
 Start-Sleep -Milliseconds 95
 $c=[ReplayWindowMessages]::PostMessage($game.MainWindowHandle,0x0202,[IntPtr]::Zero,$point)
 if (-not ($a -and $b -and $c)) {throw 'QA_BLOCKED: window rejected a target message'}
 Start-Sleep -Milliseconds 450
}
function ExpectLog([string]$pattern) {
 $found=Select-String -Path $log -Pattern $pattern -ErrorAction Stop | Select-Object -Last 1
 if (-not $found){throw ('QA_PARTIAL: missing '+$pattern)}
 Write-Output $found.Line
}
function CaptureGame([string]$path) {
 $wr=New-Object ReplayClientRect
 if (-not [ReplayWindowMessages]::GetWindowRect($game.MainWindowHandle,[ref]$wr)) {
  throw 'QA_BLOCKED: window rectangle unavailable'
 }
 $width=$wr.Right-$wr.Left
 $height=$wr.Bottom-$wr.Top
 if ($width -lt 1280 -or $width -gt 1350 -or $height -lt 720 -or $height -gt 790){
  throw 'QA_BLOCKED: unexpected screenshot dimensions'
 }
 $bitmap=New-Object System.Drawing.Bitmap($width,$height)
 $gfx=[System.Drawing.Graphics]::FromImage($bitmap)
 $dc=$gfx.GetHdc()
 try{$ok=[ReplayWindowMessages]::PrintWindow($game.MainWindowHandle,$dc,2)}
 finally{$gfx.ReleaseHdc($dc);$gfx.Dispose()}
 if (-not $ok){$bitmap.Dispose();throw 'QA_PARTIAL: PrintWindow unsupported'}
 $bitmap.Save($path,[System.Drawing.Imaging.ImageFormat]::Png)
 $bitmap.Dispose()
 Write-Output ('WINDOW_CAPTURED '+$path)
}
Write-Output "GAME_PID_PINNED=$TargetProcessId; no global mouse input"
ClickGame 435 377
ExpectLog '\[AI HEROES\] FOREST selected'
ClickGame 235 630
ExpectLog '\[AI HEROES\] FOREST CAPTURED'
ExpectLog '\[AI HEROES BATTLE\] R1'
Write-Output 'PLAYER_AUTHORIZED_AI_BATTLE_PASS'

ClickGame 1200 20
ExpectLog '\[AI HEROES 3D\] START'
Start-Sleep -Seconds 2
ExpectLog '\[AI HEROES REPLAY\] 1/'
CaptureGame 'C:\github\AIHEROES\Saved\Screenshots\AIHeroes3DReplayQA.png'

ClickGame 235 630
ExpectLog '\[AI HEROES 3D\] SKIP requested'
Start-Sleep -Milliseconds 500
ExpectLog '\[AI HEROES 3D\] FINISH'
CaptureGame 'C:\github\AIHEROES\Saved\Screenshots\AIHeroesAfterReplayQA.png'
Write-Output 'CAMERA_RESTORE_AND_SKIP_PASS'
Write-Output '3D_REPLAY_INTERACTIVE_QA_PASS'
