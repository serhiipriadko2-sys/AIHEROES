# Fixed-scope source provenance. This is a local QA receipt, not a signed attestation.
# Content assets, the installed engine and private .iskra canon are outside this manifest.
function Get-AIHeroesInputSnapshot {
    param([Parameter(Mandatory = $true)][string]$Root)
    $ResolvedRoot = (Resolve-Path -LiteralPath $Root -ErrorAction Stop).Path.TrimEnd('\', '/')
    $Project = Join-Path $ResolvedRoot 'AIHeroes.uproject'
    if (-not (Test-Path -LiteralPath $Project -PathType Leaf)) { throw 'AIHeroes project input is missing.' }
    if ((Get-Item -LiteralPath $Project).Attributes -band [IO.FileAttributes]::ReparsePoint) {
        throw 'Reparse-point project inputs are not supported.'
    }
    # Adding local plugins must first extend this explicit build-input contract.
    if (Test-Path -LiteralPath (Join-Path $ResolvedRoot 'Plugins')) {
        throw 'Local Plugins/ inputs are outside the current provenance scope; extend the manifest before building/testing.'
    }
    $Files = @((Get-Item -LiteralPath $Project))
    foreach ($Directory in @('Source', 'Config')) {
        $Path = Join-Path $ResolvedRoot $Directory
        if (-not (Test-Path -LiteralPath $Path -PathType Container)) { throw "Required input directory is missing: $Directory" }
        if ((Get-Item -LiteralPath $Path).Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw "Reparse-point input directories are not supported: $Directory"
        }
        $Items = @(Get-ChildItem -LiteralPath $Path -Recurse -Force -ErrorAction Stop)
        if (@($Items | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint }).Count -ne 0) {
            throw "Reparse-point build inputs are not supported: $Directory"
        }
        $Files += @($Items | Where-Object { -not $_.PSIsContainer })
    }
    $Inputs = @($Files | ForEach-Object {
        [pscustomobject][ordered]@{
            path = $_.FullName.Substring($ResolvedRoot.Length + 1).Replace('\', '/')
            bytes = [long]$_.Length
            sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256 -ErrorAction Stop).Hash.ToLowerInvariant()
        }
    } | Sort-Object -Property path)
    if (@($Inputs | Where-Object { $_.path -match '^Source/' }).Count -eq 0) { throw 'No source inputs found.' }
    return ,$Inputs
}

function Get-AIHeroesBinarySnapshot {
    param([Parameter(Mandatory = $true)][string]$Root)
    $RelativePath = 'Binaries/Win64/UnrealEditor-AIHeroes.dll'
    $Path = Join-Path $Root $RelativePath
    $Item = Get-Item -LiteralPath $Path -ErrorAction Stop
    if ($Item.PSIsContainer -or $Item.Length -eq 0 -or ($Item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw 'Compiled AIHeroes DLL is invalid.'
    }
    return [pscustomobject][ordered]@{
        path = $RelativePath
        bytes = [long]$Item.Length
        sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256 -ErrorAction Stop).Hash.ToLowerInvariant()
    }
}

function Assert-AIHeroesEntriesEqual {
    param([Parameter(Mandatory = $true)][object[]]$Expected,
          [Parameter(Mandatory = $true)][object[]]$Actual,
          [string]$Label = 'inputs')
    if ($Expected.Count -ne $Actual.Count) { throw "AIHEROES_PROVENANCE_MISMATCH $Label count changed." }
    $Seen = @{}
    for ($Index = 0; $Index -lt $Expected.Count; $Index++) {
        $A = $Expected[$Index]
        $B = $Actual[$Index]
        if ($null -eq $A -or $null -eq $A.path -or
            -not ($A.bytes -is [int] -or $A.bytes -is [long]) -or
            [string]$A.sha256 -notmatch '^[a-f0-9]{64}$' -or
            $Seen.ContainsKey([string]$A.path) -or [long]$A.bytes -lt 0) {
            throw "AIHEROES_PROVENANCE_INVALID malformed $Label entry."
        }
        $Seen[[string]$A.path] = $true
        if ($A.path -cne $B.path -or $A.bytes -ne $B.bytes -or $A.sha256 -cne $B.sha256) {
            throw "AIHEROES_PROVENANCE_MISMATCH $Label changed: $($B.path)"
        }
    }
}

function Assert-AIHeroesCompiledSnapshot {
    param([Parameter(Mandatory = $true)][object]$Manifest,
          [Parameter(Mandatory = $true)][string]$Root)
    if ($null -eq $Manifest -or -not ($Manifest.schema -is [int] -or $Manifest.schema -is [long]) -or
        $Manifest.schema -ne 1 -or $Manifest.status -cne 'compiled' -or
        $Manifest.target -cne 'AIHeroesEditor Win64 Development' -or
        $Manifest.build_verified -isnot [bool] -or $Manifest.build_verified -ne $true -or $null -eq $Manifest.inputs -or
        $null -eq $Manifest.binary) {
        throw 'AIHEROES_PROVENANCE_INVALID unsupported or incomplete compiled snapshot.'
    }
    # The manifest never selects files to read: enumerate the fixed scope and fixed DLL.
    Assert-AIHeroesEntriesEqual -Expected @($Manifest.inputs) -Actual (Get-AIHeroesInputSnapshot -Root $Root)
    Assert-AIHeroesEntriesEqual -Expected @($Manifest.binary) -Actual @((Get-AIHeroesBinarySnapshot -Root $Root)) -Label 'binary'
}

function Enter-AIHeroesNativeWork {
    # Build and automation wrappers cooperate; legacy/nonparticipating tools do not.
    $Mutex = New-Object Threading.Mutex($false, 'Local\AIHEROES_NativeWork_AIHeroes')
    try {
        $Acquired = $false
        try { $Acquired = $Mutex.WaitOne(0) } catch [Threading.AbandonedMutexException] { $Acquired = $true }
        if (-not $Acquired) { throw 'Another coordinated AIHeroes build/test is running.' }
        return $Mutex
    } catch {
        $Mutex.Dispose()
        throw
    }
}

function Exit-AIHeroesNativeWork {
    param([Parameter(Mandatory = $true)][Threading.Mutex]$Mutex)
    $Mutex.ReleaseMutex()
    $Mutex.Dispose()
}
