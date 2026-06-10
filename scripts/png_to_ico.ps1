param(
    [string]$PngPath,
    [string]$IcoPath
)

Add-Type -AssemblyName System.Drawing

function New-SquareBitmap {
    param(
        [System.Drawing.Image]$Source,
        [int]$Size
    )

    $bitmap = New-Object System.Drawing.Bitmap $Size, $Size
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.Clear([System.Drawing.Color]::FromArgb(0, 0, 0, 0))
    $graphics.DrawImage($Source, 0, 0, $Size, $Size)
    $graphics.Dispose()
    return $bitmap
}

$source = [System.Drawing.Image]::FromFile($PngPath)
$sizes = @(16, 32, 48, 64, 128, 256)
$bitmaps = New-Object System.Collections.Generic.List[System.Drawing.Bitmap]

foreach ($size in $sizes) {
    $bitmaps.Add((New-SquareBitmap -Source $source -Size $size))
}

$source.Dispose()

$iconHandle = $bitmaps[$bitmaps.Count - 1].GetHicon()
$icon = [System.Drawing.Icon]::FromHandle($iconHandle)
$stream = [System.IO.File]::Open($IcoPath, [System.IO.FileMode]::Create)
$icon.Save($stream)
$stream.Close()
$icon.Dispose()

foreach ($bitmap in $bitmaps) {
    $bitmap.Dispose()
}

Write-Host "Wrote $IcoPath"
