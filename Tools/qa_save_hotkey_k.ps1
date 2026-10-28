# Dedicated QA only: verifies K save does not also toggle Unreal debug viewmode.
$ErrorActionPreference = 'Stop'
$root = 'C:\github\AIHEROES'
$slot = Join-Path $root 'Saved\SaveGames\AIHEROES_Campaign_Resilient_v1_Player3_A.sav'
$slotB = Join-Path $root 'Saved\SaveGames\AIHEROES_Campaign_Resilient_v1_Player3_B.sav'
$log = Join-Path $root 'Saved\Logs\AIHeroesHotkeyQA.log'
if ((Test-Path $slot) -or (Test-Path $slotB)) {
    throw 'QA_BLOCKED: User slot 3 already exists, no keys sent.'
}
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class SaveHotkeyQA {
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
}
'@
$matches = @(Get-CimInstance Win32_Process | Where-Object {
    $_.Name -eq 'UnrealEditor.exe' -and $_.CommandLine -like '*AIHeroes.uproject*' -and
    $_.CommandLine -like '*-log=AIHeroesHotkeyQA.log*'
})
if($matches.Count -ne 1){throw ('Expected a unique owned game: '+$matches.Count)}
$game=Get-Process -Id $matches[0].ProcessId -ErrorAction Stop
if($game.MainWindowHandle -eq [IntPtr]::Zero -or -not(Test-Path $log)){throw 'Game window or log missing'}
function Press([int]$key) {
    if (-not [SaveHotkeyQA]::PostMessage($game.MainWindowHandle,0x100,[IntPtr]$key,[IntPtr]0)){throw 'Key down failed'}
    Start-Sleep -Milliseconds 120
    if (-not [SaveHotkeyQA]::PostMessage($game.MainWindowHandle,0x101,[IntPtr]$key,[IntPtr]0)){throw 'Key up failed'}
    Start-Sleep -Milliseconds 400
}
Press 0x43 # C -> human slot 2
Press 0x43 # C -> human slot 3
Press 0x4B # K -> save into human slot 3
$saved = Select-String -LiteralPath $log -Pattern '\[AI HEROES\] SLOT 3: GAME SAVED \[A\]' | Select-Object -Last 1
if (-not $saved -or -not (Test-Path $slot)) {throw 'QA_FAIL: K did not save slot 3'}
$debug = @(Select-String -LiteralPath $log -Pattern 'Set new viewmode: ShaderComplexity')
if ($debug.Count -gt 0) {throw 'QA_FAIL: K or other key switched to ShaderComplexity'}
Press 0x4C # L -> load from slot 3
$loaded = Select-String -LiteralPath $log -Pattern '\[AI HEROES\] GAME LOADED SLOT 3' | Select-Object -Last 1
if (-not $loaded) {throw 'QA_FAIL: L failed to load the K save'}
& (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag 'AIHeroesHotkeyQA.log' -OutputPath (Join-Path $root 'Saved\Screenshots\AIHeroesSaveKVerified.png')
if (@(Select-String -LiteralPath $log -Pattern 'Set new viewmode: ShaderComplexity').Count -gt 0) {
    throw 'QA_FAIL: shader debug viewmode activated after K/L'
}
Write-Output $saved.Line
Write-Output $loaded.Line
Write-Output ('SAVE_K_LOAD_L_NORMAL_VIEW_PASS slot3_bytes='+(Get-Item $slot).Length)
