param([string]$Root = "C:\github\AIHEROES")
$ErrorActionPreference = "Stop"
$files = @(git -C $Root ls-files --cached --others --exclude-standard | Sort-Object -Unique)
if ($LASTEXITCODE -ne 0) { throw "Git inventory failed" }

$records = @()
foreach ($rel in $files) {
    $absolute = Join-Path $Root $rel
    $item = Get-Item -LiteralPath $absolute -ErrorAction Stop
    if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Symlink/reparse point: $rel" }
    if ($rel -match '(?i)(^|/)(\.env|mcp\.json|id_rsa|[^/]*\.pem|[^/]*\.key|[^/]*secret[^/]*)$') {
        throw "Potentially sensitive path in Git inventory: $rel"
    }
    $records += [PSCustomObject]@{
        path = $rel.Replace('\\','/')
        bytes = [long]$item.Length
        sha256 = (Get-FileHash -LiteralPath $absolute -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}
$collisions = @($records | Group-Object { $_.path.ToLowerInvariant() } | Where-Object { $_.Count -gt 1 })
if ($collisions.Count -gt 0) { throw "Case-insensitive path collision" }
if ($records.Count -lt 20) { throw "Unexpectedly small candidate tree" }

$project = Get-Content -LiteralPath (Join-Path $Root 'AIHeroes.uproject') -Raw | ConvertFrom-Json
if ($project.EngineAssociation -ne '5.8' -or $project.Modules.Count -lt 1) {
    throw "Unreal project metadata invalid"
}
Add-Type -AssemblyName System.Drawing
foreach ($relativeImage in @('Docs\PROTOTYPE_SCREENSHOT.png','Docs\BATTLE_RESULT.png')) {
    $pic = [System.Drawing.Image]::FromFile((Join-Path $Root $relativeImage))
    try { if ($pic.Width -lt 800 -or $pic.Height -lt 500) { throw "Image too small: $relativeImage" } }
    finally { $pic.Dispose() }
}

$dir = Join-Path $Root 'Saved\QA'
New-Item -ItemType Directory -Path $dir -Force | Out-Null
$manifest = Join-Path $dir 'repo_manifest.json'
[PSCustomObject]@{
    inspected_at = (Get-Date).ToString('yyyy-MM-ddTHH:mm:sszzz')
    surface = 'DESKTOP-ITUAMNF'
    status = 'LOCAL_QA_PASS'
    tracked_or_untracked = 'git-visible-files'
    files = @($records)
} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifest -Encoding utf8

$allBytes = ($records | Measure-Object -Property bytes -Sum).Sum
$manifestHash = (Get-FileHash -LiteralPath $manifest -Algorithm SHA256).Hash.ToLowerInvariant()
Write-Output "PASS | files=$($records.Count) | total_bytes=$allBytes"
Write-Output "manifest=$manifest"
Write-Output "manifest_bytes=$((Get-Item $manifest).Length)"
Write-Output "manifest_sha256=$manifestHash"
Write-Output "no symlinks, case collisions, or secret-like paths; Unreal JSON and PNGs validated"
