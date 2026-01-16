#include "pch.h"
#include "RecentFiles.h"
#include "Logger.h"
#include <fstream>
#include <algorithm>
#include <winrt/Windows.Storage.h>

namespace fs = std::filesystem;
using namespace winrt::Windows::Storage;

namespace ZipSpark {

std::wstring RecentFiles::GetStorageFilePath()
{
    auto localFolder = ApplicationData::Current().LocalFolder();
    auto path = localFolder.Path();
    
    fs::path settingsDir = std::wstring(path) + L"\\ZipSpark";
    if (!fs::exists(settingsDir))
    {
        fs::create_directories(settingsDir);
    }
    
    return settingsDir.wstring() + L"\\recent.txt";
}

void RecentFiles::AddFile(const std::wstring& filePath)
{
    // Remove if already exists
    auto it = std::find(m_recentFiles.begin(), m_recentFiles.end(), filePath);
    if (it != m_recentFiles.end())
    {
        m_recentFiles.erase(it);
    }

    // Add to front
    m_recentFiles.push_front(filePath);

    // Limit size
    while (m_recentFiles.size() > MAX_RECENT_FILES)
    {
        m_recentFiles.pop_back();
    }

    Save();
}

std::vector<std::wstring> RecentFiles::GetRecentFiles() const
{
    return std::vector<std::wstring>(m_recentFiles.begin(), m_recentFiles.end());
}

void RecentFiles::Clear()
{
    m_recentFiles.clear();
    Save();
}

void RecentFiles::Load()
{
    try
    {
        std::wstring filePath = GetStorageFilePath();
        if (!fs::exists(filePath))
            return;

        std::wifstream file(filePath);
        if (!file.is_open())
            return;

        m_recentFiles.clear();
        std::wstring line;
        while (std::getline(file, line) && m_recentFiles.size() < MAX_RECENT_FILES)
        {
            if (!line.empty() && fs::exists(line))
            {
                m_recentFiles.push_back(line);
            }
        }

        file.close();
        LOG_INFO(L"Loaded " + std::to_wstring(m_recentFiles.size()) + L" recent files");
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(L"Failed to load recent files: " + std::wstring(e.what(), e.what() + strlen(e.what())));
    }
}

void RecentFiles::Save()
{
    try
    {
        std::wstring filePath = GetStorageFilePath();
        std::wofstream file(filePath);
        
        if (!file.is_open())
            return;

        for (const auto& recentFile : m_recentFiles)
        {
            file << recentFile << L"\n";
        }

        file.close();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(L"Failed to save recent files: " + std::wstring(e.what(), e.what() + strlen(e.what())));
    }
}

void RecentFiles::PinFile(const std::wstring& filePath)
{
    if (std::find(m_pinnedFiles.begin(), m_pinnedFiles.end(), filePath) == m_pinnedFiles.end())
    {
        m_pinnedFiles.push_back(filePath);
        Save();
    }
}

void RecentFiles::UnpinFile(const std::wstring& filePath)
{
    auto it = std::find(m_pinnedFiles.begin(), m_pinnedFiles.end(), filePath);
    if (it != m_pinnedFiles.end())
    {
        m_pinnedFiles.erase(it);
        Save();
    }
}

std::vector<std::wstring> RecentFiles::GetPinnedFiles() const
{
    return m_pinnedFiles;
}

} // namespace ZipSpark
