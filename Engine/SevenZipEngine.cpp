#include "pch.h"
#include "SevenZipEngine.h"
#include "../Utils/Logger.h"
#include <filesystem>
#include <windows.h>
#include <sstream>

namespace fs = std::filesystem;

namespace ZipSpark {

SevenZipEngine::SevenZipEngine()
{
}

SevenZipEngine::~SevenZipEngine()
{
    // Ensure handle is closed if still open (unlikely)
    if (m_hSubProcess)
    {
        CloseHandle(static_cast<HANDLE>(m_hSubProcess));
        m_hSubProcess = nullptr;
    }
}

// Timeout for extraction process (30 seconds)
constexpr DWORD EXTRACTION_TIMEOUT_MS = 30000;

bool SevenZipEngine::CanHandle(const std::wstring& archivePath)
{
    fs::path path(archivePath);
    std::wstring ext = path.extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    
    return IsSupportedFormat(ext);
}

bool SevenZipEngine::IsSupportedFormat(const std::wstring& extension)
{
    // 7-Zip handles almost everything
    return true; 
}

ArchiveInfo SevenZipEngine::GetArchiveInfo(const std::wstring& archivePath)
{
    ArchiveInfo info;
    info.archivePath = archivePath;
    
    // For now, basic info. Detailed info would require parsing "7z l" output.
    try
    {
        fs::path path(archivePath);
        if (fs::exists(path))
        {
            info.totalSize = fs::file_size(path);
        }
        info.fileCount = 0; // Placeholder
        
        // Guess format based on extension for metadata
        std::wstring ext = path.extension().wstring();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
        
        if (ext == L".7z") info.format = ArchiveFormat::SevenZ;
        else if (ext == L".zip") info.format = ArchiveFormat::ZIP;
        else if (ext == L".rar") info.format = ArchiveFormat::RAR;
        else info.format = ArchiveFormat::Unknown; // 7-Zip will figure it out anyway
    }
    catch (...) {}
    
    return info;
}

std::wstring SevenZipEngine::Get7zExePath()
{
    // Assuming 7z.exe is next to the executable or in External/7-Zip
    // In production (MSIX), it should be in the App layout.
    
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    fs::path appDir = fs::path(exePath).parent_path();
    
    // Check 1: Same directory
    fs::path p1 = appDir / L"7z.exe";
    if (fs::exists(p1)) return p1.wstring();
    
    // Check 2: External/7-Zip (Dev environment)
    fs::path p2 = appDir / L"External" / L"7-Zip" / L"7z.exe";
    if (fs::exists(p2)) return p2.wstring();
    
    return L"";
}

std::wstring SevenZipEngine::DetermineDestination(const ArchiveInfo& info, const ExtractionOptions& options)
{
    fs::path archivePath(info.archivePath);
    fs::path parentDir = archivePath.parent_path();
    
    if (!options.destinationPath.empty())
    {
        return options.destinationPath;
    }
    
    // Default: Extract to subfolder
    std::wstring folderName = archivePath.stem().wstring();
    return (parentDir / folderName).wstring();
}

void SevenZipEngine::Extract(const ArchiveInfo& info, const ExtractionOptions& options, IProgressCallback* callback)
{
    m_cancelled = false;
    
    std::wstring exe7z = Get7zExePath();
    if (exe7z.empty())
    {
        LOG_ERROR(L"7z.exe not found! Searched in application directory and External/7-Zip");
        if (callback) 
        {
            callback->OnError(ErrorCode::ExtractionFailed, 
                L"7z.exe is missing.\n\n"
                L"Please run Setup-7Zip.ps1 to download it, or place 7z.exe in the application directory.\n\n"
                L"The extraction engine requires 7-Zip to extract archives.");
        }
        return;
    }
    
    LOG_INFO(L"Found 7z.exe at: " + exe7z);
    
    std::wstring dest = DetermineDestination(info, options);
    
    // Command: 7z.exe x "Archive" -o"Dest" -y
    std::wstringstream cmd;
    cmd << L"\"" << exe7z << L"\" x \"" << info.archivePath << L"\" -o\"" << dest << L"\" -y";
    
    if (callback) callback->OnStart(0); // Indeterminate start
    
    LOG_INFO(L"Launching 7-Zip: " + cmd.str());
    
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    
    // We need a mutable string for CreateProcess
    std::wstring cmdStr = cmd.str();
    std::vector<wchar_t> cmdVec(cmdStr.begin(), cmdStr.end());
    cmdVec.push_back(0);
    
    if (CreateProcessW(NULL, cmdVec.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        m_hSubProcess = pi.hProcess; // Store for cancellation (naive, thread safety needed usually but simplified here)
        CloseHandle(pi.hThread);
        
        LOG_INFO(L"7z.exe process started successfully");
        
        // Wait for it to finish with timeout
        // Track total elapsed time to implement timeout
        auto startTime = std::chrono::steady_clock::now();
        DWORD totalElapsed = 0;
        
        while (true)
        {
            DWORD waitResult = WaitForSingleObject(pi.hProcess, 100);
            
            if (waitResult == WAIT_OBJECT_0)
            {
                // Process completed
                LOG_INFO(L"7z.exe process completed");
                break;
            }
            else if (waitResult == WAIT_TIMEOUT)
            {
                // Check for cancellation
                if (m_cancelled)
                {
                    LOG_INFO(L"Terminating 7z.exe due to user cancellation...");
                    TerminateProcess(pi.hProcess, 1);
                    break;
                }
                
                // Check for timeout (30 seconds)
                auto now = std::chrono::steady_clock::now();
                totalElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
                
                if (totalElapsed > EXTRACTION_TIMEOUT_MS)
                {
                    LOG_ERROR(L"7z.exe process timed out after " + std::to_wstring(totalElapsed) + L"ms");
                    TerminateProcess(pi.hProcess, 1);
                    CloseHandle(pi.hProcess);
                    m_hSubProcess = nullptr;
                    
                    if (callback)
                    {
                        callback->OnError(ErrorCode::ExtractionFailed, 
                            L"Extraction timed out after 30 seconds.\n\n"
                            L"The archive may be corrupted or too large.");
                    }
                    return;
                }
                
                // Continue waiting
                continue;
            }
            else
            {
                // Wait failed
                DWORD err = GetLastError();
                LOG_ERROR(L"WaitForSingleObject failed with error: " + std::to_wstring(err));
                break;
            }
        }
        
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        CloseHandle(pi.hProcess);
        m_hSubProcess = nullptr;
        
        if (m_cancelled)
        {
             LOG_INFO(L"Extraction was cancelled by user");
             if (callback) callback->OnError(ErrorCode::CancellationRequested, L"Cancelled");
             return;
        }
        
        if (exitCode == 0)
        {
             LOG_INFO(L"7-Zip finished successfully.");
             if (callback) callback->OnComplete(dest);
        }
        else
        {
             LOG_ERROR(L"7-Zip exited with code: " + std::to_wstring(exitCode));
             if (callback) callback->OnError(ErrorCode::ExtractionFailed, L"7-Zip Error Code: " + std::to_wstring(exitCode));
        }
    }
    else
    {
        DWORD err = GetLastError();
        LOG_ERROR(L"Failed to start 7z.exe. Error: " + std::to_wstring(err));
        if (callback) callback->OnError(ErrorCode::UnknownError, L"Failed to launch extractor. Error code: " + std::to_wstring(err));
    }
}

void SevenZipEngine::CreateArchive(const std::wstring& destinationPath, const std::vector<std::wstring>& sourceFiles, const std::wstring& format, IProgressCallback* callback)
{
    m_cancelled = false;
    
    std::wstring exe7z = Get7zExePath();
    if (exe7z.empty())
    {
        LOG_ERROR(L"7z.exe not found!");
        if (callback) 
        {
            callback->OnError(ErrorCode::ExtractionFailed, L"7z.exe is missing. Cannot create archive.");
        }
        return;
    }
    
    // Create temporary file list
    // 7-Zip supports list files with @listfile
    WCHAR tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    WCHAR listFileName[MAX_PATH];
    GetTempFileNameW(tempPath, L"7ZL", 0, listFileName);
    
    FILE* f = nullptr;
    _wfopen_s(&f, listFileName, L"w, ccs=UTF-8");
    if (!f)
    {
        LOG_ERROR(L"Failed to create list file");
        if (callback) callback->OnError(ErrorCode::UnknownError, L"Failed to create temporary list file");
        return;
    }
    
    for (const auto& file : sourceFiles)
    {
        fwprintf(f, L"%s\n", file.c_str());
    }
    fclose(f);
    
    // Command: 7z.exe a -t<format> "Destination" @listfile
    std::wstringstream cmd;
    cmd << L"\"" << exe7z << L"\" a -t" << (format.empty() ? L"zip" : format.substr(1)) << L" \"" << destinationPath << L"\" \"@" << listFileName << L"\"";
    
    if (callback) callback->OnStart(static_cast<int>(sourceFiles.size()));
    
    LOG_INFO(L"Launching 7-Zip Creation: " + cmd.str());
    
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    
    std::wstring cmdStr = cmd.str();
    std::vector<wchar_t> cmdVec(cmdStr.begin(), cmdStr.end());
    cmdVec.push_back(0);
    
    if (CreateProcessW(NULL, cmdVec.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        m_hSubProcess = pi.hProcess;
        CloseHandle(pi.hThread);
        
        while (true)
        {
            DWORD waitResult = WaitForSingleObject(pi.hProcess, 100);
            if (waitResult == WAIT_OBJECT_0) break;
            
            if (m_cancelled)
            {
                TerminateProcess(pi.hProcess, 1);
                break;
            }
        }
        
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        m_hSubProcess = nullptr;
        
        // Cleanup list file
        DeleteFileW(listFileName);
        
        if (m_cancelled)
        {
             if (callback) callback->OnError(ErrorCode::CancellationRequested, L"Cancelled");
             return;
        }
        
        if (exitCode == 0)
        {
             if (callback) callback->OnComplete(destinationPath);
        }
        else
        {
             if (callback) callback->OnError(ErrorCode::ExtractionFailed, L"7-Zip Error Code: " + std::to_wstring(exitCode));
        }
    }
    else
    {
        DeleteFileW(listFileName);
        DWORD err = GetLastError();
        if (callback) callback->OnError(ErrorCode::UnknownError, L"Failed to launch 7z.exe (Create)");
    }
}

void SevenZipEngine::Cancel()
{
    m_cancelled = true;
    // Termination handling is in the loop in Extract
}

} // namespace ZipSpark
