# Window-targeted AIHeroes gameplay smoke test (Unreal game, 1280x720).
# Sends WM_MOUSE* only to verified AIHeroes -game window. No foreground or global input.
$ErrorActionPreference='Stop'
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct AIHClientRect { public int Left; public int Top; public int Right; public int Bottom; }
public static class AIHWindowInput {
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out AIHClientRect rect);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
}
'@

$game = Get-Process UnrealEditor -ErrorAction Stop |
    Where-Object { $_.MainWindowHandle -ne [IntPtr]::Zero -and $_.MainWindowTitle -like '*AIHeroes*' } |
    Select-Object -First 1
if (-not $game) { throw 'QA_BLOCKED: AIHeroes game process not found' }
$commandLine = (Get-CimInstance Win32_Process -Filter "ProcessId=$($game.Id)").CommandLine
if ($commandLine -notmatch 'AIHeroes[.]uproject' -or $commandLine -notmatch '-game') {
    throw 'QA_BLOCKED: window does not belong to the intended -game process'
}
$client = New-Object AIHClientRect
if (-not [AIHWindowInput]::GetClientRect($game.MainWindowHandle,[ref]$client)) {
    throw 'QA_BLOCKED: cannot get game client dimensions'
}
if ($client.Right -ne 1280 -or $client.Bottom -ne 720) {
    throw "QA_BLOCKED: wrong viewport ($($client.Right) x $($client.Bottom))"
}

function ClickGame([int]$x, [int]$y) {
    $point = [IntPtr](($y -shl 16) -bor $x)
    if (-not [AIHWindowInput]::PostMessage($game.MainWindowHandle,0x0200,[IntPtr]::Zero,$point)) {
        throw 'QA_BLOCKED: WM_MOUSEMOVE denied'
    }
    Start-Sleep -Milliseconds 100
    if (-not [AIHWindowInput]::PostMessage($game.MainWindowHandle,0x0201,[IntPtr]1,$point)) {
        throw 'QA_BLOCKED: WM_LBUTTONDOWN denied'
    }
    Start-Sleep -Milliseconds 100
    if (-not [AIHWindowInput]::PostMessage($game.MainWindowHandle,0x0202,[IntPtr]::Zero,$point)) {
        throw 'QA_BLOCKED: WM_LBUTTONUP denied'
    }
    Start-Sleep -Milliseconds 450
}

$log = 'C:\github\AIHEROES\Saved\Logs\AIHeroes.log'
function LastGameEvent() {
    $matches = @(Select-String -Path $log -Pattern '\[AI HEROES\]' -ErrorAction Stop)
    if ($matches.Count -eq 0) { return '' }
    return $matches[-1].Line
}
function ExpectEvent([string]$text) {
    $actual=LastGameEvent
    Write-Output $actual
    if ($actual -notlike ('*'+$text+'*')) {
        throw ('QA_PARTIAL: expected '+$text+', got '+$actual)
    }
}
Write-Output "WINDOW_TARGET_VERIFIED pid=$($game.Id); no global input"
ClickGame 435 377 # forest 3D mesh
ExpectEvent 'FOREST selected'
Write-Output 'SELECT_PASS'

ClickGame 505 631 # ASK HERO (4-column HUD)
& 'C:\github\AIHEROES\Tools\qa_capture_game.ps1' | Out-Null
Copy-Item 'C:\github\AIHEROES\Saved\Screenshots\AIHeroesQA.png' 'C:\github\AIHEROES\Saved\Screenshots\AIHeroesAdviceQA.png' -Force
Write-Output 'ADVICE_CAPTURED'

ClickGame 235 630 # confirm move / battle
ExpectEvent 'FOREST CAPTURED'
Write-Output 'BATTLE_AND_CAPTURE_PASS'

ClickGame 775 631 # next day
ExpectEvent 'Day 2: income collected'
Write-Output 'TURN_INCOME_PASS'

ClickGame 775 684 # model slot only, independent of cognitive protocol
ExpectEvent 'MODEL SLOT 3B unlocked'
Write-Output 'MODEL_SLOT_3B_PASS'
& 'C:\github\AIHEROES\Tools\qa_capture_game.ps1' | Out-Null
Copy-Item 'C:\github\AIHEROES\Saved\Screenshots\AIHeroesQA.png' 'C:\github\AIHEROES\Saved\Screenshots\AIHeroesSlotBeforeProtocolQA.png' -Force
ClickGame 505 684 # upgrade protocol
ExpectEvent 'Unlocked THREAT ANALYSIS'
Write-Output 'PROTOCOL_UPGRADE_PASS'

ClickGame 805 340 # select ORE node for newly unlocked advice
ExpectEvent 'ORE selected'
ClickGame 505 631 # ask hero, TRACE available
& 'C:\github\AIHEROES\Tools\qa_capture_game.ps1' | Out-Null
Copy-Item 'C:\github\AIHEROES\Saved\Screenshots\AIHeroesQA.png' 'C:\github\AIHEROES\Saved\Screenshots\AIHeroesPlaythroughQA.png' -Force
Write-Output 'FINAL_SCREEN_CAPTURED'
Select-String -Path $log -Pattern '\[AI HEROES\]|\[AI HEROES BATTLE\]' |
    Select-Object -Last 15 | ForEach-Object { $_.Line }
Write-Output 'INTERACTIVE_QA_PASS'
