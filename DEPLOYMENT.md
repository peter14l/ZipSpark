# Running ZipSpark

## Prerequisites

**Install Windows App Runtime 1.8** (required for WinUI 3 apps):

1. Download the installer from Microsoft:
   - https://aka.ms/windowsappsdk/1.8/latest/windowsappruntimeinstall-x64.exe

2. Run the installer (double-click the downloaded `.exe`)

3. Verify installation:
   ```powershell
   Get-AppxPackage | Where-Object {$_.Name -like "*WindowsAppRuntime*"}
   ```

> **Note**: You only need to install this once per machine. After installation, all WinUI 3 apps will work.

---

## Important: WinUI 3 Deployment

WinUI 3 apps **cannot** be run by double-clicking the `.exe` file directly. They must be deployed as MSIX packages.

## Option 1: Deploy from Visual Studio (Recommended)

1. Open `ZipSpark-New.sln` in Visual Studio 2022
2. Set configuration to **Debug** or **Release**
3. Set platform to **x64**
4. Press **F5** (or click the green "Play" button)
   - This will build, deploy, and run the app
5. The app will appear in your Start Menu as "ZipSpark"

## Option 2: Deploy from GitHub Actions Artifacts (Easiest for Testing)

1. **Download the artifact** from GitHub Actions:
   - Go to https://github.com/peter14l/ZipSpark/actions
   - Click on the latest successful workflow run
   - Scroll down to "Artifacts"
   - Download `ZipSpark-x64-Release.zip`

2. **Extract the ZIP file** to a folder (e.g., `C:\Temp\ZipSpark`)

3. **Deploy using PowerShell**:
   ```powershell
   # Navigate to the extracted folder
   cd "C:\Temp\ZipSpark\ZipSpark-x64-Release"
   
   # Deploy the MSIX package
   Add-AppxPackage -Register AppxManifest.xml
   ```

4. **Launch the app**:
   - Search for "ZipSpark" in Windows Start Menu
   - Or run: `explorer.exe shell:AppsFolder\$(Get-AppxPackage | Where-Object {$_.Name -like "*ZipSpark*"} | Select-Object -ExpandProperty PackageFamilyName)!App`

> **Note**: The downloaded ZIP contains the MSIX package files, not a standalone EXE. You must use `Add-AppxPackage` to deploy it.

## Option 3: Manual MSIX Deployment (PowerShell)

```powershell
# Navigate to the build output
cd "x64\Debug\ZipSpark-New"

# Install the MSIX package
Add-AppxPackage -Register AppxManifest.xml
```

## Troubleshooting

### "Nothing happens when I click the EXE"

This is expected! WinUI 3 apps require the Windows App SDK runtime and must be packaged as MSIX.

**Solution**: Use Option 1 (Visual Studio F5) or Option 3 (PowerShell deployment)

### "App crashes immediately"

1. Check Windows Event Viewer for crash logs
2. Run from Visual Studio with debugger attached (F5)
3. Check that all dependencies are installed

### "Can't find the app after deployment"

- Search for "ZipSpark" in Windows Start Menu
- Or run: `Get-AppxPackage | Where-Object {$_.Name -like "*ZipSpark*"}`

## Uninstalling

```powershell
Get-AppxPackage | Where-Object {$_.Name -like "*ZipSpark*"} | Remove-AppxPackage
```

## Development Workflow

For active development:
1. Keep Visual Studio open
2. Make code changes
3. Press **F5** to rebuild and run
4. App will auto-deploy and launch

The deployed app will update each time you run from Visual Studio.
