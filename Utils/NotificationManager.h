#pragma once
#include "pch.h"
#include <string>
#include <map>
#include <winrt/Windows.UI.Notifications.h>
#include <winrt/Windows.Data.Xml.Dom.h>
#include <ShObjIdl.h>
#include <wrl/client.h>

namespace ZipSpark {

class NotificationManager
{
public:
    static NotificationManager& GetInstance()
    {
        static NotificationManager instance;
        return instance;
    }

    /// <summary>
    /// Show a toast notification
    /// </summary>
    void ShowNotification(const std::wstring& title, const std::wstring& message);

    /// <summary>
    /// Show extraction complete notification
    /// </summary>
    void ShowExtractionComplete(const std::wstring& archiveName, const std::wstring& destination);

    /// <summary>
    /// Show error notification
    /// </summary>
    void ShowError(const std::wstring& title, const std::wstring& message);

    /// <summary>
    /// Update taskbar progress
    /// </summary>
    void UpdateTaskbarProgress(HWND hwnd, int progress, int total);

    /// <summary>
    /// Set taskbar state (normal, paused, error)
    /// </summary>
    void SetTaskbarState(HWND hwnd, int state);
    
private:
    NotificationManager() = default;
    NotificationManager(const NotificationManager&) = delete;
    NotificationManager& operator=(const NotificationManager&) = delete;

    Microsoft::WRL::ComPtr<ITaskbarList3> m_taskbar;
    void EnsureTaskbarInterface();
};

} // namespace ZipSpark
