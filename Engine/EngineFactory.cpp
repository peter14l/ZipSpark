#include "pch.h"
#include "EngineFactory.h"
#include "SevenZipEngine.h"
#include "../Utils/Logger.h"
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace ZipSpark {

ArchiveFormat EngineFactory::DetectFormat(const std::wstring& archivePath)
{
    fs::path path(archivePath);
    std::wstring ext = path.extension().wstring();
    
    LOG_INFO(L"DetectFormat checking extension: '" + ext + L"' for path: '" + archivePath + L"'");
    
    // Use _wcsicmp for robust case-insensitive comparison on Windows
    if (_wcsicmp(ext.c_str(), L".zip") == 0) return ArchiveFormat::ZIP;
    if (_wcsicmp(ext.c_str(), L".7z") == 0) return ArchiveFormat::SevenZ;
    if (_wcsicmp(ext.c_str(), L".rar") == 0) return ArchiveFormat::RAR;
    if (_wcsicmp(ext.c_str(), L".tar") == 0) return ArchiveFormat::TAR;
    if (_wcsicmp(ext.c_str(), L".gz") == 0) return ArchiveFormat::GZ;
    if (_wcsicmp(ext.c_str(), L".tgz") == 0) return ArchiveFormat::TAR_GZ;
    if (_wcsicmp(ext.c_str(), L".txz") == 0) return ArchiveFormat::TAR_XZ;
    if (_wcsicmp(ext.c_str(), L".xz") == 0) return ArchiveFormat::XZ;
    
    LOG_WARNING(L"Detected Unknown format for extension: '" + ext + L"'");
    return ArchiveFormat::Unknown;
}

std::unique_ptr<IExtractionEngine> EngineFactory::CreateEngine(const std::wstring& archivePath)
{
    ArchiveFormat format = DetectFormat(archivePath);
    
    LOG_INFO(L"Detected format: " + std::to_wstring(static_cast<int>(format)) + L" for " + archivePath);
    
    switch (format)
    {
    case ArchiveFormat::ZIP:
    case ArchiveFormat::SevenZ:
    case ArchiveFormat::RAR:
    case ArchiveFormat::TAR:
    case ArchiveFormat::GZ:
    case ArchiveFormat::TAR_GZ:
    case ArchiveFormat::TAR_XZ:
    case ArchiveFormat::XZ:
        // Use 7-Zip process isolation for maximum stability and format support
        return std::make_unique<SevenZipEngine>();
        
    default:
        LOG_ERROR(L"Unknown archive format: " + archivePath);
        return nullptr;
    }
}

} // namespace ZipSpark
