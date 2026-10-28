# Checks deterministic rival-week integration in a uniquely targeted QA window.
# Sends input ONLY to the own -log=AIHeroesRivalQA.log process.
# No SaveGame calls, no global keyboard injection, no forced process closure.
$ErrorActionPreference = 'Stop'
$root = 'C:\github\AIHEROES'
$tag = 'AIHeroesRivalQA.log'
$log = Join-Path $root ('Saved\Logs\' + $tag)
$saveDir = Join-Path $root 'Saved\SaveGames'
$before = @{}
if (Test-Path $saveDir) {
    Get-ChildItem $saveDir -File | ForEach-Object {
        $before[$_.FullName] = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }
}

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class AIHeroesRivalQAMessages {
 [DllImport("user32.dll")]
 public static extern bool PostMessage(IntPtr hWnd, uint msg, IntPtr wp, IntPtr lp);
}
'@

$candidates = @(Get-CimInstance Win32_Process | Where-Object {
    $_.Name -eq 'UnrealEditor.exe' -and
    $_.CommandLine -like '*AIHeroes.uproject*' -and
    $_.CommandLine -like '*-log=AIHeroesRivalQA.log*'
})
if ($candidates.Count -ne 1) {
    throw "QA_BOUNDARY: expected exactly one dedicated test game, got $($candidates.Count)"
}
$game = Get-Process -Id $candidates[0].ProcessId -ErrorAction Stop
if ($game.MainWindowHandle -eq [IntPtr]::Zero -or -not(Test-Path $log)) {
    throw 'QA_BOUNDARY: game window or dedicated log missing'
}
Write-Output ('RIVAL_QA_TARGET_PID='+$game.Id)

function Press([int]$key) {
    $handle = $game.MainWindowHandle
    if (-not [AIHeroesRivalQAMessages]::PostMessage($handle, 0x100, [IntPtr]$key, [IntPtr]0)) {
        throw "QA input rejected ($key down)"
    }
    Start-Sleep -Milliseconds 135
    if (-not [AIHeroesRivalQAMessages]::PostMessage($handle, 0x101, [IntPtr]$key, [IntPtr]0)) {
        throw "QA input rejected ($key up)"
    }
    Start-Sleep -Milliseconds 340
}
function Expect([string]$regex) {
    $line = Select-String -LiteralPath $log -Pattern $regex | Select-Object -Last 1
    if (-not $line) { throw ('QA_FAILED: no logged event matching '+$regex) }
    Write-Output $line.Line
}
for ($day = 2; $day -le 7; $day++) {
    Press 0x20  # SPACE is the player-authorized day transition.
    Expect ('\[AI HEROES\] Day '+$day+': income collected')
}
& (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $tag -OutputPath (Join-Path $root 'Docs\RIVAL_PRE_WEEK_QA_2026-10-28.png')
Press 0x20 # Day 7 -> Day 8 is first week boundary.
Expect '\[AI HEROES WEEK\] week=2 day=8 recruits='
Expect '\[AI HEROES RIVAL\] day=8 action=1 amount=22'
Expect '\[AI HEROES\] Day 8: income collected'
& (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $tag -OutputPath (Join-Path $root 'Docs\RIVAL_WEEK_TWO_QA_2026-10-28.png')

$after=@{}
if (Test-Path $saveDir) {
    Get-ChildItem $saveDir -File | ForEach-Object {
        $after[$_.FullName] = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }
}
if ($before.Count -ne $after.Count) { throw 'QA_FAIL: SaveGame file count unexpectedly changed' }
foreach ($path in $before.Keys) {
    if (-not $after.ContainsKey($path) -or $after[$path] -ne $before[$path]) {
        throw ('QA_FAIL: test changed a campaign save: '+$path)
    }
}
Write-Output 'RIVAL_WEEKLY_INTEGRATION_PASS no_save_writes=1 day8_reinforcement=1'
