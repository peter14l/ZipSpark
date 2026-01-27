#include "pch.h"
#include "CreateArchiveDialog.xaml.h"
#if __has_include("CreateArchiveDialog.g.cpp")
#include "CreateArchiveDialog.g.cpp"
#endif

#include "Utils/Logger.h"
#include <filesystem>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <shobjidl.h>

namespace fs = std::filesystem;

namespace winrt::ZipSpark_New::implementation
{
    CreateArchiveDialog::CreateArchiveDialog()
    {
        InitializeComponent();
        UpdateCompressionLabel();
    }

    winrt::hstring CreateArchiveDialog::ArchiveName()
    {
        return winrt::hstring(m_archiveName);
    }

    winrt::hstring CreateArchiveDialog::ArchiveFormat()
    {
        return winrt::hstring(m_archiveFormat);
    }

    winrt::hstring CreateArchiveDialog::DestinationPath()
    {
        return winrt::hstring(m_destinationPath);
    }

    int32_t CreateArchiveDialog::CompressionLevel()
    {
        return m_compressionLevel;
    }

    bool CreateArchiveDialog::WasConfirmed()
    {
        return m_wasConfirmed;
    }

    void CreateArchiveDialog::Initialize(winrt::Windows::Foundation::Collections::IVectorView<winrt::hstring> filePaths)
    {
        m_filePaths.clear();
        for (const auto& path : filePaths)
        {
            m_filePaths.push_back(std::wstring(path));
        }

        // Set default archive name based on first file or folder
        if (!m_filePaths.empty())
        {
            fs::path firstPath(m_filePaths[0]);
            if (m_filePaths.size() == 1)
            {
                // Single file/folder: use its name
                m_archiveName = firstPath.stem().wstring();
            }
            else
            {
                // Multiple files: use parent folder name
                m_archiveName = firstPath.parent_path().filename().wstring();
                if (m_archiveName.empty())
                {
                    m_archiveName = L"Archive";
                }
            }
            
            // Set default destination to the parent folder of first file
            m_destinationPath = firstPath.parent_path().wstring();
        }

        // Update UI
        ArchiveNameBox().Text(winrt::hstring(m_archiveName));
        DestinationBox().Text(winrt::hstring(m_destinationPath));
        UpdateFileInfo();
    }

    void CreateArchiveDialog::UpdateFileInfo()
    {
        // Update file count
        size_t count = m_filePaths.size();
        std::wstring countText = std::to_wstring(count) + (count == 1 ? L" file selected" : L" files selected");
        FileCountText().Text(winrt::hstring(countText));

        // Calculate and display total size
        uint64_t totalSize = CalculateTotalSize();
        FileSizeText().Text(winrt::hstring(L"Total size: " + FormatFileSize(totalSize)));
    }

    uint64_t CreateArchiveDialog::CalculateTotalSize()
    {
        uint64_t total = 0;
        for (const auto& path : m_filePaths)
        {
            try
            {
                fs::path p(path);
                if (fs::is_directory(p))
                {
                    for (const auto& entry : fs::recursive_directory_iterator(p, fs::directory_options::skip_permission_denied))
                    {
                        if (fs::is_regular_file(entry))
                        {
                            total += fs::file_size(entry);
                        }
                    }
                }
                else if (fs::is_regular_file(p))
                {
                    total += fs::file_size(p);
                }
            }
            catch (...)
            {
                // Skip files we can't access
            }
        }
        return total;
    }

    std::wstring CreateArchiveDialog::FormatFileSize(uint64_t bytes)
    {
        const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
        int unitIndex = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024.0 && unitIndex < 4)
        {
            size /= 1024.0;
            unitIndex++;
        }

        wchar_t buffer[64];
        if (unitIndex == 0)
        {
            swprintf(buffer, 64, L"%.0f %s", size, units[unitIndex]);
        }
        else
        {
            swprintf(buffer, 64, L"%.2f %s", size, units[unitIndex]);
        }
        return buffer;
    }

    void CreateArchiveDialog::UpdateCompressionLabel()
    {
        const wchar_t* labels[] = { L"Store (No compression)", L"Fast", L"Normal", L"Best (Slowest)" };
        int level = static_cast<int>(CompressionSlider().Value());
        if (level >= 0 && level <= 3)
        {
            CompressionLevelText().Text(winrt::hstring(labels[level]));
            m_compressionLevel = level;
        }
    }

    void CreateArchiveDialog::CompressionSlider_ValueChanged(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&)
    {
        UpdateCompressionLabel();
    }

    void CreateArchiveDialog::BrowseButton_Click(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        try
        {
            // Use COM folder picker directly for better compatibility
            IFileDialog* pfd = nullptr;
            HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
            
            if (SUCCEEDED(hr))
            {
                DWORD dwOptions;
                pfd->GetOptions(&dwOptions);
                pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
                pfd->SetTitle(L"Select Destination Folder");
                
                // Set initial folder if we have a destination
                if (!m_destinationPath.empty())
                {
                    IShellItem* psiFolder = nullptr;
                    hr = SHCreateItemFromParsingName(m_destinationPath.c_str(), NULL, IID_PPV_ARGS(&psiFolder));
                    if (SUCCEEDED(hr))
                    {
                        pfd->SetFolder(psiFolder);
                        psiFolder->Release();
                    }
                }
                
                hr = pfd->Show(NULL);
                if (SUCCEEDED(hr))
                {
                    IShellItem* psiResult = nullptr;
                    hr = pfd->GetResult(&psiResult);
                    if (SUCCEEDED(hr))
                    {
                        PWSTR pszPath = nullptr;
                        hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                        if (SUCCEEDED(hr))
                        {
                            m_destinationPath = pszPath;
                            DestinationBox().Text(winrt::hstring(m_destinationPath));
                            CoTaskMemFree(pszPath);
                        }
                        psiResult->Release();
                    }
                }
                pfd->Release();
            }
        }
        catch (...)
        {
            LOG_ERROR(L"Failed to show folder picker");
        }
    }

    void CreateArchiveDialog::OnPrimaryButtonClick(
        winrt::Microsoft::UI::Xaml::Controls::ContentDialog const&,
        winrt::Microsoft::UI::Xaml::Controls::ContentDialogButtonClickEventArgs const& args)
    {
        // Get values from UI
        m_archiveName = std::wstring(ArchiveNameBox().Text());
        
        // Get format from combo box
        auto selectedItem = FormatComboBox().SelectedItem().as<winrt::Microsoft::UI::Xaml::Controls::ComboBoxItem>();
        if (selectedItem)
        {
            auto tag = selectedItem.Tag();
            if (tag)
            {
                m_archiveFormat = std::wstring(winrt::unbox_value<winrt::hstring>(tag));
            }
        }
        
        m_compressionLevel = static_cast<int32_t>(CompressionSlider().Value());
        
        // Validate
        if (m_archiveName.empty())
        {
            args.Cancel(true);
            // TODO: Show error
            return;
        }
        
        if (m_destinationPath.empty())
        {
            args.Cancel(true);
            return;
        }
        
        m_wasConfirmed = true;
        LOG_INFO(L"CreateArchiveDialog confirmed: " + m_archiveName + m_archiveFormat + L" at " + m_destinationPath);
    }

    void CreateArchiveDialog::OnCloseButtonClick(
        winrt::Microsoft::UI::Xaml::Controls::ContentDialog const&,
        winrt::Microsoft::UI::Xaml::Controls::ContentDialogButtonClickEventArgs const&)
    {
        m_wasConfirmed = false;
    }
}
