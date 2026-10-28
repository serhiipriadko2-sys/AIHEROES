param([Parameter(Mandatory=$true)][int]$TargetProcessId)
# Isolated window / exact PID. No desktop focus, global mouse or save writes.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct WeekClientRect { public int Left; public int Top; public int Right; public int Bottom; }
public static class WeekWin {
 [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out WeekClientRect rect);
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out WeekClientRect rect);
 [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
}
'@
$game = Get-Process -Id $TargetProcessId -ErrorAction Stop
$cmd = (Get-CimInstance Win32_Process -Filter "ProcessId=$TargetProcessId").CommandLine
if($game.MainWindowHandle -eq [IntPtr]::Zero -or
   $game.MainWindowTitle -notlike '*AIHeroes*' -or
   $cmd -notmatch 'AIHeroes[.]uproject' -or
   $cmd -notmatch '-game' -or
   $cmd -notmatch 'AIHeroesWorldWeekQA[.]log') {
 throw 'QA_BLOCKED: wrong game PID/window/log'
}
$rect=New-Object WeekClientRect
if (-not [WeekWin]::GetClientRect($game.MainWindowHandle,[ref]$rect) -or
    $rect.Right -ne 1280 -or $rect.Bottom -ne 720) {
 throw 'QA_BLOCKED: expected 1280x720 game client'
}
$log='C:\github\AIHEROES\Saved\Logs\AIHeroesWorldWeekQA.log'
if (!(Test-Path $log)) {throw 'QA_BLOCKED: separate game log missing'}
function ClickGame([int]$x,[int]$y) {
 $point=[IntPtr](($y -shl 16) -bor $x)
 $a=[WeekWin]::PostMessage($game.MainWindowHandle,0x200,[IntPtr]::Zero,$point)
 Start-Sleep -Milliseconds 95
 $b=[WeekWin]::PostMessage($game.MainWindowHandle,0x201,[IntPtr]1,$point)
 Start-Sleep -Milliseconds 90
 $c=[WeekWin]::PostMessage($game.MainWindowHandle,0x202,[IntPtr]::Zero,$point)
 if(-not ($a -and $b -and $c)){throw 'QA_FAIL: game rejected targeted mouse input'}
 Start-Sleep -Milliseconds 380
}
function Expect([string]$pattern) {
 $hit=Select-String -LiteralPath $log -Pattern $pattern |Select-Object -Last 1
 if(!$hit){throw ('QA_FAIL: missing expected log '+$pattern)}
 Write-Output $hit.Line
}
function Capture([string]$name) {
 $wr=New-Object WeekClientRect
 if(-not [WeekWin]::GetWindowRect($game.MainWindowHandle,[ref]$wr)){
    throw 'QA_FAIL: no window dimensions'
 }
 $w=$wr.Right-$wr.Left;$h=$wr.Bottom-$wr.Top
 if($w -lt 1280 -or $w -gt 1350 -or $h -lt 720 -or $h -gt 790){
    throw 'QA_FAIL: unexpected window screenshot dimensions'
 }
 $bmp=New-Object System.Drawing.Bitmap($w,$h)
 $g=[System.Drawing.Graphics]::FromImage($bmp)
 $dc=$g.GetHdc()
 try {$ok=[WeekWin]::PrintWindow($game.MainWindowHandle,$dc,2)}
 finally {$g.ReleaseHdc($dc);$g.Dispose()}
 if(!$ok){$bmp.Dispose();throw 'QA_FAIL: PrintWindow unsupported'}
 $dst='C:\github\AIHEROES\Saved\Screenshots\'+$name
 $bmp.Save($dst,[System.Drawing.Imaging.ImageFormat]::Png)
 $bmp.Dispose()
 Write-Output ('GAME_ONLY_SCREENSHOT '+$dst)
}
Write-Output ('WINDOW_PID_PINNED='+$TargetProcessId)
ClickGame 435 377 # FOREST node
Expect '\[AI HEROES\] FOREST selected'
Capture 'AIHeroesWeekDay1QA.png'
for($day=2;$day -le 8;$day++){
 ClickGame 775 628 # NEXT DAY; UI action index 2 at 1280x720
 Expect ('\[AI HEROES\] Day '+$day+': income collected')
}
Expect '\[AI HEROES WEEK\] week=2 day=8 recruits=7'
if(Select-String -LiteralPath $log -Pattern 'GAME SAVED|LIVE BATTLE COMMIT'){
 throw 'QA_FAIL: advancing an unopposed week caused save or combat commit'
}
Capture 'AIHeroesWeekDay8QA.png'
Write-Output 'WORLD_WEEK_EXACT_WINDOW_QA_PASS day=8 week=2 new_neutral_units=7 no_game_save'
