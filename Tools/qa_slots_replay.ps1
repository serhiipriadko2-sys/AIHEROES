# Smoke-test ONLY the dedicated AIHeroesSlotsQA.log window.
# Never send global input or overwrite pre-existing human campaign slots.
$ErrorActionPreference = 'Stop'
$root = 'C:\github\AIHEROES'
$log = Join-Path $root 'Saved\Logs\AIHeroesSlotsQA.log'
$saveDir = Join-Path $root 'Saved\SaveGames'
$base = 'AIHEROES_Campaign_Resilient_v1_Player2'
$slotA = Join-Path $saveDir ($base + '_A.sav')
$slotB = Join-Path $saveDir ($base + '_B.sav')
$legacy = Join-Path $saveDir 'AIHEROES_Campaign_v1.sav'
if ((Test-Path $slotA) -or (Test-Path $slotB)) {
    throw 'QA_BLOCKED: Player slot 2 is already occupied. No keys sent.'
}
$legacyHash = if (Test-Path $legacy) { (Get-FileHash -LiteralPath $legacy -Algorithm SHA256).Hash } else { '' }

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct SlotQAClient {public int Left,Top,Right,Bottom;}
public static class SlotQAWindow {
 [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h,out SlotQAClient r);
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h,uint msg,IntPtr w,IntPtr l);
}
'@
$candidates = @(Get-CimInstance Win32_Process |
    Where-Object { $_.Name -eq 'UnrealEditor.exe' -and
        $_.CommandLine -like '*AIHeroes.uproject*' -and
        $_.CommandLine -like '*-log=AIHeroesSlotsQA.log*' })
if ($candidates.Count -ne 1) { throw "QA_BLOCKED: unexpected game process count $($candidates.Count)" }
$game = Get-Process -Id $candidates[0].ProcessId -ErrorAction Stop
if ($game.MainWindowHandle -eq [IntPtr]::Zero -or -not (Test-Path $log)) {
    throw 'QA_BLOCKED: expected game/log missing'
}
$viewport = New-Object SlotQAClient
if (-not [SlotQAWindow]::GetClientRect($game.MainWindowHandle,[ref]$viewport) -or
    $viewport.Right -ne 1280 -or $viewport.Bottom -ne 720) {
    throw 'QA_BLOCKED: unexpected game client dimensions'
}
Write-Output ('QA_TARGET_VERIFIED pid=' + $game.Id)

function HitKey([int]$key) {
    $h = $game.MainWindowHandle
    $press = [SlotQAWindow]::PostMessage($h,0x100,[IntPtr]$key,[IntPtr]0)
    Start-Sleep -Milliseconds 180
    $release = [SlotQAWindow]::PostMessage($h,0x101,[IntPtr]$key,[IntPtr]0)
    if (-not ($press -and $release)) { throw 'QA_INPUT_FAILED: game rejected a key' }
    Start-Sleep -Milliseconds 520
}
function Click([int]$x,[int]$y) {
    $p=[IntPtr](($y -shl 16) -bor $x)
    foreach($cmd in @(0x200,0x201,0x202)){
        $w=if($cmd -eq 0x201){[IntPtr]1}else{[IntPtr]0}
        if(-not [SlotQAWindow]::PostMessage($game.MainWindowHandle,[uint32]$cmd,$w,$p)){
            throw "QA_INPUT_FAILED: click message $cmd"
        }
        Start-Sleep -Milliseconds 140
    }
    Start-Sleep -Milliseconds 400
}
function Expect([string]$pattern) {
    $found = Select-String -LiteralPath $log -Pattern $pattern | Select-Object -Last 1
    if (-not $found) { throw ('QA_PARTIAL: missing '+$pattern) }
    Write-Output $found.Line
}
function Capture([string]$name) {
    & (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag 'AIHeroesSlotsQA.log' -OutputPath (Join-Path $root ('Saved\Screenshots\'+$name))
}

# Slot 2: independent A/B saves. Existing legacy is never written.
HitKey 0x43  # C: human save slot 1 -> 2
Expect '\[AI HEROES\].*SAVE SLOT 2/3'
HitKey 0x4B  # F5: first slot-2 A save
Expect '\[AI HEROES\].*SLOT 2: GAME SAVED'
if (-not (Test-Path $slotA)) { throw 'QA_FAIL: slot-2 primary A missing' }
HitKey 0x20  # SPACE: next day
Expect '\[AI HEROES\] Day 2:'
HitKey 0x4B  # F5: next save rotates to B
Expect '\[AI HEROES\].*SLOT 2: GAME SAVED'
if (-not (Test-Path $slotB)) { throw 'QA_FAIL: slot-2 recovery B missing' }
HitKey 0x4C  # L: load most recent
Expect '\[AI HEROES\].*GAME LOADED SLOT 2: day 2'
Write-Output 'MULTI_SLOT_SAVE_AND_BACKUP_PASS'

# Authorize one neutral combat and test both text and 3D replay.
Click 435 377   # Forest node
Expect '\[AI HEROES\] FOREST selected'
HitKey 0x0D  # Enter: player-confirmed battle
Expect '\[AI HEROES\] FOREST CAPTURED'
HitKey 0x4A  # J: tactical transcript
Expect '\[AI HEROES\] REPLAY FOREST 1/'
Capture 'AIHeroesReplayTextQA.png'
HitKey 0x56  # V: 3D tactical representation
Expect '\[AI HEROES 3D\] START'
Start-Sleep -Seconds 2
Expect '\[AI HEROES REPLAY\] 1/'
Capture 'AIHeroesReplay3DQA.png'
HitKey 0x1B  # Escape: skip 3D replay, return to map
Expect '\[AI HEROES 3D\] SKIP'
Start-Sleep -Seconds 1
Expect '\[AI HEROES 3D\] FINISH'
Write-Output 'TACTICAL_REPLAY_SCREEN_TEST_PASS'

# Persist a battle record in slot 2, then verify a fresh load and replay.
HitKey 0x4B
Expect '\[AI HEROES\].*SLOT 2: GAME SAVED'
HitKey 0x4C
Expect '\[AI HEROES\].*GAME LOADED SLOT 2: day 2'
HitKey 0x4A
Expect '\[AI HEROES\] REPLAY FOREST'
Capture 'AIHeroesReplayRestoredQA.png'
Write-Output 'REPLAY_PERSISTENCE_PASS'

# Slot 3 never loads slot 2; slot 1 retains the original legacy file.
HitKey 0x43   # -> 3
Expect '\[AI HEROES\].*SAVE SLOT 3/3'
HitKey 0x4C
Expect '\[AI HEROES\].*LOAD BLOCKED:'
HitKey 0x43   # -> 1
Expect '\[AI HEROES\].*SAVE SLOT 1/3'
if ($legacyHash) {
    HitKey 0x4C
    Expect '\[AI HEROES\].*GAME LOADED SLOT 1:'
    if ((Get-FileHash -LiteralPath $legacy -Algorithm SHA256).Hash -ne $legacyHash) {
        throw 'QA_FAIL: legacy save hash changed'
    }
    Write-Output 'LEGACY_SAVE_READ_ONLY_PASS'
}

Write-Output ('QA_SAVES Slot2_A='+(Get-Item $slotA).Length+' Slot2_B='+(Get-Item $slotB).Length)
Write-Output 'MANUAL_WINDOWS_PLAYTHROUGH_PASS'
