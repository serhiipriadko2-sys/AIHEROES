param(
    [Parameter(Mandatory=$true)][string]$GameCompiledReceipt,
    [ValidateRange(1,100000)][int]$MinTests=197,
    [switch]$PreflightOnly
)
# Fails closed before UAT unless the COMPLETE Editor->tests->Game source
# evidence still matches the live project and Game exe. Creates no player save.
$ErrorActionPreference='Stop'
$Root=Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'aiheroes_build_evidence.ps1')
$Mutex=Enter-AIHeroesNativeWork
try {
    function OwnedPath([string]$Path) {
        if ([string]::IsNullOrWhiteSpace($Path)) {throw 'PACKAGE_BLOCKED: empty receipt path'}
        if (-not [IO.Path]::IsPathRooted($Path)) {$Path=Join-Path $Root $Path}
        return (Resolve-Path -LiteralPath $Path -ErrorAction Stop).Path
    }
    function ContentFiles {
        $C=Join-Path $Root 'Content'
        $All=@(Get-ChildItem -LiteralPath $C -Force -Recurse -ErrorAction Stop)
        if (@($All|Where-Object{$_.Attributes -band [IO.FileAttributes]::ReparsePoint}).Count) {
            throw 'PACKAGE_BLOCKED: Content has linked resources'
        }
        $All|Where-Object{-not $_.PSIsContainer}|ForEach-Object {
            [pscustomobject][ordered]@{
                path=$_.FullName.Substring($Root.Length+1).Replace('\','/')
                bytes=[long]$_.Length
                sha256=(Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
            }
        }|Sort-Object path
    }
    function SaveInventory {
        $Folder=Join-Path $Root 'Saved\SaveGames'
        if (-not(Test-Path -LiteralPath $Folder)) {return}
        Get-ChildItem -LiteralPath $Folder -File -Force|Sort-Object Name|ForEach-Object {
            [pscustomobject][ordered]@{
                name=$_.Name
                bytes=[long]$_.Length
                modified_ticks=$_.LastWriteTimeUtc.Ticks
                sha256=(Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
            }
        }
    }
    $Busy=@(Get-CimInstance Win32_Process -Filter "Name='dotnet.exe' OR Name='UnrealEditor-Cmd.exe' OR Name='UnrealEditor.exe'"|
        Where-Object{$_.Name -in @('UnrealEditor-Cmd.exe','UnrealEditor.exe') -or $_.CommandLine -match 'UnrealBuildTool'})
    if ($Busy.Count -or (Get-Process -Name LiveCodingConsole -ErrorAction SilentlyContinue)) {
        throw 'PACKAGE_BLOCKED: native compiler, Editor, or Live Coding is active'
    }
    $ReceiptPath=OwnedPath $GameCompiledReceipt
    $Game=Get-Content $ReceiptPath -Raw -Encoding UTF8|ConvertFrom-Json
    if ($Game.schema -ne 1 -or $Game.status -cne 'compiled' -or
        $Game.build_verified -ne $true -or
        $Game.target -cne 'AIHeroes Win64 Development' -or
        $null -eq $Game.inputs -or $null -eq $Game.game_binary) {
        throw 'PACKAGE_BLOCKED: invalid verified Game compilation receipt'
    }
    $EditorPath=OwnedPath $Game.editor_manifest
    $Editor=Get-Content $EditorPath -Raw -Encoding UTF8|ConvertFrom-Json
    Assert-AIHeroesCompiledSnapshot -Manifest $Editor -Root $Root
    Assert-AIHeroesEntriesEqual -Expected @($Editor.inputs) -Actual @($Game.inputs)
    Assert-AIHeroesEntriesEqual -Expected @($Editor.binary) -Actual @($Game.editor_binary) -Label 'Editor DLL'
    $ReportPath=OwnedPath $Game.automation_report
    $Report=Get-Content $ReportPath -Raw -Encoding UTF8|ConvertFrom-Json
    $Tests=@($Report.tests)
    if ($Tests.Count -lt $MinTests -or $Report.succeeded -ne $Tests.Count -or
        $Report.failed -ne 0 -or $Report.notRun -ne 0 -or
        $Report.succeededWithWarnings -ne 0 -or
        @($Tests|Where-Object{$_.state -ne 'Success'}).Count) {
        throw 'PACKAGE_BLOCKED: full Automation is not green'
    }
    $ReportSha=(Get-FileHash $ReportPath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($ReportSha -cne $Game.automation_report_sha256 -or
        $Game.automation_tests -ne $Tests.Count) {
        throw 'PACKAGE_BLOCKED: test report was changed'
    }
    $TestProvenancePath=Join-Path (Split-Path $ReportPath) 'provenance.json'
    $Provenance=Get-Content $TestProvenancePath -Raw -Encoding UTF8|ConvertFrom-Json
    if ($Provenance.status -cne 'passed' -or $Provenance.build_verified -ne $true -or
        $Provenance.filter -cne 'AIHeroes' -or
        $Provenance.report_sha256 -cne $ReportSha -or
        (Get-FileHash $TestProvenancePath -Algorithm SHA256).Hash.ToLowerInvariant() -cne
            $Game.automation_provenance_sha256) {
        throw 'PACKAGE_BLOCKED: test provenance invalid'
    }
    Assert-AIHeroesEntriesEqual -Expected @($Editor.inputs) -Actual @($Provenance.inputs)
    Assert-AIHeroesEntriesEqual -Expected @($Editor.binary) -Actual @($Provenance.binary) -Label 'test Editor DLL'
    $Exe=Join-Path $Root 'Binaries\Win64\AIHeroes.exe'
    $ExeItem=Get-Item $Exe -ErrorAction Stop
    $ExeSha=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($ExeSha -cne $Game.game_binary.sha256 -or
        $ExeItem.Length -ne $Game.game_binary.bytes) {
        throw 'PACKAGE_BLOCKED: Game exe does not match compiled receipt'
    }
    # This helper returns a single, non-enumerated typed array. Do not wrap
    # the invocation in @(): that would create a nested one-item array.
    $BeforeInputs=Get-AIHeroesInputSnapshot -Root $Root
    $BeforeContent=@(ContentFiles)
    $BeforeSaves=@(SaveInventory)
    Assert-AIHeroesEntriesEqual -Expected @($Editor.inputs) -Actual $BeforeInputs
    if ((Get-PSDrive -Name C).Free -lt 10GB) {
        throw 'PACKAGE_BLOCKED: fewer than ten GiB available'
    }

    if($PreflightOnly) {
        Write-Output ('AIHEROES_PACKAGE_PREFLIGHT_PASS inputs='+$BeforeInputs.Count+' tests='+$Tests.Count+' save_count='+$BeforeSaves.Count+' exe='+$ExeSha)
        return
    }
    $UAT='C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat'
    if (-not (Test-Path $UAT)) {throw 'PACKAGE_BLOCKED: RunUAT unavailable'}
    $Id=[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
    $Folder=Join-Path $Root ('Saved\QA\Win64Package_'+$Id)
    if (Test-Path $Folder) {throw 'PACKAGE_BLOCKED: output already exists'}
    $Stage=Join-Path $Folder 'stage'
    $Archive=Join-Path $Folder 'archive'
    New-Item -ItemType Directory -Path $Folder -ErrorAction Stop|Out-Null
    Write-Output ('UAT_PACKAGE_START source_count='+$BeforeInputs.Count+' tests='+$Tests.Count+' game_sha256='+$ExeSha)
    & $UAT BuildCookRun "-project=$Root\AIHeroes.uproject" -noP4 -platform=Win64 -clientconfig=Development -nocompileeditor -nocompileuat -skipbuild -cook -stage -package -pak -archive "-stagingdirectory=$Stage" "-archivedirectory=$Archive" -unattended -utf8output 2>&1 | Tee-Object -FilePath (Join-Path $Folder 'uat.log')
    if ($LASTEXITCODE -ne 0) {
        throw ('PACKAGE_FAILED: UAT returned '+$LASTEXITCODE+', log='+$Folder+'\uat.log')
    }
    Assert-AIHeroesCompiledSnapshot -Manifest $Editor -Root $Root
    Assert-AIHeroesEntriesEqual -Expected $BeforeContent -Actual @(ContentFiles) -Label 'Content'
    if (($BeforeSaves|ConvertTo-Json -Depth 4 -Compress) -cne
        (@(SaveInventory)|ConvertTo-Json -Depth 4 -Compress)) {
        throw 'PACKAGE_FAILED: player SaveGame inventory changed'
    }
    if ((Get-FileHash $Exe -Algorithm SHA256).Hash.ToLowerInvariant() -cne $ExeSha) {
        throw 'PACKAGE_FAILED: Game binary changed during packaging'
    }
    $Items=@(Get-ChildItem -LiteralPath $Archive -Recurse -Force -File -ErrorAction Stop)
    $ExeFiles=@($Items|Where-Object{$_.Name -eq 'AIHeroes.exe'})
    $Containers=@($Items|Where-Object{$_.Extension -in @('.pak','.utoc','.ucas')})
    if ($Items.Count -eq 0 -or $ExeFiles.Count -eq 0 -or
        $Containers.Count -eq 0 -or
        @($Items|Where-Object{$_.Attributes -band [IO.FileAttributes]::ReparsePoint}).Count) {
        throw 'PACKAGE_FAILED: output has no exe or cooked assets, or contains links'
    }
    $Files=@($Items|ForEach-Object{
        [pscustomobject][ordered]@{
            path=$_.FullName.Substring($Archive.Length+1).Replace('\','/')
            bytes=[long]$_.Length
            sha256=(Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    }|Sort-Object path)
    if (@($Files|Group-Object {$_.path.ToLowerInvariant()}|Where-Object{$_.Count -gt 1}).Count) {
        throw 'PACKAGE_FAILED: duplicate paths'
    }
    $Output=[pscustomobject][ordered]@{
        schema=1
        status='PACKAGED_UNTESTED'
        lifecycle='Windows Development game cooked/staged/packaged; GUI acceptance pending'
        created_utc=[DateTime]::UtcNow.ToString('o')
        game_receipt=$GameCompiledReceipt
        game_receipt_sha256=(Get-FileHash $ReceiptPath -Algorithm SHA256).Hash.ToLowerInvariant()
        source_count=$BeforeInputs.Count
        automation_tests=$Tests.Count
        player_save_files=$BeforeSaves.Count
        player_save_drift=0
        game_sha256=$ExeSha
        archive=$Archive
        archive_files=$Files.Count
        archive_bytes=[long](($Items|Measure-Object Length -Sum).Sum)
        asset_containers=$Containers.Count
        outputs=$Files
        uat_sha256=(Get-FileHash (Join-Path $Folder 'uat.log') -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    $OutputPath=Join-Path $Folder 'package_receipt.json'
    [IO.File]::WriteAllText($OutputPath,($Output|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
    $Read=Get-Content $OutputPath -Raw -Encoding UTF8|ConvertFrom-Json
    if ($Read.archive_files -ne $Files.Count -or $Read.game_sha256 -cne $ExeSha) {
        throw 'PACKAGE_FAILED: readback mismatch'
    }
    Write-Output ('AIHEROES_PACKAGE_PASS files='+$Files.Count+' assets='+$Containers.Count+' receipt='+$OutputPath)
} finally {
    Exit-AIHeroesNativeWork -Mutex $Mutex
}
