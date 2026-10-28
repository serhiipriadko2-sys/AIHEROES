param(
    [string]$LogTag = 'AIHeroesSlotsQA.log',
    [string]$OutputPath = 'C:\github\AIHEROES\Saved\Screenshots\AIHeroesSlotsQA.png'
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct QARect { public int Left, Top, Right, Bottom; }
public static class QAGameCapture {
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out QARect r);
 [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
}
'@
$candidates = @(Get-CimInstance Win32_Process |
    Where-Object { $_.Name -eq 'UnrealEditor.exe' -and
        $_.CommandLine -like "*AIHeroes.uproject*" -and
        $_.CommandLine -like "*-log=$LogTag*" })
if ($candidates.Count -ne 1) { throw "Expected exactly one owned QA window; found $($candidates.Count)" }
$game = Get-Process -Id $candidates[0].ProcessId -ErrorAction Stop
if ($game.MainWindowHandle -eq [IntPtr]::Zero) { throw 'Game window unavailable' }
$rect = New-Object QARect
if (-not [QAGameCapture]::GetWindowRect($game.MainWindowHandle, [ref]$rect)) { throw 'Window bounds unavailable' }
$width = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top
if ($width -lt 800 -or $height -lt 500) { throw 'Unexpected game window size' }
$bitmap = New-Object System.Drawing.Bitmap($width, $height)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$dc = $graphics.GetHdc()
try { $result = [QAGameCapture]::PrintWindow($game.MainWindowHandle, $dc, 2) }
finally { $graphics.ReleaseHdc($dc); $graphics.Dispose() }
if (-not $result) { $bitmap.Dispose(); throw 'Game window capture rejected' }
New-Item -ItemType Directory -Path (Split-Path $OutputPath) -Force | Out-Null
$bitmap.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)
$bitmap.Dispose()
$info = Get-Item -LiteralPath $OutputPath
Write-Output ("QA_CAPTURE_PASS pid={0} dimensions={1}x{2} bytes={3} sha256={4}" -f
    $game.Id,$width,$height,$info.Length,(Get-FileHash -LiteralPath $OutputPath -Algorithm SHA256).Hash)
