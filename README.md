# ZipSpark

A modern Windows archive extraction utility with macOS-style double-click extraction, built with WinUI 3 and C++20.

## Features

- **Instant Extraction**: Double-click archives to extract with minimal friction
- **Multi-Format Support**: ZIP, 7z, RAR, TAR, GZ, TAR.GZ, TAR.XZ, XZ
- **Context-Aware Destination**: Smart folder selection based on archive contents
- **Fluent Design**: Modern UI with Mica backdrop and Phanvi's color scheme
- **Fast & Secure**: Built on libarchive/7-Zip SDK with robust error handling
- **File Associations**: Registers as default handler for supported archive formats

## Tech Stack

- **Language**: C++20
- **UI Framework**: WinUI 3 (Windows App SDK)
- **Extraction Engine**: libarchive (primary), 7-Zip SDK (fallback)
- **Build System**: MSBuild, Visual Studio 2022
- **CI/CD**: GitHub Actions

## Building

### Prerequisites

- Windows 10 version 1809 or later
- Visual Studio 2022 with:
  - Desktop development with C++
  - Windows App SDK
  - C++/WinRT tools

### Build Steps

1. Clone the repository:
   ```bash
   git clone https://github.com/peter14l/ZipSpark.git
   cd ZipSpark-New
   ```

2. Open `ZipSpark-New.sln` in Visual Studio 2022

3. Restore NuGet packages (automatic on first build)

4. Build the solution:
   - Press `Ctrl+Shift+B`
   - Or: `Build` → `Build Solution`

### GitHub Actions Build

The project includes a GitHub Actions workflow that automatically builds on push:
- Builds both Debug and Release configurations
- Uploads Release artifacts for download

## Project Structure

```
ZipSpark-New/
├── Core/              # Business logic and data models
├── Engine/            # Extraction engine interfaces
├── UI/                # XAML windows and controls
├── Utils/             # Helper utilities (logging, error handling)
├── Resources/
│   └── Themes/        # Dark and Light theme resources
└── Assets/            # App icons and images
```

## Current Status

**Phase 1 Complete**: Project setup and architecture
- ✅ Core data models (ArchiveInfo, ExtractionOptions, ExtractionProgress)
- ✅ Extraction engine interface
- ✅ Logging and error handling utilities
- ✅ Theme resources with Phanvi's color scheme
- ✅ File type associations configured

**Next**: Phase 2 - Extraction engine implementation

## Color Scheme (Phanvi's 2565EB)

- **Primary**: Blue 600 (#2565EB)
- **Accent**: Green 500 (#22C55E)
- **Error**: Red 500 (#EF4444)
- **Warning**: Amber 500 (#F59E0B)

## License

[Add your license here]

## Support the Project
    
If you find ZipSpark useful, please consider supporting its development to keep the project alive!

- ☕ **Ko-fi**: [Buy me a coffee](https://ko-fi.com/zipspark)
- 🇮🇳 **UPI (India)**: `zipspark@upi` (Replace with actual UPI ID if available, using placeholder for now as per request)

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
