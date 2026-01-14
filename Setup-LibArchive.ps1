# Download and Setup libarchive Pre-Built Binaries
# Run this script to download and configure libarchive

Write-Host "=== libarchive Setup Script ===" -ForegroundColor Cyan
Write-Host ""

# Create libs directory
$libsDir = Join-Path $PSScriptRoot "libs"
$libarchiveDir = Join-Path $libsDir "libarchive"

if (!(Test-Path $libsDir)) {
    New-Item -ItemType Directory -Path $libsDir | Out-Null
}

# Download libarchive
$downloadUrl = "https://github.com/libarchive/libarchive/releases/download/v3.7.2/libarchive-v3.7.2-win64.zip"
$zipPath = Join-Path $libsDir "libarchive.zip"

Write-Host "Downloading libarchive 3.7.2..." -ForegroundColor Green
try {
    Invoke-WebRequest -Uri $downloadUrl -OutFile $zipPath -UseBasicParsing
    Write-Host "✓ Download complete" -ForegroundColor Green
} catch {
    Write-Host "✗ Download failed: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "Manual download:" -ForegroundColor Yellow
    Write-Host "1. Download from: $downloadUrl" -ForegroundColor White
    Write-Host "2. Extract to: $libarchiveDir" -ForegroundColor White
    exit 1
}

# Extract
Write-Host "Extracting..." -ForegroundColor Green
try {
    Expand-Archive -Path $zipPath -DestinationPath $libsDir -Force
    
    # Rename extracted folder to libarchive
    $extractedFolder = Get-ChildItem -Path $libsDir -Directory | Where-Object { $_.Name -like "libarchive*" } | Select-Object -First 1
    if ($extractedFolder -and $extractedFolder.FullName -ne $libarchiveDir) {
        if (Test-Path $libarchiveDir) {
            Remove-Item $libarchiveDir -Recurse -Force
        }
        Rename-Item -Path $extractedFolder.FullName -NewName "libarchive"
    }
    
    Write-Host "✓ Extraction complete" -ForegroundColor Green
} catch {
    Write-Host "✗ Extraction failed: $_" -ForegroundColor Red
    exit 1
}

# Cleanup
Remove-Item $zipPath -Force

Write-Host ""
Write-Host "=== Setup Complete ===" -ForegroundColor Cyan
Write-Host "libarchive installed to: $libarchiveDir" -ForegroundColor Gray
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Build the project (libarchive will be linked automatically)" -ForegroundColor White
Write-Host "2. Test multi-format extraction" -ForegroundColor White
Write-Host ""
