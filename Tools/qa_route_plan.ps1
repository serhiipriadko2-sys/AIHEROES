# AIHEROES Route UI QA — sends mouse events only to a validated isolated
# 1280x720 -game HWND. Never uses global input, save/load or user file writes.
# Launch UE with -log=AIHeroesRouteQA.log before running.
param(
    [switch]$ExpectUnlocked,
    [string]$LogTag = 'AIHeroesRouteQA.log'
)
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct RouteQARect { public int Left; public int Top; public int Right; public int Bottom; }
public static class RouteQAInput {
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RouteQARect r);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
}
'@

$root = 'C:\github\AIHEROES'
$matches = @(Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'" |
    Where-Object {
        $_.CommandLine -like '*AIHeroes.uproject*' -and
        $_.CommandLine -like '*-game*' -and
        $_.CommandLine -like "*-log=$LogTag*"
    })
if ($matches.Count -ne 1) {
    throw "ROUTE_QA_BLOCKED: expected exactly one isolated -game QA window; found $($matches.Count)"
}
$game = Get-Process -Id $matches[0].ProcessId -ErrorAction Stop
if ($game.MainWindowHandle -eq [IntPtr]::Zero) {
    throw 'ROUTE_QA_BLOCKED: target game has no visible main HWND'
}
$rect = New-Object RouteQARect
if (-not [RouteQAInput]::GetClientRect($game.MainWindowHandle, [ref]$rect)) {
    throw 'ROUTE_QA_BLOCKED: cannot verify client viewport'
}
if ($rect.Right -ne 1280 -or $rect.Bottom -ne 720) {
    throw "ROUTE_QA_BLOCKED: viewport $($rect.Right)x$($rect.Bottom) != 1280x720"
}
$log = Join-Path $root ('Saved\Logs\' + $LogTag)
if (-not (Test-Path -LiteralPath $log)) {throw 'ROUTE_QA_BLOCKED: expected dedicated game log is missing'}

# Snapshot metadata on user-owned saves and ensure the test never changes it.
$saveDir = Join-Path $root 'Saved\SaveGames'
function Get-SaveInventory {
    if (-not (Test-Path $saveDir)) {return @()}
    return @(
        Get-ChildItem -Path $saveDir -File | Sort-Object Name |
            ForEach-Object {
                '{0}|{1}|{2}|{3}' -f $_.Name,$_.Length,$_.LastWriteTimeUtc.Ticks,
                    (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
            }
    )
}
$beforeFiles = @(Get-SaveInventory)
$priorUI = @(Select-String -Path $log -Pattern '\[AI HEROES UI\] Button action=12' -ErrorAction SilentlyContinue).Count
$priorPlan = @(Select-String -Path $log -Pattern '\[AI HEROES ROUTE\]' -ErrorAction SilentlyContinue).Count
$priorLocked = @(Select-String -Path $log -Pattern '\[AI HEROES\] ROUTE LOCKED:' -ErrorAction SilentlyContinue).Count

# At 1280px width: grid four columns at X=105/375/645/915; PLAN is
# row 2 col 4: X=915..1175 Y=660..703. This does not overlap top utilities.
$x=1010; $y=681
$point = [IntPtr](($y -shl 16) -bor $x)
Write-Output "ROUTE_QA_TARGET pid=$($game.Id) client=$($rect.Right)x$($rect.Bottom) click=$x,$y"
if (-not [RouteQAInput]::PostMessage($game.MainWindowHandle,0x0200,[IntPtr]::Zero,$point)) {
    throw 'ROUTE_QA_BLOCKED: WM_MOUSEMOVE was refused'
}
Start-Sleep -Milliseconds 125
if (-not [RouteQAInput]::PostMessage($game.MainWindowHandle,0x0201,[IntPtr]1,$point)) {
    throw 'ROUTE_QA_BLOCKED: WM_LBUTTONDOWN was refused'
}
Start-Sleep -Milliseconds 125
if (-not [RouteQAInput]::PostMessage($game.MainWindowHandle,0x0202,[IntPtr]::Zero,$point)) {
    throw 'ROUTE_QA_BLOCKED: WM_LBUTTONUP was refused'
}
Start-Sleep -Milliseconds 550

$nextUI = @(Select-String -Path $log -Pattern '\[AI HEROES UI\] Button action=12' -ErrorAction SilentlyContinue).Count
$nextPlan = @(Select-String -Path $log -Pattern '\[AI HEROES ROUTE\]' -ErrorAction SilentlyContinue).Count
$nextLocked = @(Select-String -Path $log -Pattern '\[AI HEROES\] ROUTE LOCKED:' -ErrorAction SilentlyContinue).Count
if ($nextUI -le $priorUI) {
    throw 'ROUTE_QA_FAIL: clicking PLAN did not dispatch HUD action 12'
}
if ($ExpectUnlocked) {
    if ($nextPlan -le $priorPlan) {
        throw 'ROUTE_QA_FAIL: expected planner output (protocol tier 3)'
    }
    Write-Output 'ROUTE_UNLOCKED_PLAN_PASS'
} else {
    if ($nextLocked -le $priorLocked) {
        throw 'ROUTE_QA_FAIL: locked protocol did not reject plan safely'
    }
    Write-Output 'ROUTE_LOCKED_GUARD_PASS'
}

$afterFiles = @(Get-SaveInventory)
if (Compare-Object -ReferenceObject $beforeFiles -DifferenceObject $afterFiles) {
    throw 'ROUTE_QA_FAIL: persistent SaveGame files changed unexpectedly'
}
Write-Output 'SAVEGAME_IMMUTABILITY_PASS'

# Visual capture is an output artifact only, never proof of a 3D UI inspection.
$output = Join-Path $root 'Saved\Screenshots\AIHeroesRouteLockedQA.png'
if ($ExpectUnlocked) {
    $output = Join-Path $root 'Saved\Screenshots\AIHeroesRouteUnlockedQA.png'
}
& (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $LogTag -OutputPath $output
if (-not (Test-Path -LiteralPath $output) -or
    (Get-Item -LiteralPath $output).Length -lt 10000) {
    throw 'ROUTE_QA_FAIL: screenshot artifact missing or empty'
}
# Backwards-compatibility regression: the new recruitment tip is smaller,
# and the historic ASK HERO click at (505,630) must still work.
function ClickAction([int]$cx,[int]$cy) {
    $xy = [IntPtr](($cy -shl 16) -bor $cx)
    if (-not [RouteQAInput]::PostMessage($game.MainWindowHandle,0x0200,[IntPtr]::Zero,$xy)) {
        throw 'ROUTE_QA_FAIL: follow-up mouse move was refused'
    }
    Start-Sleep -Milliseconds 120
    if (-not [RouteQAInput]::PostMessage($game.MainWindowHandle,0x0201,[IntPtr]1,$xy)) {
        throw 'ROUTE_QA_FAIL: follow-up mouse down was refused'
    }
    Start-Sleep -Milliseconds 120
    if (-not [RouteQAInput]::PostMessage($game.MainWindowHandle,0x0202,[IntPtr]::Zero,$xy)) {
        throw 'ROUTE_QA_FAIL: follow-up mouse up was refused'
    }
    Start-Sleep -Milliseconds 440
}
$priorAsk = @(Select-String -Path $log -Pattern '\[AI HEROES UI\] Button action=1$' -ErrorAction SilentlyContinue).Count
$priorRecruit = @(Select-String -Path $log -Pattern '\[AI HEROES UI\] Button action=200$' -ErrorAction SilentlyContinue).Count
ClickAction 505 630
if (@(Select-String -Path $log -Pattern '\[AI HEROES UI\] Button action=1$').Count -le $priorAsk) {
    throw 'ROUTE_QA_FAIL: legacy ASK HERO center no longer responds'
}
Write-Output 'LEGACY_ASK_HERO_CLICK_PASS'
ClickAction 590 630
if (@(Select-String -Path $log -Pattern '\[AI HEROES UI\] Button action=200$').Count -le $priorRecruit) {
    throw 'ROUTE_QA_FAIL: independent recruitment-advisor control does not respond'
}
Write-Output 'RECRUITMENT_ADVICE_BUTTON_PASS'
$finalSaves = @(Get-SaveInventory)
if (Compare-Object -ReferenceObject $beforeFiles -DifferenceObject $finalSaves) {
    throw 'ROUTE_QA_FAIL: advisor-only button changed campaign files'
}
Write-Output 'ADVISOR_ACTIONS_READ_ONLY_PASS'
Write-Output 'ROUTE_CLICK_ACTION_PASS'
Write-Output "ROUTE_LOG=$log"
