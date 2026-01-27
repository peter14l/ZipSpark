#pragma once

#include "CreateArchiveDialog.g.h"
#include <vector>
#include <string>

namespace winrt::ZipSpark_New::implementation
{
    struct CreateArchiveDialog : CreateArchiveDialogT<CreateArchiveDialog>
    {
        CreateArchiveDialog();

        // IDL Properties
        winrt::hstring ArchiveName();
        winrt::hstring ArchiveFormat();
        winrt::hstring DestinationPath();
        int32_t CompressionLevel();
        bool WasConfirmed();

        // IDL Methods
        void Initialize(winrt::Windows::Foundation::Collections::IVectorView<winrt::hstring> filePaths);

        // Event Handlers
        void OnPrimaryButtonClick(winrt::Microsoft::UI::Xaml::Controls::ContentDialog const& sender,
                                   winrt::Microsoft::UI::Xaml::Controls::ContentDialogButtonClickEventArgs const& args);
        void OnCloseButtonClick(winrt::Microsoft::UI::Xaml::Controls::ContentDialog const& sender,
                                 winrt::Microsoft::UI::Xaml::Controls::ContentDialogButtonClickEventArgs const& args);
        void BrowseButton_Click(winrt::Windows::Foundation::IInspectable const& sender,
                                 winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void CompressionSlider_ValueChanged(winrt::Windows::Foundation::IInspectable const& sender,
                                             winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& e);

    private:
        std::vector<std::wstring> m_filePaths;
        std::wstring m_archiveName;
        std::wstring m_archiveFormat = L".zip";
        std::wstring m_destinationPath;
        int32_t m_compressionLevel = 2; // 0=Store, 1=Fast, 2=Normal, 3=Best
        bool m_wasConfirmed = false;

        void UpdateFileInfo();
        void UpdateCompressionLabel();
        uint64_t CalculateTotalSize();
        std::wstring FormatFileSize(uint64_t bytes);
    };
}

namespace winrt::ZipSpark_New::factory_implementation
{
    struct CreateArchiveDialog : CreateArchiveDialogT<CreateArchiveDialog, implementation::CreateArchiveDialog>
    {
    };
}
