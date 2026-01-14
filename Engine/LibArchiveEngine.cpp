#include "pch.h"
#include "LibArchiveEngine.h"
#include "../Utils/Logger.h"
#include "../Utils/ErrorHandler.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace ZipSpark {

LibArchiveEngine::LibArchiveEngine()
{
    LOG_INFO(L"LibArchiveEngine initialized");
}

LibArchiveEngine::~LibArchiveEngine()
{
}

bool LibArchiveEngine::CanHandle(const std::wstring& archivePath)
{
    fs::path path(archivePath);
    std::wstring ext = path.extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    
    return IsSupportedFormat(ext);
}

bool LibArchiveEngine::IsSupportedFormat(const std::wstring& extension)
{
    // Supported formats
    return extension == L".7z" ||
           extension == L".rar" ||
           extension == L".tar" ||
           extension == L".gz" ||
           extension == L".xz" ||
           extension == L".tgz" ||
           extension == L".txz";
}

ArchiveInfo LibArchiveEngine::GetArchiveInfo(const std::wstring& archivePath)
{
    ArchiveInfo info;
    info.archivePath = archivePath;
    
    try
    {
        fs::path path(archivePath);
        std::wstring ext = path.extension().wstring();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
        
        // Detect format
        if (ext == L".7z")
            info.format = ArchiveFormat::SevenZ;
        else if (ext == L".rar")
            info.format = ArchiveFormat::RAR;
        else if (ext == L".tar")
            info.format = ArchiveFormat::TAR;
        else if (ext == L".gz")
            info.format = ArchiveFormat::GZ;
        else if (ext == L".xz")
            info.format = ArchiveFormat::XZ;
        else if (ext == L".tgz" || path.filename().wstring().find(L".tar.gz") != std::wstring::npos)
            info.format = ArchiveFormat::TAR_GZ;
        else if (ext == L".txz" || path.filename().wstring().find(L".tar.xz") != std::wstring::npos)
            info.format = ArchiveFormat::TAR_XZ;
        
        if (fs::exists(path))
        {
            info.totalSize = fs::file_size(path);
        }
        
        // For now, assume multiple roots (will be refined with actual libarchive integration)
        info.hasSingleRoot = false;
        info.fileCount = 0; // Unknown until we scan with libarchive
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(L"Failed to get archive info: " + std::wstring(e.what(), e.what() + strlen(e.what())));
    }
    
    return info;
}

std::wstring LibArchiveEngine::DetermineDestination(const ArchiveInfo& info, const ExtractionOptions& options)
{
    fs::path archivePath(info.archivePath);
    fs::path parentDir = archivePath.parent_path();
    
    // If user specified a destination, use it
    if (!options.destinationPath.empty())
    {
        return options.destinationPath;
    }
    
    // Context-aware destination logic
    if (info.hasSingleRoot || !options.createSubfolder)
    {
        // Extract to same folder as archive
        return parentDir.wstring();
    }
    else
    {
        // Create subfolder named after archive (without extension)
        std::wstring folderName = archivePath.stem().wstring();
        
        // Handle .tar.gz and .tar.xz (double extension)
        if (folderName.length() >= 4 && 
            folderName.substr(folderName.length() - 4) == L".tar")
        {
            folderName = folderName.substr(0, folderName.length() - 4);
        }
        
        fs::path destPath = parentDir / folderName;
        return destPath.wstring();
    }
}

void LibArchiveEngine::Extract(const ArchiveInfo& info, const ExtractionOptions& options, IProgressCallback* callback)
{
    m_cancelled = false;
    
    try
    {
        LOG_INFO(L"Starting extraction with libarchive: " + info.archivePath);
        
        // Determine destination
        std::wstring destination = DetermineDestination(info, options);
        
        // Create destination directory if needed
        fs::path destPath(destination);
        if (!fs::exists(destPath))
        {
            fs::create_directories(destPath);
        }
        
        LOG_INFO(L"Extracting to: " + destination);
        
        if (callback) callback->OnStart(info.fileCount);
        
        // TODO: Implement actual libarchive extraction
        // For now, show error that libarchive is not yet integrated
        if (callback)
        {
            callback->OnError(ErrorCode::UnsupportedFormat, 
                L"libarchive integration in progress. Currently only ZIP files are supported.");
        }
        
        LOG_WARNING(L"libarchive not yet integrated - extraction failed");
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(L"Extraction failed: " + std::wstring(e.what(), e.what() + strlen(e.what())));
        if (callback) callback->OnError(ErrorCode::ExtractionFailed, L"Extraction error");
    }
}

void LibArchiveEngine::Cancel()
{
    m_cancelled = true;
    LOG_INFO(L"Extraction cancelled by user");
}

} // namespace ZipSpark
