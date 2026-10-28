# Visual acceptance for the read-only GOALS dialog in a uniquely targeted game.
# No global keyboard injection, no SaveGame writes, no editor/process termination.
$ErrorActionPreference = 'Stop'
$root = 'C:\github\AIHEROES'
$tag = 'AIHeroesObjectivesQA.log'
$log = Join-Path $root ('Saved\Logs\' + $tag)
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class ObjectivesQAInput {
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
}
'@
$candidates = @(Get-CimInstance Win32_Process | Where-Object {
    $_.Name -eq 'UnrealEditor.exe' -and
    $_.CommandLine -like '*AIHeroes.uproject*' -and
    $_.CommandLine -like '*-log=AIHeroesObjectivesQA.log*'
})
if ($candidates.Count -ne 1) { throw ('Unique test window required, found ' + $candidates.Count) }
$game = Get-Process -Id $candidates[0].ProcessId -ErrorAction Stop
if ($game.MainWindowHandle -eq [IntPtr]::Zero -or -not(Test-Path -LiteralPath $log)) {
    throw 'Game window or dedicated log is missing'
}
function Press([int]$Key) {
    if (-not [ObjectivesQAInput]::PostMessage($game.MainWindowHandle,0x100,[IntPtr]$Key,[IntPtr]0)) {
        throw ('Key press failed ' + $Key)
    }
    Start-Sleep -Milliseconds 140
    if (-not [ObjectivesQAInput]::PostMessage($game.MainWindowHandle,0x101,[IntPtr]$Key,[IntPtr]0)) {
        throw ('Key release failed ' + $Key)
    }
    Start-Sleep -Milliseconds 420
}
function Expect([string]$Pattern) {
    $line = Select-String -LiteralPath $log -Pattern $Pattern | Select-Object -Last 1
    if (-not $line) { throw ('Expected game event missing: ' + $Pattern) }
    Write-Output $line.Line
}
function Capture([string]$File) {
    & (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $tag -OutputPath (Join-Path $root ('Docs\' + $File))
}
Press 0x4F # O: open goals panel
Expect '\[AI HEROES OBJECTIVES\] OPEN 0/4 victory=0 no_order=1'
Capture 'MISSION_OBJECTIVES_DOSSIER_2026-10-28.png'
Press 0x20 # Space: forbidden strategic turn while modal
Expect '\[AI HEROES\] REPORT OPEN:'
if (Select-String -LiteralPath $log -Pattern '\[AI HEROES\] Day 2:' -Quiet) {
    throw 'Mission dossier permitted an unauthorized day transition'
}
Press 0x4F # O: close goals without an order
Expect '\[AI HEROES DOSSIER\] CLOSE no_order=1'
Capture 'MISSION_OBJECTIVES_RETURN_2026-10-28.png'
if (Select-String -LiteralPath $log -Pattern '\[AI HEROES\] (SLOT .*GAME SAVED|GAME LOADED)' -Quiet) {
    throw 'Read-only UI unexpectedly issued a save or load'
}
Write-Output ('OBJECTIVES_MODAL_INPUT_PASS pid='+$game.Id+' no_turn=1 no_save=1')
