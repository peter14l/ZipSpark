#include "pch.h"
#include "Settings.h"
#include "Logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <winrt/Windows.Storage.h>

namespace fs = std::filesystem;
using namespace winrt::Windows::Storage;

namespace ZipSpark {

std::wstring Settings::GetSettingsFilePath()
{
    // Get local app data folder
    auto localFolder = ApplicationData::Current().LocalFolder();
    auto path = localFolder.Path();
    
    // Create ZipSpark folder if it doesn't exist
    fs::path settingsDir = std::wstring(path) + L"\\ZipSpark";
    if (!fs::exists(settingsDir))
    {
        fs::create_directories(settingsDir);
    }
    
    return settingsDir.wstring() + L"\\settings.json";
}

void Settings::Load()
{
    try
    {
        std::wstring filePath = GetSettingsFilePath();
        
        if (!fs::exists(filePath))
        {
            LOG_INFO(L"Settings file not found, using defaults");
            return;
        }
        
        std::wifstream file(filePath);
        if (!file.is_open())
        {
            LOG_ERROR(L"Failed to open settings file");
            return;
        }
        
        // Simple JSON parsing (basic implementation)
        std::wstring line;
        while (std::getline(file, line))
        {
            // Parse key-value pairs
            size_t colonPos = line.find(L':');
            if (colonPos != std::wstring::npos)
            {
                std::wstring key = line.substr(0, colonPos);
                std::wstring value = line.substr(colonPos + 1);
                
                // Trim whitespace and quotes
                key.erase(0, key.find_first_not_of(L" \t\""));
                key.erase(key.find_last_not_of(L" \t\",") + 1);
                value.erase(0, value.find_first_not_of(L" \t\""));
                value.erase(value.find_last_not_of(L" \t\",") + 1);
                
                // Parse settings
                if (key == L"createSubfolder")
                    createSubfolder = (value == L"true");
                else if (key == L"preserveTimestamps")
                    preserveTimestamps = (value == L"true");
                else if (key == L"overwritePolicy")
                    overwritePolicy = static_cast<OverwritePolicy>(std::stoi(value));
                else if (key == L"closeAfterExtraction")
                    closeAfterExtraction = (value == L"true");
                else if (key == L"theme")
                    theme = static_cast<winrt::Microsoft::UI::Xaml::ElementTheme>(std::stoi(value));
                else if (key == L"enableLogging")
                    enableLogging = (value == L"true");
                else if (key == L"bufferSize")
                    bufferSize = std::stoul(value);
            }
        }
        
        file.close();
        LOG_INFO(L"Settings loaded successfully");
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(L"Failed to load settings: " + std::wstring(e.what(), e.what() + strlen(e.what())));
    }
}

void Settings::Save()
{
    try
    {
        std::wstring filePath = GetSettingsFilePath();
        std::wofstream file(filePath);
        
        if (!file.is_open())
        {
            LOG_ERROR(L"Failed to create settings file");
            return;
        }
        
        // Write JSON
        file << L"{\n";
        file << L"  \"createSubfolder\": " << (createSubfolder ? L"true" : L"false") << L",\n";
        file << L"  \"preserveTimestamps\": " << (preserveTimestamps ? L"true" : L"false") << L",\n";
        file << L"  \"overwritePolicy\": " << static_cast<int>(overwritePolicy) << L",\n";
        file << L"  \"closeAfterExtraction\": " << (closeAfterExtraction ? L"true" : L"false") << L",\n";
        file << L"  \"theme\": " << static_cast<int>(theme) << L",\n";
        file << L"  \"enableLogging\": " << (enableLogging ? L"true" : L"false") << L",\n";
        file << L"  \"bufferSize\": " << bufferSize << L"\n";
        file << L"}\n";
        
        file.close();
        LOG_INFO(L"Settings saved successfully");
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(L"Failed to save settings: " + std::wstring(e.what(), e.what() + strlen(e.what())));
    }
}

void Settings::ResetToDefaults()
{
    createSubfolder = true;
    preserveTimestamps = true;
    overwritePolicy = OverwritePolicy::Prompt;
    closeAfterExtraction = false;
    theme = winrt::Microsoft::UI::Xaml::ElementTheme::Default;
    enableLogging = true;
    bufferSize = 65536;
    
    Save();
    LOG_INFO(L"Settings reset to defaults");
}

} // namespace ZipSpark
