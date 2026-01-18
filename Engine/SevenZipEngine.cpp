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
        LOG_ERROR(L"7z.exe not found!");
        if (callback) callback->OnError(ErrorCode::EngineInitializationFailed, L"7z.exe missing. Please reinstall.");
        return;
    }
    
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
        
        // Wait for it to finish
        // TODO: For better progress, we'd use pipes and PeekNamedPipe/ReadFile loop
        // For now, we wait and poll for cancellation
        
        while (WaitForSingleObject(pi.hProcess, 100) == WAIT_TIMEOUT)
        {
            if (m_cancelled)
            {
                LOG_INFO(L"Terminating 7z.exe...");
                TerminateProcess(pi.hProcess, 1);
                break;
            }
            // Indeterminate progress update?
        }
        
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        CloseHandle(pi.hProcess);
        m_hSubProcess = nullptr;
        
        if (m_cancelled)
        {
             if (callback) callback->OnError(ErrorCode::Unknown, L"Cancelled");
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
        if (callback) callback->OnError(ErrorCode::Unknown, L"Failed to launch extractor.");
    }
}

void SevenZipEngine::Cancel()
{
    m_cancelled = true;
    // Termination handling is in the loop in Extract
}

} // namespace ZipSpark
