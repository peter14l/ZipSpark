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
        std::string what = e.what();
        std::wstring wwhat(what.begin(), what.end());
        LOG_ERROR(L"Failed to get archive info: " + wwhat);
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

// Helper to sanitize filenames (remove invalid chars < > : " / \ | ? *)
std::wstring SanitizePathComponent(const std::wstring& component)
{
    std::wstring sanitized = component;
    const std::wstring invalidChars = L"<>:\"/\\|?*";
    
    for (auto& ch : sanitized)
    {
        if (invalidChars.find(ch) != std::wstring::npos || ch < 32)
        {
            ch = L'_';
        }
    }
    return sanitized;
}

// Helper to log hard crashes (avoiding C2712 in Extract)
void LogHardCrash(DWORD code, IProgressCallback* callback)
{
    std::wstring msg = L"CRITICAL ERROR: Access Violation (0x" + std::to_wstring(code) + L") caught in extraction engine.";
    LOG_ERROR(msg);
    if (callback)
    {
        callback->OnError(ErrorCode::ExtractionFailed, L"System Memory Error (Access Violation). The archive may be corrupt or incompatible.");
    }
}

void LibArchiveEngine::Extract(const ArchiveInfo& info, const ExtractionOptions& options, IProgressCallback* callback)
{
    m_cancelled = false;
    
    // SEH WRAPPER: Catch "Hard Crashes" (Access Violations) preventing app termination
    // NOTE: This function must NOT contain any objects with destructors (C2712)
    __try
    {
        ExtractInternal(info, options, callback);
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        LogHardCrash(GetExceptionCode(), callback);
    }
}

void LibArchiveEngine::ExtractInternal(const ArchiveInfo& info, const ExtractionOptions& options, IProgressCallback* callback)
{
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
        
        // Open archive using RAII for automatic cleanup
        struct archive* raw_a = archive_read_new();
        auto archive_deleter = [](struct archive* ptr) { 
            if (ptr) archive_read_free(ptr); 
        };
        std::unique_ptr<struct archive, decltype(archive_deleter)> a(raw_a, archive_deleter);

        archive_read_support_filter_all(a.get());
        archive_read_support_format_all(a.get());
        
        // Use native wide-char API for Windows
        int r = archive_read_open_filename_w(a.get(), info.archivePath.c_str(), 10240);
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
            std::wstring entryPathW;
            
            if (entryPath)
            {
                int wsize = MultiByteToWideChar(CP_UTF8, 0, entryPath, -1, nullptr, 0);
                if (wsize > 0)
                {
                    entryPathW.resize(wsize);
                    MultiByteToWideChar(CP_UTF8, 0, entryPath, -1, &entryPathW[0], wsize);
                    entryPathW.resize(wsize - 1); // Exclude null terminator
                }
            }
            else
            {
                LOG_ERROR(L"Skipping entry with null path");
                continue;
            }
            
            LOG_INFO(L"Processing entry: " + entryPathW); // Trace logging
            
            // SECURITY CHECK: Prevent Zip Slip (Directory Traversal)
            // Sanitize path components to prevent invalid chars
            fs::path entryPathObj(entryPathW);
            fs::path sanitizedEntryPath;
            
            for (const auto& component : entryPathObj)
            {
                 // Don't sanitize separators (handled by fs::path iteration)
                 // But do sanitize component names
                 sanitizedEntryPath /= SanitizePathComponent(component.wstring());
            }

            // 1. Force relative path
            if (sanitizedEntryPath.is_absolute())
            {
                sanitizedEntryPath = sanitizedEntryPath.relative_path();
            }
            
            // 2. Build full potential path
            fs::path fullPath = destPath / sanitizedEntryPath;
            
            // 3. Verify it is still inside destPath
            try 
            {
                fs::path canonicalDest = fs::weakly_canonical(destPath);
                fs::path canonicalFull = fs::weakly_canonical(fullPath);
                
                std::wstring sDest = canonicalDest.wstring();
                std::wstring sFull = canonicalFull.wstring();
                
                // Ensure sFull starts with sDest
                if (sFull.length() < sDest.length() || 
                    sFull.compare(0, sDest.length(), sDest) != 0)
                {
                    LOG_ERROR(L"Security Warning: Skipped file with invalid path (outside destination): " + entryPathW);
                    continue;
                }
            }
            catch (...)
            {
                LOG_ERROR(L"Error validating path: " + entryPathW);
                continue;
            }
            
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
    }
    catch (const std::exception& e)
    {
        std::string what = e.what();
        std::wstring wwhat(what.begin(), what.end());
        LOG_ERROR(L"Exception in ExtractInternal: " + wwhat);
        if (callback) callback->OnError(ErrorCode::ExtractionFailed, wwhat);
    }
    catch (...)
    {
        LOG_ERROR(L"Unknown exception in ExtractInternal");
        if (callback) callback->OnError(ErrorCode::ExtractionFailed, L"Unknown Error");
    }
}
void LibArchiveEngine::Cancel()
{
    m_cancelled = true;
    LOG_INFO(L"Extraction cancelled by user");
}

} // namespace ZipSpark
