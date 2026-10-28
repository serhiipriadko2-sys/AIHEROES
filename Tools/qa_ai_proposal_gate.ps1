param([Parameter(Mandatory=$true)][int]$TargetProcessId)

# Window-targeted advisor/proposal QA. No global keyboard/mouse and no saves.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct ProposalClientRect { public int Left; public int Top; public int Right; public int Bottom; }
public static class ProposalWindow {
 [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out ProposalClientRect rect);
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out ProposalClientRect rect);
 [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
}
'@
$game = Get-Process -Id $TargetProcessId -ErrorAction Stop
$cmd = (Get-CimInstance Win32_Process -Filter "ProcessId=$TargetProcessId").CommandLine
if ($game.MainWindowHandle -eq [IntPtr]::Zero -or
    $game.MainWindowTitle -notlike '*AIHeroes*' -or
    $cmd -notmatch 'AIHeroes[.]uproject' -or $cmd -notmatch '-game' -or
    $cmd -notmatch 'AIHeroesProposalQA[.]log') {
    throw 'QA_BLOCKED: wrong game process/window/log'
}
$rect=New-Object ProposalClientRect
if (-not [ProposalWindow]::GetClientRect($game.MainWindowHandle,[ref]$rect) -or
    $rect.Right -ne 1280 -or $rect.Bottom -ne 720) {
    throw 'QA_BLOCKED: viewport not 1280x720'
}
$log = 'C:\github\AIHEROES\Saved\Logs\AIHeroesProposalQA.log'
if (!(Test-Path $log)) {throw 'QA_BLOCKED: separate game log missing'}

function ClickGame([int]$x,[int]$y) {
    $point = [IntPtr](($y -shl 16) -bor $x)
    $a=[ProposalWindow]::PostMessage($game.MainWindowHandle,0x200,[IntPtr]::Zero,$point)
    Start-Sleep -Milliseconds 100
    $b=[ProposalWindow]::PostMessage($game.MainWindowHandle,0x201,[IntPtr]1,$point)
    Start-Sleep -Milliseconds 90
    $c=[ProposalWindow]::PostMessage($game.MainWindowHandle,0x202,[IntPtr]::Zero,$point)
    if (-not ($a -and $b -and $c)) {throw 'QA_BLOCKED: cannot target game HWND'}
    Start-Sleep -Milliseconds 550
}

function RequireLog([string]$pattern) {
    $hit=Select-String -LiteralPath $log -Pattern $pattern |Select-Object -Last 1
    if (!$hit) {throw "QA_FAIL: missing game log $pattern"}
    Write-Output $hit.Line
}

function CaptureGame([string]$path) {
    $wr=New-Object ProposalClientRect
    if (-not [ProposalWindow]::GetWindowRect($game.MainWindowHandle,[ref]$wr)) {
        throw 'QA_FAIL: window geometry unavailable'
    }
    $w=$wr.Right-$wr.Left
    $h=$wr.Bottom-$wr.Top
    if($w -lt 1280 -or $w -gt 1350 -or $h -lt 720 -or $h -gt 790){
        throw 'QA_FAIL: unexpected window geometry'
    }
    $bitmap=New-Object System.Drawing.Bitmap($w,$h)
    $gfx=[System.Drawing.Graphics]::FromImage($bitmap)
    $dc=$gfx.GetHdc()
    try {$ok=[ProposalWindow]::PrintWindow($game.MainWindowHandle,$dc,2)}
    finally {$gfx.ReleaseHdc($dc);$gfx.Dispose()}
    if(-not $ok){$bitmap.Dispose();throw 'QA_FAIL: window capture unsupported'}
    $bitmap.Save($path,[System.Drawing.Imaging.ImageFormat]::Png)
    $bitmap.Dispose()
    Write-Output ('GAME_SCREENSHOT '+$path)
}

Write-Output ('QA_TARGET_PID='+$TargetProcessId)
ClickGame 435 377
RequireLog '\[AI HEROES\] FOREST selected'
ClickGame 505 628  # ASK HERO: the starting SEED cannot propose an order.
RequireLog '\[AI HEROES PROPOSAL\] accepted=1 requires_player=0 target=None source=offline'
ClickGame 1180 587 # Open read-only dossier without keyboard focus.
RequireLog '\[AI HEROES DOSSIER\] OPEN fresh=1 kind=strategy'
$before=@(Select-String -LiteralPath $log -Pattern '\[AI HEROES\] FOREST selected').Count
ClickGame 435 377  # Modal must consume this click, not select the map.
$after=@(Select-String -LiteralPath $log -Pattern '\[AI HEROES\] FOREST selected').Count
if($before -ne $after){throw 'QA_FAIL: modal click selected a map node'}
CaptureGame 'C:\github\AIHEROES\Saved\Screenshots\AIHeroesProposalSeedQA.png'
ClickGame 995 628  # Dossier CLOSE.
RequireLog '\[AI HEROES DOSSIER\] CLOSE no_order=1'

# Explicit human upgrade unlocks ANALYST; the gate cannot do it itself.
ClickGame 505 681  # Unlock cognitive protocol (P), strategic bottom row.
RequireLog '\[AI HEROES\] Unlocked THREAT ANALYSIS'
ClickGame 505 628  # ASK HERO again with the higher cognition tier.
RequireLog '\[AI HEROES PROPOSAL\] accepted=1 requires_player=1 target=FOREST source=offline'
ClickGame 1180 587
RequireLog '\[AI HEROES DOSSIER\] OPEN fresh=1 kind=strategy'
CaptureGame 'C:\github\AIHEROES\Saved\Screenshots\AIHeroesProposalAnalystQA.png'
ClickGame 995 628
RequireLog '\[AI HEROES DOSSIER\] CLOSE no_order=1'
if(Select-String -LiteralPath $log -Pattern '\[AI HEROES LIVE\] COMMIT|SLOT [123]: SAVED'){
    throw 'QA_FAIL: an advice-only interaction started combat or saved the campaign'
}
CaptureGame 'C:\github\AIHEROES\Saved\Screenshots\AIHeroesProposalMapQA.png'
Write-Output 'AI_PROPOSAL_WINDOW_QA_PASS SEED_ADVICE=1 ANALYST_CONFIRMATION_REQUIRED=1'
