# Verify that a FUTURE NPC event is a warning, not reported as completed.
# Unique -log=AIHeroesRivalForecastV2QA.log window; no SaveGame writes.
$ErrorActionPreference = 'Stop'
$root='C:\github\AIHEROES'
$tag='AIHeroesRivalForecastV2QA.log'
$log=Join-Path $root ('Saved\Logs\'+$tag)
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class ForecastQAMessages {
 [DllImport("user32.dll")]
 public static extern bool PostMessage(IntPtr hWnd,uint msg,IntPtr wp,IntPtr lp);
}
'@
$matches=@(Get-CimInstance Win32_Process|Where-Object{
    $_.Name -eq 'UnrealEditor.exe' -and
    $_.CommandLine -like '*AIHeroes.uproject*' -and
    $_.CommandLine -like '*-log=AIHeroesRivalForecastV2QA.log*'
})
if($matches.Count -ne 1){throw "Unique game required, got $($matches.Count)"}
$game=Get-Process -Id $matches[0].ProcessId -ErrorAction Stop
if($game.MainWindowHandle -eq [IntPtr]::Zero -or -not(Test-Path $log)){throw 'No QA game window/log'}
for($day=2;$day -le 7;$day++){
    $pattern='\[AI HEROES\] Day '+$day+': income'
    if(Select-String -LiteralPath $log -Pattern $pattern -Quiet){continue}
    $confirmed=$false
    for($attempt=0;$attempt -lt 3;$attempt++){
        if(-not[ForecastQAMessages]::PostMessage($game.MainWindowHandle,0x100,[IntPtr]0x20,[IntPtr]0)){throw 'Space down rejected'}
        Start-Sleep -Milliseconds 200
        if(-not[ForecastQAMessages]::PostMessage($game.MainWindowHandle,0x101,[IntPtr]0x20,[IntPtr]0)){throw 'Space up rejected'}
        Start-Sleep -Milliseconds 500
        if(Select-String -LiteralPath $log -Pattern $pattern -Quiet){$confirmed=$true;break}
    }
    if(-not $confirmed){throw "Day transition $day not confirmed after retries"}
}
& (Join-Path $root 'Tools\qa_capture_targeted_game.ps1') -LogTag $tag -OutputPath (Join-Path $root 'Docs\RIVAL_FORECAST_FUTURE_TENSE_QA_2026-10-28.png')
if(Select-String -LiteralPath $log -Pattern '\[AI HEROES RIVAL\]' -Quiet){
    throw 'Rival executed before expected day 8'
}
Write-Output ('RIVAL_FUTURE_FORECAST_QA_PASS day7=1 future_day8=1 pid='+$game.Id)
