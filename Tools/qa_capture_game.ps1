# Capture only the UnrealEditor game window, never the entire desktop.
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct NativeRect { public int Left; public int Top; public int Right; public int Bottom; }
public static class NativeScreenshot {
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr handle, out NativeRect bounds);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr handle, IntPtr destDC, uint flags);
}
'@
$game = Get-Process -Name UnrealEditor -ErrorAction Stop |
    Where-Object { $_.MainWindowHandle -ne [IntPtr]::Zero } |
    Select-Object -First 1
if (-not $game) { throw "UnrealEditor window not found" }
$rect = New-Object NativeRect
if (-not [NativeScreenshot]::GetWindowRect($game.MainWindowHandle, [ref]$rect)) {
    throw "Could not read the game window bounds"
}
$width = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top
if ($width -le 0 -or $height -le 0) { throw "Invalid window bounds" }
$bmp = New-Object System.Drawing.Bitmap($width, $height)
$gfx = [System.Drawing.Graphics]::FromImage($bmp)
$dc = $gfx.GetHdc()
try { $ok = [NativeScreenshot]::PrintWindow($game.MainWindowHandle, $dc, 2) }
finally { $gfx.ReleaseHdc($dc); $gfx.Dispose() }
if (-not $ok) { $bmp.Dispose(); throw "PrintWindow was unavailable for this renderer" }
$path = "C:\github\AIHEROES\Saved\Screenshots\AIHeroesQA.png"
New-Item -Path (Split-Path $path) -ItemType Directory -Force | Out-Null
$bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
$sample = $bmp.GetPixel([Math]::Min(100,$width-1), [Math]::Min(100,$height-1))
$bmp.Dispose()
Write-Output "Saved window-only capture: $path | dimensions=$width x $height | pixel100=$sample"
