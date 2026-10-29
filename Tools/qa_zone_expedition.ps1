param(
    [int]$Seed = 20261029,
    [int]$Branches = 3
)
# Windows development-only visual acceptance. The process is uniquely tagged.
# No campaign SaveGame is written, read, renamed or deleted by this script.
$ErrorActionPreference = 'Stop'
$root = 'C:\github\AIHEROES'
$tag = 'AIHeroesZoneExpeditionQA.log'
$log = Join-Path $root ('Saved\Logs\' + $tag)
$saveDir = Join-Path $root 'Saved\SaveGames'
$before = @{}
if (Test-Path $saveDir) {
    Get-ChildItem -LiteralPath $saveDir -File | ForEach-Object {
        $before[$_.FullName] = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }
}
$existing = @(Get-CimInstance Win32_Process | Where-Object {
    $_.Name -eq 'UnrealEditor.exe' -and
    $_.CommandLine -like '*-log=AIHeroesZoneExpeditionQA.log*'
})
if ($existing.Count -gt 0) { throw 'QA_BOUNDARY: dedicated prior window still running' }
if (Test-Path $log) { Remove-Item -LiteralPath $log -Force }
$exe = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$game = Start-Process -FilePath $exe -PassThru -ArgumentList @(
    ('"{0}"' -f (Join-Path $root 'AIHeroes.uproject')),
    '"/Engine/Maps/Entry?game=/Script/AIHeroes.AIHeroesZoneExpeditionGameMode"',
    '-game', '-windowed', '-ResX=1280', '-ResY=720', '-NoSplash',
    '-NoP4', '-NoSound', "-ZoneSeed=$Seed", "-ZoneBranches=$Branches", "-log=$tag"
)
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class ZoneExpeditionQAInput {
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
}
'@
function Await([string]$Regex, [int]$Limit = 110) {
    for ($i = 0; $i -lt $Limit; $i++) {
        if (Test-Path $log) {
            $hit = Select-String -LiteralPath $log -Pattern $Regex | Select-Object -Last 1
            if ($hit) { Write-Output $hit.Line; return }
        }
        Start-Sleep -Milliseconds 350
    }
    throw ('ZONE_EXPEDITION_QA_TIMEOUT ' + $Regex)
}
function KeyPress([int]$Vk) {
    $g = Get-Process -Id $game.Id -ErrorAction Stop
    if ($g.MainWindowHandle -eq [IntPtr]::Zero) { throw 'QA_WINDOW_HANDLE_MISSING' }
    if (-not [ZoneExpeditionQAInput]::PostMessage($g.MainWindowHandle, 0x100, [IntPtr]$Vk, [IntPtr]0)) {
        throw 'QA_POST_KEYDOWN_FAILED'
    }
    Start-Sleep -Milliseconds 230
    if (-not [ZoneExpeditionQAInput]::PostMessage($g.MainWindowHandle, 0x101, [IntPtr]$Vk, [IntPtr]0)) {
        throw 'QA_POST_KEYUP_FAILED'
    }
    Start-Sleep -Milliseconds 620
}
function Capture([string]$OutputName) {
    & (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $tag -OutputPath (Join-Path $root ('Docs\' + $OutputName))
    if ($LASTEXITCODE -ne 0) { throw 'QA_CAPTURE_FAILED' }
}
Await '\[ZONE EXPEDITION\] READY seed='
Capture 'AIHEROES_EXPEDITION_START_2026-10-29.png'
# Initial target FOREST is selected in this mode, but no command issued.
KeyPress 0x0D  # HUMAN ENTER, not an AI auto-order.
Await '\[ZONE EXPEDITION\] PLAYER CONFIRMED FOREST \| battle=1 won=1'
Capture 'AIHEROES_EXPEDITION_FOREST_2026-10-29.png'
KeyPress 0x20  # HUMAN SPACE: restore movement
Await '\[ZONE EXPEDITION\] DAY 2'
KeyPress 0x09  # HUMAN TAB: select next visible zone
Await '\[ZONE EXPEDITION\] Selected known zone '
KeyPress 0x0D  # HUMAN ENTER: next explicitly confirmed order
Await '\[ZONE EXPEDITION\] PLAYER CONFIRMED '
Capture 'AIHEROES_EXPEDITION_DAY2_2026-10-29.png'
$after = @{}
if (Test-Path $saveDir) {
    Get-ChildItem -LiteralPath $saveDir -File | ForEach-Object {
        $after[$_.FullName] = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }
}
if ($before.Count -ne $after.Count) { throw 'QA_SAVE_COUNT_CHANGED' }
foreach ($path in $before.Keys) {
    if (-not $after.ContainsKey($path) -or $after[$path] -ne $before[$path]) {
        throw ('QA_PLAYER_SAVE_CHANGED ' + $path)
    }
}
Write-Output ('ZONE_EXPEDITION_GAME_QA_PASS seed=' + $Seed +
    ' branches=' + $Branches + ' player_confirmed_orders=2 user_save_writes=0 pid=' + $game.Id)
