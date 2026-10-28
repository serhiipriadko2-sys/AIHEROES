param(
    [Parameter(Mandatory = $true)][int]$ExpectedPid,
    [string]$LogTag = 'AIHeroesProtocolLabQA.log'
)
# Isolated GUI smoke. All input is PostMessage to a *verified* -game window.
# Never touches foreground app, player saves or desktop input.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$remote = Get-CimInstance Win32_Process -Filter "ProcessId=$ExpectedPid"
if (-not $remote -or $remote.Name -ne 'UnrealEditor.exe' -or
    $remote.CommandLine -notlike '*AIHeroes.uproject*' -or
    $remote.CommandLine -notlike '*-game*' -or
    $remote.CommandLine -notlike "*-log=$LogTag*") {
    throw 'LAB_QA_BLOCKED: game process is not the dedicated laboratory window'
}
$game = Get-Process -Id $ExpectedPid -ErrorAction Stop
if ($game.MainWindowHandle -eq [IntPtr]::Zero) {
    throw 'LAB_QA_BLOCKED: dedicated game has no visible window'
}
$log = Join-Path $root ('Saved\Logs\' + $LogTag)
if (-not (Test-Path -LiteralPath $log)) {
    throw 'LAB_QA_BLOCKED: dedicated log does not exist'
}
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct LabClientRect {public int Left,Top,Right,Bottom;}
public static class LabWindowMessages {
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hwnd,out LabClientRect rect);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hwnd,uint msg,IntPtr w,IntPtr l);
}
'@
$screen=New-Object LabClientRect
if(-not [LabWindowMessages]::GetClientRect($game.MainWindowHandle,[ref]$screen) -or
   $screen.Right -ne 1280 -or $screen.Bottom -ne 720) {
    throw "LAB_QA_BLOCKED: unexpected viewport $($screen.Right)x$($screen.Bottom)"
}
function Get-SaveInventory {
    $dir=Join-Path $root 'Saved\SaveGames'
    if(-not (Test-Path $dir)){return @()}
    return @(Get-ChildItem $dir -File|Sort-Object Name|ForEach-Object {
        $_.Name+'|'+$_.Length+'|'+$_.LastWriteTimeUtc.Ticks+'|'+
        (Get-FileHash $_.FullName -Algorithm SHA256).Hash
    })
}
$before=@(Get-SaveInventory)
function SendKey([int]$key) {
    if(-not [LabWindowMessages]::PostMessage($game.MainWindowHandle,0x0100,[IntPtr]$key,[IntPtr]0)) {
        throw 'LAB_QA_FAIL: refused WM_KEYDOWN'
    }
    Start-Sleep -Milliseconds 150
    if(-not [LabWindowMessages]::PostMessage($game.MainWindowHandle,0x0101,[IntPtr]$key,[IntPtr]0)) {
        throw 'LAB_QA_FAIL: refused WM_KEYUP'
    }
    Start-Sleep -Milliseconds 430
}
function Click([int]$x,[int]$y) {
    $xy=[IntPtr](($y -shl 16)-bor $x)
    foreach($message in @(0x0200,0x0201,0x0202)){
        $flags=if($message -eq 0x0201){[IntPtr]1}else{[IntPtr]0}
        if(-not [LabWindowMessages]::PostMessage($game.MainWindowHandle,$message,$flags,$xy)){
            throw 'LAB_QA_FAIL: refused targeted mouse event'
        }
        Start-Sleep -Milliseconds 120
    }
    Start-Sleep -Milliseconds 430
}
function UntilLog([string]$pattern,[int]$seconds=18) {
    $end=(Get-Date).AddSeconds($seconds)
    while((Get-Date) -lt $end) {
        $match=Select-String -LiteralPath $log -Pattern $pattern -ErrorAction SilentlyContinue|Select-Object -Last 1
        if($match){Write-Output $match.Line;return}
        if(-not (Get-Process -Id $ExpectedPid -ErrorAction SilentlyContinue)){
            throw 'LAB_QA_FAIL: game crashed'
        }
        Start-Sleep -Milliseconds 300
    }
    throw ('LAB_QA_FAIL: expected evidence missing: '+$pattern)
}
function Capture([string]$name) {
    $path=Join-Path $root ('Saved\QA\ProtocolLab\' + $name)
    & (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $LogTag -OutputPath $path
    if(-not (Test-Path $path) -or (Get-Item $path).Length -lt 10000){
        throw ('LAB_QA_FAIL: screenshot missing or empty: '+$name)
    }
}
Write-Output "LAB_QA_PINNED pid=$ExpectedPid screen=1280x720"
Click 1040 475 # LAB Y touch button, beside GOALS O.
UntilLog '\[AI HEROES UI\] Button action=203'
UntilLog '\[AI HEROES LAB\] OPEN verified=0 variants=0'
Capture 'LAB_NO_HISTORY.png'
SendKey 0x1B # escape: close read-only dossier, never alter map.
UntilLog '\[AI HEROES DOSSIER\] CLOSE no_order=1'
Click 435 377 # current scenario forest map node.
UntilLog '\[AI HEROES\] FOREST selected'
SendKey 0x0D # explicit human attack authorization
UntilLog '\[AI HEROES LIVE\] BEGIN node=FOREST'
SendKey 0x42 # fast-finish live fight
UntilLog '\[AI HEROES LIVE\] COMMIT node=FOREST'
SendKey 0x0D # leave real battle and show map
UntilLog '\[AI HEROES\] Returned to map'
$commitsBefore=@(Select-String -LiteralPath $log -Pattern '\[AI HEROES LIVE\] COMMIT node=FOREST').Count
if($commitsBefore -ne 1){throw ('LAB_QA_FAIL: expected exactly one real combat commit: '+$commitsBefore)}
SendKey 0x59 # Y: protocol lab; must not replay combat
UntilLog '\[AI HEROES LAB\] OPEN verified=1 variants=4 history_version=2 no_order=1'
Capture 'LAB_FOUR_PROTOCOLS.png'
# Four tiers occupy two readable report pages at 1280x720.
# NEXT proves the last counterfactual row and authority disclaimers are reachable.
Click 519 626
UntilLog '\[AI HEROES DOSSIER\] PAGE 2/2'
Capture 'LAB_FOUR_PROTOCOLS_PAGE2.png'
# A dossier refresh must preserve type=lab, rather than silently switching
# to the generic strategic advice or invoking an AI order.
$refreshBefore=@(Select-String -LiteralPath $log -Pattern '\[AI HEROES DOSSIER\] REFRESH').Count
SendKey 0x1B
UntilLog '\[AI HEROES DOSSIER\] CLOSE no_order=1'
SendKey 0x59
Start-Sleep -Milliseconds 350
$opens=@(Select-String -LiteralPath $log -Pattern '\[AI HEROES LAB\] OPEN verified=1 variants=4 history_version=2 no_order=1').Count
if($opens -ne 2){throw ('LAB_QA_FAIL: repeated analysis lost history or format: '+$opens)}
SendKey 0x1B
$commitsAfter=@(Select-String -LiteralPath $log -Pattern '\[AI HEROES LIVE\] COMMIT node=FOREST').Count
if($commitsAfter -ne 1){throw 'LAB_QA_FAIL: read-only laboratory applied combat a second time'}
$after=@(Get-SaveInventory)
if(Compare-Object -ReferenceObject $before -DifferenceObject $after){
    throw 'LAB_QA_FAIL: SaveGame bytes or metadata changed'
}
Write-Output 'LAB_QA_PASS: no history -> one real battle -> four verifiable policy outcomes -> repeated read-only view'
Write-Output 'LAB_SAVES_UNCHANGED_PASS'
Write-Output "LAB_QA_LOG=$log"
