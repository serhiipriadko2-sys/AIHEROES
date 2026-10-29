param(
    [ValidateRange(0,2147483647)][int]$Seed = 20261029,
    [ValidateRange(0,4)][int]$Branches = 3,
    [switch]$KeepOpen
)
# Launch a uniquely tagged opt-in game and accept ONLY engine GPU frames.
# Never press K/L, call save APIs, or modify existing user campaign slots.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class AIHeroesExpeditionQAKeys {
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
}
'@
$Root = Split-Path -Parent $PSScriptRoot
$Stamp = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$Tag = 'AIHeroesZoneDossierQA_' + $Stamp + '.log'
$Log = Join-Path $Root ('Saved\Logs\' + $Tag)
$QaDir = Join-Path $Root ('Saved\QA\ZoneExpedition_' + $Stamp)
$FrameDir = Join-Path $Root 'Saved\QA\ZoneExpeditionFrames'
New-Item -ItemType Directory -Path $QaDir -ErrorAction Stop | Out-Null
$SaveDir = Join-Path $Root 'Saved\SaveGames'
function SaveHashes {
    $Values = @{}
    if (Test-Path $SaveDir) {
        Get-ChildItem -LiteralPath $SaveDir -File | ForEach-Object {
            $Values[$_.FullName] = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        }
    }
    return $Values
}
$Before = SaveHashes
$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$Mode = 'AIHeroesZoneExpeditionGameMode'
$Project = Join-Path $Root 'AIHeroes.uproject'
if (-not (Test-Path $Editor) -or -not (Test-Path $Project)) {
    throw 'No installed game editor or .uproject'
}
$ArgsList = @(
    $Project,('/Engine/Maps/Entry?game=/Script/AIHeroes.'+$Mode),
    '-game','-windowed','-ResX=1280','-ResY=720','-NoSplash',
    '-NoP4','-NoSound',('-ZoneSeed='+$Seed),('-ZoneBranches='+$Branches),
    '-ZoneExpeditionCapture',('-log='+$Tag)
)
$Game = Start-Process -FilePath $Editor -PassThru -ArgumentList $ArgsList
Write-Output ('EXPEDITION_QA_OWN_PID='+$Game.Id+' dir='+$QaDir)
function AwaitLog([string]$Regex,[int]$Checks=160) {
    for($i=0;$i -lt $Checks;$i++) {
        $Game.Refresh()
        if($Game.HasExited) { throw ('QA_GAME_CRASH exit='+$Game.ExitCode) }
        if(Test-Path $Log) {
            $Bad=Select-String -LiteralPath $Log -Pattern '\[ZONE EXPEDITION\] (BLOCKED|QA_CAPTURE_DIRECTORY_FAILED)' |
                Select-Object -Last 1
            if($Bad) { throw ('QA_LOG_FATAL '+$Bad.Line) }
            $Hit=Select-String -LiteralPath $Log -Pattern $Regex|Select-Object -Last 1
            if($Hit) { Write-Host $Hit.Line;return $Hit.Line }
        }
        Start-Sleep -Milliseconds 300
    }
    throw ('QA_TIMEOUT '+$Regex)
}
function PostKey([int]$Vk) {
    $Proc=Get-Process -Id $Game.Id -ErrorAction Stop
    $Identity=(Get-CimInstance Win32_Process -Filter ('ProcessId='+$Game.Id)).CommandLine
    if($Proc.MainWindowHandle -eq [IntPtr]::Zero -or
       $Identity -notlike ('*-log='+$Tag+'*') -or
       $Identity -notlike ('*'+$Mode+'*')) { throw 'QA_WRONG_TARGET_WINDOW' }
    if(-not [AIHeroesExpeditionQAKeys]::PostMessage($Proc.MainWindowHandle,0x100,[IntPtr]$Vk,[IntPtr]0)) {
        throw 'QA_KEY_DOWN_FAILED'
    }
    Start-Sleep -Milliseconds 220
    if(-not [AIHeroesExpeditionQAKeys]::PostMessage($Proc.MainWindowHandle,0x101,[IntPtr]$Vk,[IntPtr]0)) {
        throw 'QA_KEY_UP_FAILED'
    }
    Start-Sleep -Milliseconds 600
}
$FrameReceipts = @()
function CaptureFrame([int]$Serial,[string]$Name) {
    $Line = AwaitLog ('\[ZONE EXPEDITION\] SCREENSHOT_REQUEST serial='+$Serial+' path=')
    $Raw = ($Line -replace '^.*SCREENSHOT_REQUEST serial=[0-9]+ path=','').Trim()
    $ExpectedFilenamePattern='^Expedition_'+('{0:D2}' -f $Serial)+'_[0-9a-fA-F]{32}\.png$'
    $Filename=[IO.Path]::GetFileName($Raw.Replace('/','\'))
    if($Filename -notmatch $ExpectedFilenamePattern -or
       $Raw -notmatch '[\\/]Saved[\\/]QA[\\/]ZoneExpeditionFrames[\\/]') {
        throw ('QA_UNTRUSTED_SCREENSHOT_PATH '+$Raw)
    }
    $Source=[IO.Path]::GetFullPath((Join-Path $FrameDir $Filename))
    $Allowed=[IO.Path]::GetFullPath($FrameDir).TrimEnd('\')+'\'
    if(-not $Source.StartsWith($Allowed,[StringComparison]::OrdinalIgnoreCase)) {
        throw 'QA_SCREENSHOT_OUTSIDE_FRAME_DIR'
    }
    $Found=$false
    for($i=0;$i -lt 140;$i++) {
        if((Test-Path $Source) -and (Get-Item $Source).Length -gt 12000) {
            $Found=$true;break
        }
        Start-Sleep -Milliseconds 250
    }
    if(-not $Found) { throw ('QA_GPU_FRAME_MISSING '+$Source) }
    $Image=[System.Drawing.Bitmap]::FromFile($Source)
    $Colors=New-Object 'System.Collections.Generic.HashSet[int]'
    try {
        if($Image.Width -ne 1280 -or $Image.Height -ne 720) {
            throw ('QA_WRONG_FRAME_DIMENSIONS '+$Image.Width+'x'+$Image.Height)
        }
        for($y=25;$y -lt $Image.Height;$y+=27) {
            for($x=25;$x -lt $Image.Width;$x+=29) {
                [void]$Colors.Add($Image.GetPixel($x,$y).ToArgb())
            }
        }
        if($Colors.Count -lt 8) { throw ('QA_BLANK_GPU_FRAME unique_colors='+$Colors.Count) }
    } finally { $Image.Dispose() }
    $Target=Join-Path $QaDir ($Name+'.png')
    Copy-Item -LiteralPath $Source -Destination $Target -ErrorAction Stop
    $A=(Get-FileHash -LiteralPath $Source -Algorithm SHA256).Hash
    $B=(Get-FileHash -LiteralPath $Target -Algorithm SHA256).Hash
    if($A -ne $B) { throw 'QA_IMAGE_COPY_HASH_DRIFT' }
    $Info=Get-Item -LiteralPath $Target
    $Receipt=[ordered]@{
       stage=$Name; serial=$Serial; screenshot=$Target;
       bytes=$Info.Length; sha256=$B.ToLowerInvariant();
       width=1280; height=720; unique_sampled_colors=$Colors.Count
    }
    Write-Host ('EXPEDITION_GPU_FRAME_PASS stage='+$Name+' bytes='+$Info.Length+
        ' colors='+$Colors.Count+' sha256='+$B)
    return [pscustomobject]$Receipt
}
try {
    [void](AwaitLog '\[ZONE EXPEDITION\] READY seed=')
    $FrameReceipts+=CaptureFrame 0 'initial'
    PostKey 0x48  # Offline H: read-only suggestion, NOT an order.
    $AdvisorLine=AwaitLog '\[ZONE EXPEDITION\] READ-ONLY HERO ADVICE \| HERO'
    if($AdvisorLine -match 'RUINS|OUTPOST|PASS|WAY_0') {
        throw ('ADVISOR_LEAKED_FOGGED_SITE '+$AdvisorLine)
    }
    if($AdvisorLine -notmatch 'Select it yourself|Human ENTER required') {
        throw ('ADVISOR_DID_NOT_PRESERVE_HUMAN_AUTHORITY '+$AdvisorLine)
    }
    if(Select-String -LiteralPath $Log -Pattern '\[ZONE EXPEDITION\] PLAYER CONFIRMED|\[ZONE EXPEDITION\] EXPEDITION SAVED' -ErrorAction SilentlyContinue) {
        throw 'ADVISOR_H_ISSUED_A_HUMAN_ORDER_OR_SAVED'
    }
    # Send left Shift and H to this exact game window only. No desktop focus.
    $Inspector=Get-Process -Id $Game.Id -ErrorAction Stop
    $Identity=(Get-CimInstance Win32_Process -Filter ('ProcessId='+$Game.Id)).CommandLine
    if($Inspector.MainWindowHandle -eq [IntPtr]::Zero -or
       $Identity -notlike ('*-log='+$Tag+'*') -or
       $Identity -notlike ('*'+$Mode+'*')) { throw 'ADVISOR_INSPECT_WRONG_WINDOW' }
    $Hwnd=$Inspector.MainWindowHandle
    if(-not [AIHeroesExpeditionQAKeys]::PostMessage($Hwnd,0x100,[IntPtr]0xA0,[IntPtr]0x002A0001)) {
        throw 'ADVISOR_SHIFT_DOWN_FAILED'
    }
    try {
        Start-Sleep -Milliseconds 220
        if(-not [AIHeroesExpeditionQAKeys]::PostMessage($Hwnd,0x100,[IntPtr]0x48,[IntPtr]0x00230001)) {
            throw 'ADVISOR_H_DOWN_FAILED'
        }
        Start-Sleep -Milliseconds 260
        [void][AIHeroesExpeditionQAKeys]::PostMessage($Hwnd,0x101,[IntPtr]0x48,([IntPtr]([long]0xC0230001)))
    } finally {
        [void][AIHeroesExpeditionQAKeys]::PostMessage($Hwnd,0x101,[IntPtr]0xA0,([IntPtr]([long]0xC02A0001)))
    }
    $InspectionLine=AwaitLog '\[ZONE EXPEDITION\] READ-ONLY HERO ADVICE \| HERO SEED \| FOREST costs'
    if($InspectionLine -notmatch 'outcome UNKNOWN|Human ENTER required') {
        throw ('ADVISOR_INSPECTION_CLAIMS_UNVERIFIED_BATTLE '+$InspectionLine)
    }
    if(Select-String -LiteralPath $Log -Pattern '\[ZONE EXPEDITION\] PLAYER CONFIRMED|\[ZONE EXPEDITION\] EXPEDITION SAVED' -ErrorAction SilentlyContinue) {
        throw 'SHIFT_H_INSPECT_WRONGFULLY_COMMITTED_STATE'
    }
    PostKey 0x44  # D: user opens a read-only 3-option comparison.
    $DossierOpen=AwaitLog '\[ZONE EXPEDITION\] READ-ONLY DOSSIER OPEN options=3 known_choices=3 top=SUPPLY'
    if($DossierOpen -match 'PASS|OUTPOST|RUINS|WAY_') {
        throw ('DOSSIER_LEAKED_FOGGED_TARGET '+$DossierOpen)
    }
    $FrameReceipts+=CaptureFrame 1 'dossier'
    PostKey 0x0D  # Test that Enter is blocked WHILE the dossier is open.
    PostKey 0x20  # Test that next-day spending is also blocked.
    if(Select-String -LiteralPath $Log -Pattern '\[ZONE EXPEDITION\] PLAYER CONFIRMED|\[ZONE EXPEDITION\] DAY 2' -ErrorAction SilentlyContinue) {
        throw 'DOSSIER_ALLOWED_MUTATING_INPUT'
    }
    PostKey 0x1B  # Esc closes this snapshot without action.
    $DossierClose=AwaitLog '\[ZONE EXPEDITION\] READ-ONLY DOSSIER CLOSED \| no orders executed'
    PostKey 0x0D  # Only now does the HUMAN confirm FOREST battle
    [void](AwaitLog '\[ZONE EXPEDITION\] PLAYER CONFIRMED FOREST \| battle=1 won=1')
    $FrameReceipts+=CaptureFrame 2 'forest'
    PostKey 0x20  # Player ends day 1
    [void](AwaitLog '\[ZONE EXPEDITION\] DAY 2')
    $FrameReceipts+=CaptureFrame 3 'day2'
    PostKey 0x09  # Human cycles to visible ORE
    [void](AwaitLog '\[ZONE EXPEDITION\] Selected known zone ORE')
    PostKey 0x0D  # Player confirms ORE battle
    [void](AwaitLog '\[ZONE EXPEDITION\] PLAYER CONFIRMED ORE \| battle=1 won=1')
    $FrameReceipts+=CaptureFrame 4 'ore'

    $After=SaveHashes
    if($Before.Count -ne $After.Count) { throw 'QA_USER_SAVE_FILE_COUNT_CHANGED' }
    foreach($Path in $Before.Keys) {
        if(-not $After.ContainsKey($Path) -or $Before[$Path] -ne $After[$Path]) {
            throw ('QA_USER_SAVE_HASH_DRIFT '+$Path)
        }
    }
    $Result=[ordered]@{
       status='PASS_ZONE_EXPEDITION_DOSSIER_PLAYTHROUGH';
       seed=$Seed;branches=$Branches; pid=$Game.Id;
       generated_zones=7+$Branches;
       player_confirmed_battles=2;
       read_only_advice_events=2;
       read_only_dossier_events=2;
       denied_mutating_commands_while_open=2;
       dossier_open_log=$DossierOpen;
       dossier_close_log=$DossierClose;
       advice_result=$AdvisorLine;
       inspect_result=$InspectionLine;
       human_next_day=1;
       screenshots=$FrameReceipts;
       user_save_files_unchanged=$true;
       save_writes=0;
       observed_utc=[DateTime]::UtcNow.ToString('o');
    }
    $Receipt=Join-Path $QaDir 'receipt.json'
    [IO.File]::WriteAllText($Receipt,($Result|ConvertTo-Json -Depth 10),
        (New-Object Text.UTF8Encoding($false)))
    Write-Output ('ZONE_EXPEDITION_DOSSIER_GAME_QA_PASS receipt='+$Receipt+
        ' scenes=5 battles=2 dossier=1 no_save_writes=1')
    if($KeepOpen) { Write-Output ('OWN_WINDOW_LEFT_OPEN_PID='+$Game.Id) }
} finally {
    if(-not $KeepOpen -and $Game) {
        $Process=Get-CimInstance Win32_Process -Filter ('ProcessId='+$Game.Id) -ErrorAction SilentlyContinue
        if($Process -and $Process.CommandLine -like ('*-log='+$Tag+'*') -and
           $Process.CommandLine -like ('*'+$Mode+'*')) {
            $Window=Get-Process -Id $Game.Id -ErrorAction SilentlyContinue
            if($Window) {
                $Closed=$Window.CloseMainWindow()
                Write-Output ('OWN_QA_WINDOW_GRACEFUL_CLOSE='+$Closed)
            }
        }
    }
}
