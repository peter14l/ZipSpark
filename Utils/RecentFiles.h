#pragma once
#include "pch.h"
#include <string>
#include <vector>
#include <deque>
#include <filesystem>

namespace ZipSpark {

/// <summary>
/// Manages recent files list
/// </summary>
class RecentFiles
{
public:
    static RecentFiles& GetInstance()
    {
        static RecentFiles instance;
        return instance;
    }

    /// <summary>
    /// Add a file to recent list
    /// </summary>
    void AddFile(const std::wstring& filePath);

    /// <summary>
    /// Get recent files list
    /// </summary>
    std::vector<std::wstring> GetRecentFiles() const;

    /// <summary>
    /// Clear recent files
    /// </summary>
    void Clear();

    /// <summary>
    /// Load from storage
    /// </summary>
    void Load();

    /// <summary>
    /// Save to storage
    /// </summary>
    void Save();

    /// <summary>
    /// Pin/favorite a file
    /// </summary>
    void PinFile(const std::wstring& filePath);

    /// <summary>
    /// Unpin a file
    /// </summary>
    void UnpinFile(const std::wstring& filePath);

    /// <summary>
    /// Get pinned files
    /// </summary>
    std::vector<std::wstring> GetPinnedFiles() const;

private:
    RecentFiles() = default;
    RecentFiles(const RecentFiles&) = delete;
    RecentFiles& operator=(const RecentFiles&) = delete;

    static constexpr size_t MAX_RECENT_FILES = 10;
    std::deque<std::wstring> m_recentFiles;
    std::vector<std::wstring> m_pinnedFiles;

    std::wstring GetStorageFilePath();
};

} // namespace ZipSpark
