# Creating and Using a Self-Signed Certificate for ZipSpark

## Option 1: For Development/Testing (Self-Signed Certificate)

### Step 1: Generate Certificate (Run on your local machine)

```powershell
# Generate a self-signed certificate
$cert = New-SelfSignedCertificate -Type Custom `
    -Subject "CN=ZipSpark Development" `
    -KeyUsage DigitalSignature `
    -FriendlyName "ZipSpark Development Certificate" `
    -CertStoreLocation "Cert:\CurrentUser\My" `
    -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}") `
    -NotAfter (Get-Date).AddYears(2)

# Export certificate with private key (for signing)
$password = ConvertTo-SecureString -String "YourPasswordHere" -Force -AsPlainText
Export-PfxCertificate -Cert $cert -FilePath "ZipSpark.pfx" -Password $password

# Export public certificate (for users to install)
Export-Certificate -Cert $cert -FilePath "ZipSpark.cer"

Write-Host "Certificate created successfully!"
Write-Host "Thumbprint: $($cert.Thumbprint)"
```

### Step 2: Update Package.appxmanifest

The `Publisher` in Package.appxmanifest must match the certificate subject:

```xml
<Identity
  Name="8b8d2ade-6cf4-482b-8497-bc4ad96ff63d"
  Publisher="CN=ZipSpark Development"
  Version="1.0.0.0" />
```

### Step 3: Add Certificate to GitHub Secrets

1. Go to GitHub repository Settings > Secrets and variables > Actions
2. Add new repository secret:
   - Name: `CERTIFICATE_BASE64`
   - Value: Base64-encoded PFX file
   
```powershell
# Convert PFX to Base64
$bytes = [System.IO.File]::ReadAllBytes("ZipSpark.pfx")
$base64 = [System.Convert]::ToBase64String($bytes)
$base64 | Set-Clipboard
# Now paste into GitHub secret
```

3. Add another secret:
   - Name: `CERTIFICATE_PASSWORD`
   - Value: The password you used when creating the PFX

### Step 4: Update GitHub Actions Workflow

The workflow will decode the certificate and sign the MSIX:

```yaml
- name: Decode certificate
  run: |
    $base64 = "${{ secrets.CERTIFICATE_BASE64 }}"
    $bytes = [System.Convert]::FromBase64String($base64)
    [System.IO.File]::WriteAllBytes("ZipSpark.pfx", $bytes)

- name: Build and sign MSIX
  run: |
    msbuild ZipSpark-New.vcxproj /p:Configuration=Release /p:Platform=x64 /p:AppxBundle=Never /p:UapAppxPackageBuildMode=SideloadOnly /p:AppxPackageDir=AppPackages\ /p:GenerateAppxPackageOnBuild=true /p:PackageCertificateKeyFile=ZipSpark.pfx /p:PackageCertificatePassword=${{ secrets.CERTIFICATE_PASSWORD }} /m /v:minimal
```

---

## Option 2: For Production (Trusted Certificate)

For production distribution, you need a certificate from a trusted Certificate Authority:

1. **Purchase a Code Signing Certificate** from:
   - DigiCert
   - Sectigo
   - GlobalSign
   
2. **Cost**: ~$100-400/year

3. **Benefits**:
   - No certificate installation required for users
   - Trusted by Windows automatically
   - Can publish to Microsoft Store

---

## Installation Instructions for Users

### If Using Self-Signed Certificate:

**Before installing the MSIX, users must install the certificate:**

1. Download `ZipSpark.cer` from the release
2. Right-click `ZipSpark.cer` and select "Install Certificate"
3. Choose "Local Machine" (requires admin)
4. Select "Place all certificates in the following store"
5. Click "Browse" and select "Trusted Root Certification Authorities"
6. Click "OK" through all dialogs
7. Now double-click `ZipSpark.msix` to install

**Alternative (PowerShell as Admin):**
```powershell
Import-Certificate -FilePath "ZipSpark.cer" -CertStoreLocation Cert:\LocalMachine\Root
```

---

## Current Status

- ✅ MSIX package is being generated
- ❌ MSIX is not signed (causes certificate error)
- ⏳ Need to create certificate and add to GitHub secrets

---

## Quick Fix for Local Testing

If you just want to test locally without certificate:

```powershell
# Allow unsigned packages (not recommended for production)
Add-AppxPackage -Register AppxManifest.xml -AllowUnsigned
```

Or use the existing method:
```powershell
Add-AppxPackage -Register AppxManifest.xml
```
