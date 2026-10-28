# UE 5.8 requirements checked 2026-10-28: https://dev.epicgames.com/documentation/unreal-engine/android-development-requirements-for-unreal-engine
# Read-only prerequisites audit for the installed UE 5.8 Android toolchain.
# No SDK downloads, environment changes or global installation.
$ErrorActionPreference = 'Stop'
$sdk = if ($env:ANDROID_HOME) { $env:ANDROID_HOME } elseif ($env:ANDROID_SDK_ROOT) { $env:ANDROID_SDK_ROOT } else { Join-Path $env:LOCALAPPDATA 'Android\Sdk' }
$ndkFolder = Join-Path $sdk 'ndk'
$apiFolder = Join-Path $sdk 'platforms\android-35'
$buildTools = Join-Path $sdk 'build-tools\35.0.1'
$platformTools = Join-Path $sdk 'platform-tools'
$studioJava = 'C:\Program Files\Android\Android Studio\jbr\bin\java.exe'
$ndkFound = $false
if (Test-Path $ndkFolder) {
    $ndkFound = (Get-ChildItem $ndkFolder -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -like '27.2.*' }).Count -gt 0
}
$checks = @(
    [PSCustomObject]@{ Name='Android SDK root'; Ready=(Test-Path $sdk); Evidence=$sdk },
    [PSCustomObject]@{ Name='SDK API 35 platform'; Ready=(Test-Path $apiFolder); Evidence=$apiFolder },
    [PSCustomObject]@{ Name='Build tools 35.0.1 (UE 5.8)'; Ready=(Test-Path $buildTools); Evidence=$buildTools },
    [PSCustomObject]@{ Name='Platform tools'; Ready=(Test-Path $platformTools); Evidence=$platformTools },
    [PSCustomObject]@{ Name='NDK r27c folder'; Ready=$ndkFound; Evidence=$ndkFolder },
    [PSCustomObject]@{ Name='Android Studio JBR'; Ready=(Test-Path $studioJava); Evidence=$studioJava }
)
foreach ($c in $checks) {
    $s = if ($c.Ready) { 'PASS' } else { 'BLOCKED' }
    Write-Output ('{0} | {1} | {2}' -f $s, $c.Name, $c.Evidence)
}
if (@($checks | Where-Object { -not $_.Ready }).Count -gt 0) {
    Write-Output 'ANDROID_APK_GATE=BLOCKED (read-only environment audit)'
    exit 2
}
Write-Output 'ANDROID_PREREQUISITES_PRESENT (actual Unreal Turnkey and device QA still required)'
