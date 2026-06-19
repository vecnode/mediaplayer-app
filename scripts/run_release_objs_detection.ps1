# Run object/text detection on UFO_FILES\Release_1_PNG -> bin\data\OBJS_TEXT.md
# Fast defaults: CUDA + FP16 YOLO, no KMeans colors, max-side 1600 for OCR/YOLO
# Requires: python + easyocr + ultralytics + torch (CUDA recommended)
param(
    [int]$Limit = 0,
    [switch]$NoResume,
    [switch]$NoGpu,
    [int]$MaxSide = 1600
)

$ErrorActionPreference = "Stop"

$UfoRoot = "C:\Users\luisarandas\Desktop\UFO_FILES"
$ImageDir = Join-Path $UfoRoot "Release_1_PNG"
$DetectScript = Join-Path $UfoRoot "detect_release_objects.py"
$ProjectRoot = Split-Path $PSScriptRoot -Parent
$Output = Join-Path $ProjectRoot "bin\data\OBJS_TEXT.md"

if (-not (Test-Path $DetectScript)) {
    Write-Error "Missing $DetectScript"
}
if (-not (Test-Path $ImageDir)) {
    Write-Error "Missing $ImageDir"
}

$args = @(
    $DetectScript,
    "--image-dir", $ImageDir,
    "--output", $Output
)
if ($Limit -gt 0) { $args += @("--limit", $Limit) }
if ($NoResume) { $args += "--no-resume" }
if ($NoGpu) { $args += "--no-gpu" }
if ($MaxSide -ge 0) { $args += @("--max-side", $MaxSide) }

Write-Host "Detecting regions for PNGs in:"
Write-Host "  $ImageDir"
Write-Host "Output:"
Write-Host "  $Output"
Write-Host ""

& python @args
