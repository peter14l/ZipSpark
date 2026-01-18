# Setup-7Zip.ps1
# Downloads 7-Zip binaries for ZipSpark

$ErrorActionPreference = "Stop"

$version = "2409"
$url = "https://www.7-zip.org/a/7z$version-extra.7z"
$outputDir = Join-Path $PSScriptRoot "External\7-Zip"
$zipFile = Join-Path $PSScriptRoot "7z.7z"

Write-Host "Creating output directory: $outputDir"
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

Write-Host "Downloading 7-Zip from $url..."
Invoke-WebRequest -Uri $url -OutFile $zipFile

Write-Host "Extracting 7-Zip..."
# We can't use Expand-Archive for .7z files natively in older Powershell, 
# but bootstrapping is tricky if we don't have 7-zip to extract 7-zip.
# However, 7z-extra is a .7z file.
# We might need to download the .exe installer (self-extracting) or use a zip distribution if available.
# 7-Zip usually provides a .exe installation.
# Let's try downloading the 64-bit MSI or EXE and extracting it, or finding a .zip source.
# Actually, if we use the .exe installer, we can silently install or extract.
# But for portability, let's look for a .zip source or use a pretender.
# Wait, Windows 11 supports .7z extraction natively now! The user is on Windows 11 (implied by WinUI 3).
# Let's try `tar` (built-in to Windows 10/11) to extract if possible, or assume user can handle it.
# Actually, to be safe, let's download the console version specific ZIP if valid, but 7-zip site only provides .7z for extras.
# Update: Recent Windows 11 DOES support 7z.
# Fallback: Download 7zr.exe (lzma SDK) which is .exe? No.
# Let's use the MSI, it's a standard format. We can use msiexec /a to extract.

$msiUrl = "https://www.7-zip.org/a/7z$version-x64.msi"
$msiFile = Join-Path $PSScriptRoot "7z.msi"

Write-Host "Downloading 7-Zip MSI from $msiUrl..."
Invoke-WebRequest -Uri $msiUrl -OutFile $msiFile

Write-Host "Extracting MSI..."
# Administrative install to extract files
$extractLog = Join-Path $PSScriptRoot "msi_log.txt"
Start-Process "msiexec.exe" -ArgumentList "/a `"$msiFile`" /qn TARGETDIR=`"$outputDir`"" -Wait

# The files will be in $outputDir\Files\7-Zip
$installedDir = Join-Path $outputDir "Files\7-Zip"

if (Test-Path $installedDir) {
    Write-Host "7-Zip binaries located at: $installedDir"
    
    # Copy vital files to the root of External/7-Zip for easy access
    Copy-Item (Join-Path $installedDir "7z.exe") $outputDir
    Copy-Item (Join-Path $installedDir "7z.dll") $outputDir
    Copy-Item (Join-Path $installedDir "7z.sfx") $outputDir
    
    Write-Host "Setup complete. 7z.exe is ready in $outputDir"
    
    # Cleanup
    Remove-Item $msiFile
    Remove-Item (Join-Path $outputDir "Files") -Recurse -Force
} else {
    Write-Error "Failed to extract 7-Zip MSI. Please verify the download."
}
