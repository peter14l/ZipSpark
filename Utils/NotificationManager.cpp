#include "pch.h"
#include "NotificationManager.h"
#include "Logger.h"
#include <winrt/Windows.UI.Notifications.h>
#include <winrt/Windows.Data.Xml.Dom.h>
#include <ShObjIdl.h>
#include <wrl/client.h>

namespace ZipSpark {

void NotificationManager::ShowNotification(const std::wstring& title, const std::wstring& message)
{
    try
    {
        // Create toast XML
        std::wstring toastXml = L"<toast>"
            L"<visual>"
            L"<binding template='ToastGeneric'>"
            L"<text>" + title + L"</text>"
            L"<text>" + message + L"</text>"
            L"</binding>"
            L"</visual>"
            L"</toast>";

        winrt::Windows::Data::Xml::Dom::XmlDocument doc;
        doc.LoadXml(toastXml);

        winrt::Windows::UI::Notifications::ToastNotification toast(doc);
        winrt::Windows::UI::Notifications::ToastNotificationManager::CreateToastNotifier(L"ZipSpark").Show(toast);
        
        LOG_INFO(L"Notification shown: " + title);
    }
    catch (const std::exception& e)
    {
        winrt::hstring msg = winrt::to_hstring(e.what());
        LOG_ERROR(L"Failed to show notification: " + std::wstring(msg.c_str()));
    }
}

void NotificationManager::ShowExtractionComplete(const std::wstring& archiveName, const std::wstring& destination)
{
    std::wstring message = L"Extracted to: " + destination;
    ShowNotification(L"✓ Extraction Complete", message);
}

void NotificationManager::ShowError(const std::wstring& title, const std::wstring& message)
{
    try
    {
        std::wstring toastXml = L"<toast>"
            L"<visual>"
            L"<binding template='ToastGeneric'>"
            L"<text>" + title + L"</text>"
            L"<text>" + message + L"</text>"
            L"</binding>"
            L"</visual>"
            L"<audio src='ms-winsoundevent:Notification.Default'/>"
            L"</toast>";

        winrt::Windows::Data::Xml::Dom::XmlDocument doc;
        doc.LoadXml(toastXml);

        winrt::Windows::UI::Notifications::ToastNotification toast(doc);
        winrt::Windows::UI::Notifications::ToastNotificationManager::CreateToastNotifier(L"ZipSpark").Show(toast);
    }
    catch (...) {}
}

void NotificationManager::EnsureTaskbarInterface()
{
    if (!m_taskbar)
    {
        CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_taskbar));
    }
}

void NotificationManager::UpdateTaskbarProgress(HWND hwnd, int progress, int total)
{
    try
    {
        EnsureTaskbarInterface();
        if (m_taskbar)
        {
            m_taskbar->SetProgressValue(hwnd, progress, total);
        }
    }
    catch (...) {}
}

void NotificationManager::SetTaskbarState(HWND hwnd, int state)
{
    try
    {
        EnsureTaskbarInterface();
        if (m_taskbar)
        {
            m_taskbar->SetProgressState(hwnd, (TBPFLAG)state);
        }
    }
    catch (...) {}
}

} // namespace ZipSpark
