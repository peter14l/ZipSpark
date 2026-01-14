# ZipSpark Certificate Generator
# Run this script as Administrator

Write-Host "=== ZipSpark Certificate Generator ===" -ForegroundColor Cyan
Write-Host ""

# Check if running as administrator
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "ERROR: This script must be run as Administrator!" -ForegroundColor Red
    Write-Host "Right-click PowerShell and select 'Run as Administrator'" -ForegroundColor Yellow
    pause
    exit 1
}

# Step 1: Generate Certificate
Write-Host "Step 1: Generating self-signed certificate..." -ForegroundColor Green

$cert = New-SelfSignedCertificate -Type Custom `
    -Subject "CN=ZipSpark Development" `
    -KeyUsage DigitalSignature `
    -FriendlyName "ZipSpark Development Certificate" `
    -CertStoreLocation "Cert:\CurrentUser\My" `
    -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}") `
    -NotAfter (Get-Date).AddYears(2)

Write-Host "✓ Certificate created successfully!" -ForegroundColor Green
Write-Host "  Thumbprint: $($cert.Thumbprint)" -ForegroundColor Gray
Write-Host ""

# Step 2: Export Certificate with Private Key (for signing)
Write-Host "Step 2: Exporting certificate for signing..." -ForegroundColor Green

$password = Read-Host "Enter password for certificate" -AsSecureString
$pfxPath = Join-Path $PSScriptRoot "ZipSpark.pfx"
Export-PfxCertificate -Cert $cert -FilePath $pfxPath -Password $password | Out-Null

Write-Host "✓ PFX exported to: $pfxPath" -ForegroundColor Green
Write-Host ""

# Step 3: Export Public Certificate (for users)
Write-Host "Step 3: Exporting public certificate for users..." -ForegroundColor Green

$cerPath = Join-Path $PSScriptRoot "ZipSpark.cer"
Export-Certificate -Cert $cert -FilePath $cerPath | Out-Null

Write-Host "✓ CER exported to: $cerPath" -ForegroundColor Green
Write-Host ""

# Step 4: Convert PFX to Base64 for GitHub Secrets
Write-Host "Step 4: Converting PFX to Base64 for GitHub..." -ForegroundColor Green

$bytes = [System.IO.File]::ReadAllBytes($pfxPath)
$base64 = [System.Convert]::ToBase64String($bytes)
$base64 | Set-Clipboard

Write-Host "✓ Base64 string copied to clipboard!" -ForegroundColor Green
Write-Host ""

# Step 5: Instructions
Write-Host "=== Next Steps ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "1. Go to GitHub repository Settings > Secrets and variables > Actions" -ForegroundColor Yellow
Write-Host "2. Add new repository secret:" -ForegroundColor Yellow
Write-Host "   Name: CERTIFICATE_BASE64" -ForegroundColor White
Write-Host "   Value: Paste from clipboard (Ctrl+V)" -ForegroundColor White
Write-Host ""
Write-Host "3. Add another secret:" -ForegroundColor Yellow
Write-Host "   Name: CERTIFICATE_PASSWORD" -ForegroundColor White
Write-Host "   Value: The password you entered above" -ForegroundColor White
Write-Host ""
Write-Host "4. Commit and push ZipSpark.cer to your repository" -ForegroundColor Yellow
Write-Host "   (Users will need this to install the MSIX)" -ForegroundColor Gray
Write-Host ""
Write-Host "5. The GitHub Actions workflow will now sign the MSIX automatically!" -ForegroundColor Green
Write-Host ""
Write-Host "=== Certificate Files Created ===" -ForegroundColor Cyan
Write-Host "  ZipSpark.pfx - For signing (keep private!)" -ForegroundColor Gray
Write-Host "  ZipSpark.cer - For users to install (commit to repo)" -ForegroundColor Gray
Write-Host ""
Write-Host "Press any key to exit..." -ForegroundColor Gray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
