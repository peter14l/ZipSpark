# ZipSpark Installation Guide

## Download

Download the latest release from [GitHub Releases](https://github.com/peter14l/ZipSpark/releases) or [GitHub Actions](https://github.com/peter14l/ZipSpark/actions).

---

## Installation Methods

### Method 1: MSIX Package (Recommended)

1. Download `ZipSpark.msix` from the latest release
2. Double-click the MSIX file
3. Click "Install" in the App Installer dialog
4. ZipSpark will be installed and file associations will be configured automatically

**Note:** You may need to install the certificate first (see below).

### Method 2: Manual Installation (Development)

1. Download `ZipSpark-x64-Release.zip` from GitHub Actions artifacts
2. Extract to a folder (e.g., `C:\Temp\ZipSpark`)
3. Open PowerShell as Administrator
4. Navigate to the extracted folder:
   ```powershell
   cd "C:\Temp\ZipSpark\ZipSpark-x64-Release"
   ```
5. Register the app:
   ```powershell
   Add-AppxPackage -Register AppxManifest.xml
   ```

---

## Certificate Installation (If Required)

If you see a security warning when installing the MSIX, you need to install the certificate:

### Option A: Install via MSIX (Easiest)
1. Right-click the MSIX file
2. Select "Properties"
3. Click "Digital Signatures" tab
4. Select the signature and click "Details"
5. Click "View Certificate"
6. Click "Install Certificate"
7. Choose "Local Machine"
8. Select "Place all certificates in the following store"
9. Click "Browse" and select "Trusted Root Certification Authorities"
10. Click "OK" and finish the wizard

### Option B: Install Certificate File
1. Download `ZipSpark.cer` (if provided)
2. Right-click and select "Install Certificate"
3. Choose "Local Machine"
4. Select "Place all certificates in the following store"
5. Browse to "Trusted Root Certification Authorities"
6. Click "OK" and finish

---

## Uninstallation

### Via Settings
1. Open Windows Settings
2. Go to Apps > Installed apps
3. Find "ZipSpark"
4. Click the three dots and select "Uninstall"

### Via PowerShell
```powershell
Get-AppxPackage *ZipSpark* | Remove-AppxPackage
```

---

## Usage

### Double-Click Extraction
1. Double-click any ZIP file in File Explorer
2. ZipSpark opens and extracts automatically
3. Progress shown in real-time
4. Success dialog shows extraction location

### Drag & Drop
1. Launch ZipSpark
2. Drag a ZIP file onto the window
3. Extraction starts automatically

### Manual Selection
1. Launch ZipSpark
2. Click "Extract Archive" button
3. Select ZIP file
4. Extraction starts

---

## Troubleshooting

### "This app can't run on your PC"
- Ensure you're running Windows 10 version 1809 or later
- Try installing the certificate (see above)

### "App didn't start"
- Check Windows Event Viewer for errors
- Ensure all dependencies are installed
- Try reinstalling

### File associations not working
- Reinstall the app
- Manually set ZipSpark as default for ZIP files:
  1. Right-click a ZIP file
  2. Select "Open with" > "Choose another app"
  3. Select "ZipSpark"
  4. Check "Always use this app"

---

## System Requirements

- **OS:** Windows 10 version 1809 or later / Windows 11
- **Architecture:** x64
- **Disk Space:** ~50 MB
- **.NET:** Windows App SDK (included)

---

## Support

For issues, please visit: https://github.com/peter14l/ZipSpark/issues
