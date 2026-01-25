#include "pch.h"
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
    
    // Convert to lowercase for comparison
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    
    if (ext == L".zip") return ArchiveFormat::ZIP;
    if (ext == L".7z") return ArchiveFormat::SevenZ;
    if (ext == L".rar") return ArchiveFormat::RAR;
    if (ext == L".tar") return ArchiveFormat::TAR;
    if (ext == L".gz") return ArchiveFormat::GZ;
    if (ext == L".tgz") return ArchiveFormat::TAR_GZ;
    if (ext == L".txz") return ArchiveFormat::TAR_XZ;
    if (ext == L".xz") return ArchiveFormat::XZ;
    
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
