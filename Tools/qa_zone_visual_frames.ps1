param(
    [string]$DesktopFrames = 'Saved\QA\ZoneExpedition_20261029T155215303Z',
    [string]$CompactFrames = 'Docs\VisualRuns\ZoneTouchUI_20261029\compact_960x540'
)
# Visual-frame regression receipts: load PNG pixels, not OCR, text recognition,
# live user SaveGames, global window input or external tools. Pixel evidence
# does not itself prove accessibility contrast or mobile device behavior.
$ErrorActionPreference='Stop'
$Root=Split-Path -Parent $PSScriptRoot
Add-Type -AssemblyName System.Drawing
function InProject([string]$Path) {
    if(-not [IO.Path]::IsPathRooted($Path)){$Path=Join-Path $Root $Path}
    $P=(Resolve-Path -LiteralPath $Path -ErrorAction Stop).Path
    if(-not $P.StartsWith($Root+'\',[StringComparison]::OrdinalIgnoreCase)){
        throw 'VISUAL_QA_BLOCKED: frame lies outside the project'
    }
    return $P
}
$Sizes=@(
    [pscustomobject]@{name='desktop';dir=(InProject $DesktopFrames);width=1280;height=720},
    [pscustomobject]@{name='compact';dir=(InProject $CompactFrames);width=960;height=540}
)
$Names=@('initial','dossier','forest','ore','day2')
$All=@()
foreach($Group in $Sizes){
    $Screens=@{}
    $UniqueSha=@{}
    foreach($Name in $Names){
        $P=InProject (Join-Path $Group.dir ($Name+'.png'))
        $F=Get-Item -LiteralPath $P
        if($F.Length -lt 75000 -or
           ($F.Attributes -band [IO.FileAttributes]::ReparsePoint)){
            throw ('VISUAL_QA_FAIL: incomplete/linked frame '+$P)
        }
        $Hash=(Get-FileHash -LiteralPath $P -Algorithm SHA256).Hash.ToLowerInvariant()
        if($UniqueSha.ContainsKey($Hash)){throw ('VISUAL_QA_FAIL: duplicated frame '+$P)}
        $UniqueSha[$Hash]=1
        $Image=[System.Drawing.Bitmap]::FromFile($P)
        try {
            if($Image.Width -ne $Group.width -or $Image.Height -ne $Group.height) {
                throw ('VISUAL_QA_FAIL: unexpected frame resolution '+$P)
            }
            # The playable map is between the HUD zones. Test only that it
            # has observable content; do not infer readable labels via pixels.
            $X0=[int]($Image.Width*0.29)
            $X1=[int]($Image.Width*0.72)
            $Y0=[int]($Image.Height*0.25)
            $Y1=[int]($Image.Height*0.65)
            $Seen=0;$NonDark=0;$Colors=@{};$RgbSum=[long]0
            for($Y=$Y0;$Y -lt $Y1;$Y+=9){
                for($X=$X0;$X -lt $X1;$X+=9){
                    $C=$Image.GetPixel($X,$Y)
                    $Seen++
                    $RgbSum+=($C.R+$C.G+$C.B)
                    if(($C.R+$C.G+$C.B) -ge 120){$NonDark++}
                    $Bucket=('{0}-{1}-{2}' -f [int]($C.R/16),[int]($C.G/16),[int]($C.B/16))
                    $Colors[$Bucket]=1
                }
            }
            $Coverage=[Math]::Round($NonDark/[double]$Seen,4)
            if($Name -eq 'initial' -and ($Coverage -lt 0.25 -or $Colors.Count -lt 5)){
                throw ('VISUAL_QA_FAIL: playable map too empty '+$Group.name)
            }
            $Screens[$Name]=$P
            $All+= [pscustomobject][ordered]@{
                group=$Group.name
                state=$Name
                path=$P.Substring($Root.Length+1).Replace('\','/')
                bytes=[long]$F.Length
                captured_file_mtime_utc=$F.LastWriteTimeUtc.ToString('o')
                sha256=$Hash
                width=$Image.Width
                height=$Image.Height
                map_sample_points=$Seen
                non_dark_fraction=$Coverage
                distinct_sampled_rgb16_buckets=$Colors.Count
            }
        } finally {$Image.Dispose()}
    }
    # Pixel-difference is a basic anti-empty screenshot check, not gameplay
    # correctness. Compare the same co-ordinates in every output state.
    $Baseline=[System.Drawing.Bitmap]::FromFile($Screens['initial'])
    try{
        foreach($Name in @('dossier','forest','ore','day2')){
            $Other=[System.Drawing.Bitmap]::FromFile($Screens[$Name])
            try{
                $Changed=0;$Total=0
                for($Y=60;$Y -lt ($Baseline.Height-38);$Y+=8){
                    for($X=14;$X -lt ($Baseline.Width-14);$X+=8){
                        $A=$Baseline.GetPixel($X,$Y)
                        $B=$Other.GetPixel($X,$Y)
                        $Diff=[Math]::Abs([int]$A.R-[int]$B.R)+
                            [Math]::Abs([int]$A.G-[int]$B.G)+
                            [Math]::Abs([int]$A.B-[int]$B.B)
                        if($Diff -gt 24){$Changed++}
                        $Total++
                    }
                }
                $Ratio=[Math]::Round($Changed/[double]$Total,4)
                $Entry=$All|Where-Object{$_.group -eq $Group.name -and $_.state -eq $Name}
                $Entry|Add-Member -NotePropertyName 'different_from_initial_fraction' -NotePropertyValue $Ratio
                if($Ratio -lt 0.005){
                    throw ('VISUAL_QA_FAIL: capture appears unchanged '+$Group.name+' '+$Name)
                }
            }finally{$Other.Dispose()}
        }
    }finally{$Baseline.Dispose()}
}
if($All.Count -ne 10){throw 'VISUAL_QA_FAIL: missing visual states'}
$OutDir=Join-Path $Root ('Docs\VisualRuns\VisualFraming_'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
New-Item -ItemType Directory -Path $OutDir -ErrorAction Stop|Out-Null
$Receipt=[pscustomobject][ordered]@{
    schema=1
    status='FRAME_INTEGRITY_PASS'
    visual_claim='desktop and compact screenshot files contain non-empty, visually distinct map and UI states; not an accessibility, Android or full art acceptance claim'
    captured_utc=[DateTime]::UtcNow.ToString('o')
    project_head_at_audit_time=(& git -C $Root rev-parse HEAD).Trim()
    screenshot_source_revision='UNKNOWN: screenshot files have no same-build compiled source provenance embedded'
    cross_resolution_parity='NOT_VERIFIED: desktop and compact screenshots were captured at different times; no same-build guarantee'
    screenshot_count=$All.Count
    groups=$Sizes.Count
    frames=$All
    manual_review='review map framing, text size and 4.5:1 contrast separately; pixel buckets cannot establish text readability'
    standards=@('Microsoft XAG 102 contrast','Epic UE5.8 UMG Safe Zones')
}
$Dest=Join-Path $OutDir 'visual_frames.json'
[IO.File]::WriteAllText($Dest,($Receipt|ConvertTo-Json -Depth 8),
    (New-Object Text.UTF8Encoding($false)))
$Read=Get-Content $Dest -Raw -Encoding UTF8|ConvertFrom-Json
if($Read.status -cne $Receipt.status -or @($Read.frames).Count -ne 10){
    throw 'VISUAL_QA_FAIL: read-back not complete'
}
$F=Get-Item $Dest
Write-Output ('AIH_VISUAL_FRAME_PASS screenshots='+$All.Count+' groups=2')
foreach($G in @('desktop','compact')){
    $Selection=@($All|Where-Object{$_.group -eq $G})
    Write-Output ($G+' initial_map_non_dark='+($Selection|Where-Object{$_.state -eq 'initial'}|Select-Object -ExpandProperty non_dark_fraction)+
        ' initial_rgb_buckets='+($Selection|Where-Object{$_.state -eq 'initial'}|Select-Object -ExpandProperty distinct_sampled_rgb16_buckets)+
        ' changed_min='+($Selection|Where-Object{$_.state -ne 'initial'}|Measure-Object different_from_initial_fraction -Minimum).Minimum)
}
Write-Output ('VISUAL_RECEIPT='+$Dest+' bytes='+$F.Length+
    ' sha256='+(Get-FileHash $Dest -Algorithm SHA256).Hash.ToLowerInvariant())
