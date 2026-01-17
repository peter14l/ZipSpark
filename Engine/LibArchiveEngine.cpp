#include "pch.h"
#include "LibArchiveEngine.h"
#include "../Utils/Logger.h"
#include "../Utils/ErrorHandler.h"
#include <filesystem>
#include <fstream>
#include <archive.h>
#include <archive_entry.h>

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
        
        // Convert archive path to UTF-8
        std::string archivePathUtf8;
        int size = WideCharToMultiByte(CP_UTF8, 0, info.archivePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (size > 0)
        {
            archivePathUtf8.resize(size);
            WideCharToMultiByte(CP_UTF8, 0, info.archivePath.c_str(), -1, &archivePathUtf8[0], size, nullptr, nullptr);
        }
        
        // Open archive using RAII for automatic cleanup
        struct archive* raw_a = archive_read_new();
        auto archive_deleter = [](struct archive* ptr) { 
            if (ptr) archive_read_free(ptr); 
        };
        std::unique_ptr<struct archive, decltype(archive_deleter)> a(raw_a, archive_deleter);

        archive_read_support_filter_all(a.get());
        archive_read_support_format_all(a.get());
        
        int r = archive_read_open_filename(a.get(), archivePathUtf8.c_str(), 10240);
        if (r != ARCHIVE_OK)
        {
            if (callback) callback->OnError(ErrorCode::ArchiveNotFound, L"Failed to open archive");
            return;
        }
        
        if (callback) callback->OnStart(info.fileCount);
        
        // Extract files
        struct archive_entry *entry;
        int fileIndex = 0;
        uint64_t totalExtracted = 0;
        
        while (archive_read_next_header(a.get(), &entry) == ARCHIVE_OK && !m_cancelled)
        {
            // Get entry path and convert to wide string
            const char* entryPath = archive_entry_pathname(entry);
            int wsize = MultiByteToWideChar(CP_UTF8, 0, entryPath, -1, nullptr, 0);
            std::wstring entryPathW;
            if (wsize > 0)
            {
                entryPathW.resize(wsize);
                MultiByteToWideChar(CP_UTF8, 0, entryPath, -1, &entryPathW[0], wsize);
            }
            
            // Build full destination path
            fs::path fullPath = destPath / entryPathW;
            
            // Report file progress
            if (callback)
            {
                callback->OnFileProgress(entryPathW, fileIndex, info.fileCount);
            }
            
            // Create directories
            if (archive_entry_filetype(entry) == AE_IFDIR)
            {
                fs::create_directories(fullPath);
            }
            else
            {
                // Create parent directory
                fs::create_directories(fullPath.parent_path());
                
                // Extract file
                std::ofstream outFile(fullPath, std::ios::binary);
                if (!outFile)
                {
                    LOG_ERROR(L"Failed to create file: " + fullPath.wstring());
                    continue;
                }
                
                const void* buff;
                size_t blockSize;
                int64_t offset;
                
                while (archive_read_data_block(a.get(), &buff, &blockSize, &offset) == ARCHIVE_OK)
                {
                    outFile.write(static_cast<const char*>(buff), blockSize);
                    totalExtracted += blockSize;
                    
                    // Update progress
                    int progress = info.totalSize > 0 ? 
                        static_cast<int>((totalExtracted * 100) / info.totalSize) : 0;
                    
                    if (callback)
                    {
                        callback->OnProgress(progress, totalExtracted, info.totalSize);
                    }
                }
                
                outFile.close();
            }
            
            fileIndex++;
        }
        
        // archive_read_free is called automatically by unique_ptr
        
        if (m_cancelled)
        {
            LOG_INFO(L"Extraction cancelled");
            return;
        }
        
        if (callback)
        {
            callback->OnProgress(100, info.totalSize, info.totalSize);
            callback->OnComplete(destination);
        }
        
        LOG_INFO(L"Extraction completed successfully");
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(L"Extraction failed: " + std::wstring(e.what(), e.what() + strlen(e.what())));
        if (callback) callback->OnError(ErrorCode::ExtractionFailed, L"Extraction error");
    }
    catch (...)
    {
        LOG_ERROR(L"Extraction failed: Unknown exception");
        if (callback) callback->OnError(ErrorCode::ExtractionFailed, L"Unknown extraction error");
    }
}

void LibArchiveEngine::Cancel()
{
    m_cancelled = true;
    LOG_INFO(L"Extraction cancelled by user");
}

} // namespace ZipSpark
