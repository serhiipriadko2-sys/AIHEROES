param(
    [ValidateRange(0,2147483647)][int]$Seed = 20261028,
    [ValidateRange(0,4)][int]$Branches = 3,
    [switch]$KeepOpen
)
# Runs a standalone GameMode and requests a screenshot from the GPU via
# Unreal's FScreenshotRequest. No desktop capture or global keyboard/mouse.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct ZonePreviewClientRect { public int Left; public int Top; public int Right; public int Bottom; }
public static class ZonePreviewClient {
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out ZonePreviewClientRect r);
}
'@

$Root = Split-Path -Parent $PSScriptRoot
$Stamp = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
$Output = Join-Path $Root ('Saved\QA\ZonePreview_' + $Stamp)
New-Item -ItemType Directory -Path $Output -ErrorAction Stop | Out-Null
$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$Project = Join-Path $Root 'AIHeroes.uproject'
$Log = Join-Path $Output 'visual.log'
$Screenshot = Join-Path $Output 'preview.png'
$ExpectedMode = '/Script/AIHeroes.AIHeroesZonePreviewGameMode'
if (!(Test-Path $Editor) -or !(Test-Path $Project)) {
    throw 'Missing installed Unreal Engine 5.8 or AI HEROES project.'
}
$ArgsList = @($Project,('/Engine/Maps/Entry?game=' + $ExpectedMode),
    '-game','-windowed','-ResX=1280','-ResY=720','-NoSplash',
    '-NoP4','-NoSound',('-ZoneSeed=' + $Seed),('-ZoneBranches=' + $Branches),
    '-ZoneCapture',('-abslog=' + $Log))
$Game = Start-Process -FilePath $Editor -ArgumentList $ArgsList -PassThru
Write-Output ('ZONE_PREVIEW_PROCESS_PID=' + $Game.Id + ' OWN_OUTPUT=' + $Output)

# Even failed/blank captures must not leave an orphan game window.
try {
$Ready = $false
for ($I = 0; $I -lt 150; $I++) {
    $Game.Refresh()
    if ($Game.HasExited) { throw ('ZONE_PREVIEW_GAME_CRASH exit=' + $Game.ExitCode) }
    if (Test-Path $Log) {
        $Failed = Select-String -LiteralPath $Log -Pattern '\[AI HEROES ZONE PREVIEW\] (FAIL|BLOCKED)' -ErrorAction SilentlyContinue | Select-Object -Last 1
        if ($Failed) { throw ('ZONE_PREVIEW_LOG_FAIL ' + $Failed.Line) }
        $Pass = Select-String -LiteralPath $Log -Pattern '\[AI HEROES ZONE PREVIEW\] PASS seed=' -ErrorAction SilentlyContinue | Select-Object -Last 1
        if ($Pass) { $Ready = $true; Write-Output $Pass.Line; break }
    }
    Start-Sleep -Milliseconds 400
}
if (!$Ready) { throw 'ZONE_PREVIEW_TIMEOUT: no isolated GameMode PASS log' }
$Proc = Get-Process -Id $Game.Id -ErrorAction Stop
$Identity = (Get-CimInstance Win32_Process -Filter ("ProcessId=" + $Game.Id)).CommandLine
if ($Identity -notmatch 'AIHeroesZonePreviewGameMode' -or
    $Identity -notmatch 'ZoneSeed=' -or $Identity -notmatch 'ZoneBranches=' -or
    $Identity -notmatch 'ZoneCapture' -or $Identity -notmatch '-game' -or
    $Identity -notmatch [regex]::Escape($Stamp) -or
    $Proc.MainWindowHandle -eq [IntPtr]::Zero) {
    throw 'ZONE_PREVIEW_WINDOW_IDENTITY_FAIL'
}
$Client = New-Object ZonePreviewClientRect
if (-not [ZonePreviewClient]::GetClientRect($Proc.MainWindowHandle,[ref]$Client) -or
    $Client.Right -ne 1280 -or $Client.Bottom -ne 720) {
    throw ('ZONE_PREVIEW_CLIENT_NOT_1280_720 ' + $Client.Right + 'x' + $Client.Bottom)
}

# Win32 PrintWindow/BitBlt return blank D3D content for this UE 5.8
# viewport. Wait for the viewport-native FScreenshotRequest instead.
$NativePath = $null
for ($I = 0; $I -lt 120; $I++) {
    $Hit = Select-String -LiteralPath $Log -Pattern '\[AI HEROES ZONE PREVIEW\] SCREENSHOT_REQUEST path=' -ErrorAction SilentlyContinue | Select-Object -Last 1
    if ($Hit) {
        $NativePath = ($Hit.Line -replace '^.*SCREENSHOT_REQUEST path=', '').Trim()
        break
    }
    if ($Game.HasExited) { throw 'ZONE_PREVIEW_EXITED_BEFORE_GPU_CAPTURE' }
    Start-Sleep -Milliseconds 300
    $Game.Refresh()
}
if (!$NativePath) { throw 'ZONE_PREVIEW_NO_ENGINE_SCREENSHOT_REQUEST' }
$Allowed = [IO.Path]::GetFullPath((Join-Path $Root 'Saved\QA\ZonePreviewFrames'))
# Unreal logs paths relative to its Engine/Binaries working directory.
# Trust only the exact filename suffix in its engine-owned QA folder;
# never use a possibly traversal-containing log path as a filesystem target.
if ($NativePath -notmatch 'Saved[/\\]QA[/\\]ZonePreviewFrames[/\\]ZonePreview_[0-9a-fA-F]{32}[.]png$') {
    throw ('ZONE_PREVIEW_CAPTURE_PATH_NOT_ALLOWED ' + $NativePath)
}
$Filename = [IO.Path]::GetFileName(($NativePath -replace '/', '\'))
$Resolved = [IO.Path]::GetFullPath((Join-Path $Allowed $Filename))
if (!$Resolved.StartsWith($Allowed + '\',[StringComparison]::OrdinalIgnoreCase)) {
    throw ('ZONE_PREVIEW_CAPTURE_PATH_NOT_ALLOWED ' + $Resolved)
}
$ScreenshotExists = $false
for ($I = 0; $I -lt 120; $I++) {
    if ((Test-Path $Resolved) -and (Get-Item $Resolved).Length -gt 12000) {
        $ScreenshotExists = $true
        break
    }
    Start-Sleep -Milliseconds 250
}
if (!$ScreenshotExists) { throw ('ZONE_PREVIEW_GPU_SCREENSHOT_MISSING ' + $Resolved) }
$Image = [System.Drawing.Bitmap]::FromFile($Resolved)
try {
    if ($Image.Width -ne 1280 -or $Image.Height -ne 720) {
        throw ('ZONE_PREVIEW_GPU_FRAME_SIZE_FAIL ' + $Image.Width + 'x' + $Image.Height)
    }
    $Colors = New-Object 'System.Collections.Generic.HashSet[int]'
    for ($Y = 28; $Y -lt $Image.Height; $Y += 29) {
        for ($X = 28; $X -lt $Image.Width; $X += 29) {
            [void]$Colors.Add($Image.GetPixel($X,$Y).ToArgb())
        }
    }
    if ($Colors.Count -lt 8) {
        throw ('ZONE_PREVIEW_BLANK_GPU_FRAME colors=' + $Colors.Count)
    }
} finally {
    $Image.Dispose()
}
Copy-Item -LiteralPath $Resolved -Destination $Screenshot -ErrorAction Stop
$NativeHash = (Get-FileHash $Resolved -Algorithm SHA256).Hash.ToLower()
$CopiedHash = (Get-FileHash $Screenshot -Algorithm SHA256).Hash.ToLower()
if ($NativeHash -ne $CopiedHash) { throw 'ZONE_PREVIEW_SCREENSHOT_COPY_DRIFT' }
if (Select-String -LiteralPath $Log -Pattern '\[AI HEROES\] Select a node|SAVED TO SLOT|LIVE BATTLE COMMIT' -ErrorAction SilentlyContinue) {
    throw 'ZONE_PREVIEW_WRONG_GAMEMODE_OR_CAMPAIGN_WRITE'
}
$Result = [ordered]@{
    status = 'PASS_ISOLATED_3D_ZONE_PREVIEW'
    seed = $Seed
    branches = $Branches
    game_mode = $ExpectedMode
    pid = $Game.Id
    exact_client = '1280x720'
    engine_viewport_capture = $true
    sampled_unique_colors = $Colors.Count
    screenshot_bytes = (Get-Item $Screenshot).Length
    screenshot_sha256 = $CopiedHash
    source = 'AIHeroesZoneTemplate C++17 (Unreal adapter)'
    campaign_save_writes = 0
    player_action_bindings = 'none'
    preview_log = $Log
    screenshot = $Screenshot
    observed_utc = [DateTime]::UtcNow.ToString('o')
}
$Result | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $Output 'receipt.json') -Encoding UTF8
Write-Output ('ZONE_PREVIEW_VISUAL_QA_PASS engine_gpu_frame_colors=' + $Colors.Count +
    ' screenshot_bytes=' + (Get-Item $Screenshot).Length)
Write-Output ('ZONE_PREVIEW_QA_RECEIPT=' + (Join-Path $Output 'receipt.json'))
if ($KeepOpen) {
    Write-Output ('PREVIEW_WINDOW_LEFT_OPEN_FOR_USER_PID=' + $Game.Id)
}
} finally {
    if (!$KeepOpen -and $Game) {
        $Owned = Get-CimInstance Win32_Process -Filter ('ProcessId=' + $Game.Id) -ErrorAction SilentlyContinue
        if ($Owned -and $Owned.CommandLine -match [regex]::Escape($Stamp) -and
            $Owned.CommandLine -match 'AIHeroesZonePreviewGameMode') {
            $ProcToClose = Get-Process -Id $Game.Id -ErrorAction SilentlyContinue
            if ($ProcToClose) {
                $Closed = $ProcToClose.CloseMainWindow()
                Write-Output ('OWN_WINDOW_GRACEFUL_CLOSE_SENT=' + $Closed)
            }
        }
    }
}
