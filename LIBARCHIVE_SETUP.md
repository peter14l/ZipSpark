# Quick Guide: Installing libarchive via vcpkg

## Option 1: vcpkg (Recommended for this project)

Since you already have vcpkg set up, install libarchive:

```powershell
cd c:\Users\peter\source\repos\ZipSpark-New
vcpkg install libarchive:x64-windows
```

This will take some time but handles all dependencies automatically.

## Option 2: Pre-built Binaries (Faster but manual)

1. Download from: https://github.com/libarchive/libarchive/releases/download/v3.7.2/libarchive-v3.7.2-win64.zip
2. Extract to `c:\Users\peter\source\repos\ZipSpark-New\libs\libarchive\`
3. Manually configure project

## For Now: Let's Use a Simpler Approach

Instead of waiting for vcpkg, I'll implement the extraction logic first and you can install libarchive later. The code will be ready to use once the library is available.

**Proceed with implementation?**
