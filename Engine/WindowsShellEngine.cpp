#include "pch.h"
#include "WindowsShellEngine.h"
#include "../Utils/Logger.h"
#include "../Utils/ErrorHandler.h"
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

namespace ZipSpark {

WindowsShellEngine::WindowsShellEngine()
{
    InitializeCOM();
}

WindowsShellEngine::~WindowsShellEngine()
{
    CleanupCOM();
}

bool WindowsShellEngine::InitializeCOM()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    return SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
}

void WindowsShellEngine::CleanupCOM()
{
    CoUninitialize();
}

bool WindowsShellEngine::CanHandle(const std::wstring& archivePath)
{
    // Windows Shell can only handle ZIP files
    fs::path path(archivePath);
    std::wstring ext = path.extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    return ext == L".zip";
}

ArchiveInfo WindowsShellEngine::GetArchiveInfo(const std::wstring& archivePath)
{
    ArchiveInfo info;
    info.archivePath = archivePath;
    info.format = ArchiveFormat::ZIP;
    
    try
    {
        fs::path path(archivePath);
        if (fs::exists(path))
        {
            info.totalSize = fs::file_size(path);
        }
        
        // Analyze structure to determine if single root
        int rootItemCount = 0;
        if (AnalyzeArchiveStructure(archivePath, rootItemCount))
        {
            info.hasSingleRoot = (rootItemCount == 1);
            info.fileCount = rootItemCount; // Approximate for now
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(L"Failed to get archive info: " + std::wstring(e.what(), e.what() + strlen(e.what())));
    }
    
    return info;
}

bool WindowsShellEngine::AnalyzeArchiveStructure(const std::wstring& archivePath, int& rootItemCount)
{
    rootItemCount = 0;
    
    try
    {
        // Create Shell.Application COM object
        IShellDispatch* pShell = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_Shell, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_IShellDispatch, (void**)&pShell);
        if (FAILED(hr) || !pShell)
            return false;

        // Get the ZIP folder
        _bstr_t zipPath(archivePath.c_str());
        Folder* pZipFolder = nullptr;
        hr = pShell->NameSpace(zipPath, &pZipFolder);
        
        if (SUCCEEDED(hr) && pZipFolder)
        {
            FolderItems* pItems = nullptr;
            hr = pZipFolder->Items(&pItems);
            
            if (SUCCEEDED(hr) && pItems)
            {
                long count = 0;
                pItems->get_Count(&count);
                rootItemCount = static_cast<int>(count);
                pItems->Release();
            }
            
            pZipFolder->Release();
        }
        
        pShell->Release();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::wstring WindowsShellEngine::DetermineDestination(const ArchiveInfo& info, const ExtractionOptions& options)
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
        fs::path destPath = parentDir / folderName;
        return destPath.wstring();
    }
}

void WindowsShellEngine::Extract(const ArchiveInfo& info, const ExtractionOptions& options, IProgressCallback* callback)
{
    m_cancelled = false;
    
    try
    {
        LOG_INFO(L"Starting extraction: " + info.archivePath);
        
        // Determine destination
        std::wstring destination = DetermineDestination(info, options);
        
        // Create destination directory if needed
        fs::path destPath(destination);
        if (!fs::exists(destPath))
        {
            fs::create_directories(destPath);
        }
        
        LOG_INFO(L"Extracting to: " + destination);
        
        // Create Shell.Application COM object
        IShellDispatch* pShell = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_Shell, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_IShellDispatch, (void**)&pShell);
        if (FAILED(hr) || !pShell)
        {
            if (callback) callback->OnError(ErrorCode::ExtractionFailed, L"Failed to create Shell object");
            return;
        }

        // Get source and destination folders
        _bstr_t zipPath(info.archivePath.c_str());
        _bstr_t destPathBstr(destination.c_str());
        
        Folder* pZipFolder = nullptr;
        Folder* pDestFolder = nullptr;
        
        hr = pShell->NameSpace(zipPath, &pZipFolder);
        if (FAILED(hr) || !pZipFolder)
        {
            pShell->Release();
            if (callback) callback->OnError(ErrorCode::ArchiveNotFound, L"Failed to open archive");
            return;
        }
        
        hr = pShell->NameSpace(destPathBstr, &pDestFolder);
        if (FAILED(hr) || !pDestFolder)
        {
            pZipFolder->Release();
            pShell->Release();
            if (callback) callback->OnError(ErrorCode::DestinationNotFound, L"Failed to access destination");
            return;
        }

        // Get items from ZIP
        FolderItems* pItems = nullptr;
        hr = pZipFolder->Items(&pItems);
        if (FAILED(hr) || !pItems)
        {
            pDestFolder->Release();
            pZipFolder->Release();
            pShell->Release();
            if (callback) callback->OnError(ErrorCode::ExtractionFailed, L"Failed to read archive contents");
            return;
        }

        // Start extraction
        if (callback) callback->OnStart(info.fileCount);
        
        // Options: 4 = Do not display a progress dialog box
        //         16 = Respond with "Yes to All" for any dialog box that is displayed
        VARIANT vOptions;
        vOptions.vt = VT_I4;
        vOptions.lVal = 4 | 16;
        
        VARIANT vItems;
        vItems.vt = VT_DISPATCH;
        vItems.pdispVal = pItems;
        
        // Perform the copy (extraction)
        hr = pDestFolder->CopyHere(vItems, vOptions);
        
        // Monitor progress by checking destination folder
        std::thread progressThread([this, callback, destination, info]() {
            int lastProgress = 0;
            while (!m_cancelled)
            {
                try
                {
                    // Calculate progress based on destination folder size
                    uintmax_t extractedSize = 0;
                    for (const auto& entry : fs::recursive_directory_iterator(destination))
                    {
                        if (fs::is_regular_file(entry))
                        {
                            extractedSize += fs::file_size(entry);
                        }
                    }
                    
                    int progress = info.totalSize > 0 ? 
                        static_cast<int>((extractedSize * 100) / info.totalSize) : 0;
                    
                    if (progress > lastProgress && callback)
                    {
                        callback->OnProgress(progress, extractedSize, info.totalSize);
                        lastProgress = progress;
                    }
                    
                    if (progress >= 100)
                        break;
                }
                catch (...) {}
                
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
        
        // Wait for extraction to complete (Shell API is synchronous)
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Cleanup
        progressThread.join();
        pItems->Release();
        pDestFolder->Release();
        pZipFolder->Release();
        pShell->Release();
        
        if (!m_cancelled && callback)
        {
            callback->OnComplete(destination);
            LOG_INFO(L"Extraction completed successfully");
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(L"Extraction failed: " + std::wstring(e.what(), e.what() + strlen(e.what())));
        if (callback) callback->OnError(ErrorCode::ExtractionFailed, L"Extraction error");
    }
}

void WindowsShellEngine::Cancel()
{
    m_cancelled = true;
    LOG_INFO(L"Extraction cancelled by user");
}

} // namespace ZipSpark
