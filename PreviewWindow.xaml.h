#pragma once

#include "PreviewWindow.g.h"
#include "Core/ArchiveInfo.h"
#include <vector>
#include <string>

namespace winrt::ZipSpark_New::implementation
{
    struct ArchiveEntry
    {
        std::wstring name;
        std::wstring path;
        uint64_t size;
        bool isDirectory;
        
        std::wstring GetSizeText() const
        {
            if (isDirectory) return L"";
            
            double kb = size / 1024.0;
            if (kb < 1024)
                return std::to_wstring(static_cast<int>(kb)) + L" KB";
            
            double mb = kb / 1024.0;
            if (mb < 1024)
                return std::to_wstring(static_cast<int>(mb)) + L" MB";
            
            double gb = mb / 1024.0;
            return std::to_wstring(static_cast<int>(gb)) + L" GB";
        }
    };

    struct PreviewWindow : PreviewWindowT<PreviewWindow>
    {
        PreviewWindow();
        PreviewWindow(winrt::hstring archivePath);

        void SearchBox_TextChanged(winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBox const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxTextChangedEventArgs const& args);
        void SelectAllButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void FileTreeView_ItemInvoked(winrt::Microsoft::UI::Xaml::Controls::TreeView const& sender, winrt::Microsoft::UI::Xaml::Controls::TreeViewItemInvokedEventArgs const& args);
        void ExtractButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void CancelButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        std::vector<std::wstring> GetSelectedFiles();

    private:
        void LoadArchiveContents();
        void UpdateSummary();
        void FilterFiles(const std::wstring& searchText);

        std::wstring m_archivePath;
        std::vector<ArchiveEntry> m_entries;
        std::vector<ArchiveEntry> m_filteredEntries;
    };
}

namespace winrt::ZipSpark_New::factory_implementation
{
    struct PreviewWindow : PreviewWindowT<PreviewWindow, implementation::PreviewWindow>
    {
    };
}
