#pragma once
#include "pch.h"
#include "../Core/ExtractionOptions.h"
#include <string>
#include <winrt/Microsoft.UI.Xaml.h>

namespace ZipSpark {

/// <summary>
/// Application settings with persistence
/// </summary>
class Settings
{
public:
    /// <summary>
    /// Get singleton instance
    /// </summary>
    static Settings& GetInstance()
    {
        static Settings instance;
        return instance;
    }

    // Extraction settings
    bool createSubfolder = true;
    bool preserveTimestamps = true;

    // Behavior settings
    OverwritePolicy overwritePolicy = OverwritePolicy::Prompt;
    bool closeAfterExtraction = false;

    // Appearance settings
    winrt::Microsoft::UI::Xaml::ElementTheme theme = winrt::Microsoft::UI::Xaml::ElementTheme::Default;

    // Advanced settings
    bool enableLogging = true;
    uint32_t bufferSize = 65536; // 64 KB

    /// <summary>
    /// Load settings from file
    /// </summary>
    void Load();

    /// <summary>
    /// Save settings to file
    /// </summary>
    void Save();

    /// <summary>
    /// Reset all settings to defaults
    /// </summary>
    void ResetToDefaults();

private:
    Settings() = default;
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    std::wstring GetSettingsFilePath();
};

} // namespace ZipSpark
