#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
#include "PreferencesWindow.xaml.h"

#include "Engine/EngineFactory.h"
#include "Core/ArchiveInfo.h"
#include "Core/ExtractionOptions.h"
#include "Utils/Logger.h"
#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Storage.h>
#include <microsoft.ui.xaml.window.h>
#include <Shobjidl.h>
#include <thread>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::ZipSpark_New::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
        
        // Set window title
        this->Title(L"ZipSpark");
        
        // Set window size
        this->AppWindow().Resize({ 720, 600 });
    }

    MainWindow::MainWindow(winrt::hstring archivePath) : MainWindow()
    {
        m_archivePath = archivePath.c_str();
        
        // Auto-start extraction if archive path provided
        if (!m_archivePath.empty())
        {
            StartExtraction(m_archivePath);
        }
    }

    void MainWindow::SetupMicaBackdrop()
    {
        try
        {
            // Check if Mica is supported
            if (Microsoft::UI::Composition::SystemBackdrops::MicaController::IsSupported())
            {
                auto micaController = Microsoft::UI::Composition::SystemBackdrops::MicaController();
                
                // Get the window as ICompositionSupportsSystemBackdrop
                auto windowNative = this->try_as<Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop>();
                if (windowNative)
                {
                    micaController.AddSystemBackdropTarget(windowNative);
                }
            }
        }
        catch (...)
        {
            // Fallback to solid color if Mica fails
        }
    }

    void MainWindow::ExtractButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        // Show file picker to select archive
        auto picker = winrt::Windows::Storage::Pickers::FileOpenPicker();
        
        // Initialize with window handle
        auto initializeWithWindow = picker.as<::IInitializeWithWindow>();
        auto windowNative = this->try_as<::IWindowNative>();
        HWND hwnd;
        windowNative->get_WindowHandle(&hwnd);
        initializeWithWindow->Initialize(hwnd);
        
        // Set file type filters
        picker.FileTypeFilter().Append(L".zip");
        picker.FileTypeFilter().Append(L".7z");
        picker.FileTypeFilter().Append(L".rar");
        picker.FileTypeFilter().Append(L".tar");
        picker.FileTypeFilter().Append(L".gz");
        picker.FileTypeFilter().Append(L".tgz");
        picker.FileTypeFilter().Append(L".txz");
        picker.FileTypeFilter().Append(L".xz");
        
        picker.SuggestedStartLocation(winrt::Windows::Storage::Pickers::PickerLocationId::Downloads);
        
        // Show picker
        auto file = picker.PickSingleFileAsync().get();
        if (file)
        {
            StartExtraction(file.Path().c_str());
        }
    }

    void MainWindow::ShowErrorDialog(const std::wstring& title, const std::wstring& message)
    {
        DispatcherQueue().TryEnqueue([this, title, message]() {
            Controls::ContentDialog dialog;
            dialog.XamlRoot(this->Content().XamlRoot());
            dialog.Title(winrt::box_value(winrt::hstring(title)));
            dialog.Content(winrt::box_value(winrt::hstring(message)));
            dialog.CloseButtonText(L"OK");
            dialog.DefaultButton(Controls::ContentDialogButton::Close);
            
            dialog.ShowAsync();
        });
    }

    void MainWindow::StartExtraction(const std::wstring& archivePath)
    {
        if (m_extracting)
            return;
        
        m_extracting = true;
        m_archivePath = archivePath;
        
        LOG_INFO(L"Starting extraction for: " + archivePath);
        
        // Show progress UI
        ShowExtractionProgress();
        
        // Create extraction engine
        m_engine = ZipSpark::EngineFactory::CreateEngine(archivePath);
        if (!m_engine)
        {
            OnError(ZipSpark::ErrorCode::UnsupportedFormat, L"Unsupported archive format");
            m_extracting = false;
            return;
        }
        
        // Get archive info
        ZipSpark::ArchiveInfo info = m_engine->GetArchiveInfo(archivePath);
        
        // Update UI with archive info
        DispatcherQueue().TryEnqueue([this, info]() {
            ArchivePathText().Text(L"Extracting: " + winrt::hstring(info.archivePath));
            
            std::wstring sizeStr = std::to_wstring(info.totalSize / (1024 * 1024)) + L" MB";
            std::wstring fileCountStr = std::to_wstring(info.fileCount) + L" files";
            ArchiveInfoText().Text(winrt::hstring(fileCountStr + L" • " + sizeStr));
            ArchiveInfoText().Visibility(Visibility::Visible);
        });
        
        // Set extraction options
        ZipSpark::ExtractionOptions options;
        options.createSubfolder = !info.hasSingleRoot;
        options.overwritePolicy = ZipSpark::OverwritePolicy::AutoRename;
        
        // Start extraction on background thread
        std::thread extractionThread([this, info, options]() {
            try
            {
                m_engine->Extract(info, options, this);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR(L"Extraction exception: " + std::wstring(e.what(), e.what() + strlen(e.what())));
                OnError(ZipSpark::ErrorCode::ExtractionFailed, L"Extraction failed");
            }
            m_extracting = false;
        });
        
        extractionThread.detach();
    }

    void MainWindow::CancelButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_engine && m_extracting)
        {
            m_engine->Cancel();
            LOG_INFO(L"User cancelled extraction");
        }
        
        HideExtractionProgress();
        m_extracting = false;
        
        ArchivePathText().Text(L"Drop an archive or use file associations");
        ArchiveInfoText().Visibility(Visibility::Collapsed);
    }

    void MainWindow::PreferencesButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        // Create and show preferences window
        auto prefsWindow = winrt::make<PreferencesWindow>();
        prefsWindow.Activate();
    }

    void MainWindow::Grid_DragOver(IInspectable const&, DragEventArgs const& e)
    {
        // Check if dragged item contains files
        if (e.DataView().Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::StorageItems()))
        {
            e.AcceptedOperation(winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation::Copy);
            e.DragUIOverride().Caption(L"Drop to extract");
            e.DragUIOverride().IsCaptionVisible(true);
            e.DragUIOverride().IsContentVisible(true);
        }
        else
        {
            e.AcceptedOperation(winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation::None);
        }
    }

    void MainWindow::Grid_Drop(IInspectable const&, DragEventArgs const& e)
    {
        // Get dropped files
        if (e.DataView().Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::StorageItems()))
        {
            auto items = e.DataView().GetStorageItemsAsync().get();
            if (items.Size() > 0)
            {
                auto file = items.GetAt(0).try_as<winrt::Windows::Storage::StorageFile>();
                if (file)
                {
                    std::wstring path = file.Path().c_str();
                    LOG_INFO(L"File dropped: " + path);
                    
                    // Start extraction
                    StartExtraction(path);
                }
            }
        }
    }

    void MainWindow::ShowExtractionProgress()
    {
        ProgressSection().Visibility(Visibility::Visible);
        ExtractButton().Visibility(Visibility::Collapsed);
        CancelButton().Visibility(Visibility::Visible);
    }

    void MainWindow::HideExtractionProgress()
    {
        ProgressSection().Visibility(Visibility::Collapsed);
        ExtractButton().Visibility(Visibility::Visible);
        CancelButton().Visibility(Visibility::Collapsed);
        
        OverallProgressBar().Value(0);
        FileProgressBar().Value(0);
        OverallProgressText().Text(L"0%");
        CurrentFileText().Text(L"Extracting...");
        SpeedText().Text(L"0 MB/s");
        StatusText().Text(L"Preparing extraction...");
    }

    void MainWindow::UpdateProgressUI(int percent, uint64_t bytesProcessed, uint64_t totalBytes)
    {
        DispatcherQueue().TryEnqueue([this, percent, bytesProcessed, totalBytes]() {
            OverallProgressBar().Value(percent);
            OverallProgressText().Text(winrt::hstring(std::to_wstring(percent) + L"%"));
            
            double mbProcessed = bytesProcessed / (1024.0 * 1024.0);
            double mbTotal = totalBytes / (1024.0 * 1024.0);
            StatusText().Text(winrt::hstring(
                std::to_wstring(static_cast<int>(mbProcessed)) + L" MB / " +
                std::to_wstring(static_cast<int>(mbTotal)) + L" MB"
            ));
        });
    }

    // IProgressCallback implementation
    void MainWindow::OnStart(int totalFiles)
    {
        LOG_INFO(L"Extraction started, total files: " + std::to_wstring(totalFiles));
        
        DispatcherQueue().TryEnqueue([this]() {
            StatusText().Text(L"Starting extraction...");
            FileProgressBar().IsIndeterminate(true);
        });
    }

    void MainWindow::OnProgress(int percentComplete, uint64_t bytesProcessed, uint64_t totalBytes)
    {
        UpdateProgressUI(percentComplete, bytesProcessed, totalBytes);
    }

    void MainWindow::OnFileProgress(const std::wstring& currentFile, int fileIndex, int totalFiles)
    {
        DispatcherQueue().TryEnqueue([this, currentFile, fileIndex, totalFiles]() {
            CurrentFileText().Text(winrt::hstring(currentFile));
            FileProgressBar().IsIndeterminate(false);
            
            if (totalFiles > 0)
            {
                int filePercent = (fileIndex * 100) / totalFiles;
                FileProgressBar().Value(filePercent);
            }
        });
    }

    void MainWindow::OnComplete(const std::wstring& destination)
    {
        LOG_INFO(L"Extraction completed: " + destination);
        
        DispatcherQueue().TryEnqueue([this, destination]() {
            // Update progress to 100%
            OverallProgressBar().Value(100);
            OverallProgressText().Text(L"100%");
            FileProgressBar().Value(100);
            FileProgressBar().IsIndeterminate(false);
            
            // Show completion message
            StatusText().Text(L"Extraction complete!");
            CurrentFileText().Text(L"All files extracted successfully");
            
            // Show success message in a dialog
            Controls::ContentDialog dialog;
            dialog.XamlRoot(this->Content().XamlRoot());
            dialog.Title(winrt::box_value(L"Success"));
            dialog.Content(winrt::box_value(L"Archive extracted successfully to:\n\n" + winrt::hstring(destination)));
            dialog.CloseButtonText(L"OK");
            dialog.DefaultButton(Controls::ContentDialogButton::Close);
            
            dialog.ShowAsync();
            
            // Reset UI after dialog is shown
            std::thread([this]() {
                std::this_thread::sleep_for(std::chrono::seconds(3));
                DispatcherQueue().TryEnqueue([this]() {
                    HideExtractionProgress();
                });
            }).detach();
        });
    }

    void MainWindow::OnError(ZipSpark::ErrorCode errorCode, const std::wstring& message)
    {
        LOG_ERROR(L"Extraction error: " + message);
        
        std::wstring title = L"Extraction Error";
        std::wstring fullMessage = message;
        
        // Add user-friendly context based on error code
        switch (errorCode)
        {
        case ZipSpark::ErrorCode::ArchiveNotFound:
            fullMessage = L"The archive file could not be found.\n\n" + message;
            break;
        case ZipSpark::ErrorCode::UnsupportedFormat:
            fullMessage = L"This archive format is not yet supported.\n\nCurrently supported: ZIP files only.\n\nComing soon: 7z, RAR, TAR, GZ, XZ";
            break;
        case ZipSpark::ErrorCode::ExtractionFailed:
            fullMessage = L"Failed to extract the archive.\n\n" + message;
            break;
        case ZipSpark::ErrorCode::InsufficientSpace:
            fullMessage = L"Not enough disk space to extract the archive.";
            break;
        case ZipSpark::ErrorCode::AccessDenied:
            fullMessage = L"Access denied. Check file permissions.";
            break;
        default:
            break;
        }
        
        ShowErrorDialog(title, fullMessage);
        
        DispatcherQueue().TryEnqueue([this]() {
            HideExtractionProgress();
        });
    }

    void MainWindow::DonateButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        // Create donate dialog
        Controls::ContentDialog dialog;
        dialog.XamlRoot(this->Content().XamlRoot());
        dialog.Title(winrt::box_value(L"Support ZipSpark ❤️"));
        dialog.CloseButtonText(L"Close");
        
        // Create content
        Controls::StackPanel content;
        content.Spacing(16);
        content.Padding({ 0, 8, 0, 0 });
        
        // Thank you message
        Controls::TextBlock thankYou;
        thankYou.Text(L"Thank you for considering a donation! Your support helps keep ZipSpark free and open-source.");
        thankYou.TextWrapping(TextWrapping::Wrap);
        content.Children().Append(thankYou);
        
        // UPI Section (India)
        Controls::TextBlock upiHeader;
        upiHeader.Text(L"UPI (India)");
        upiHeader.FontWeight(winrt::Windows::UI::Text::FontWeights::SemiBold());
        upiHeader.Margin({ 0, 8, 0, 0 });
        content.Children().Append(upiHeader);
        
        Controls::TextBox upiId;
        upiId.Text(L"9831060419@fam");
        upiId.IsReadOnly(true);
        content.Children().Append(upiId);
        
        Controls::Button copyUpiButton;
        copyUpiButton.Content(winrt::box_value(L"Copy UPI ID"));
        copyUpiButton.Click([upiId](auto&&, auto&&) {
            winrt::Windows::ApplicationModel::DataTransfer::DataPackage package;
            package.SetText(upiId.Text());
            winrt::Windows::ApplicationModel::DataTransfer::Clipboard::SetContent(package);
        });
        content.Children().Append(copyUpiButton);
        
        // International Section
        Controls::TextBlock intlHeader;
        intlHeader.Text(L"International");
        intlHeader.FontWeight(winrt::Windows::UI::Text::FontWeights::SemiBold());
        intlHeader.Margin({ 0, 16, 0, 0 });
        content.Children().Append(intlHeader);
        
        Controls::HyperlinkButton paypalLink;
        paypalLink.Content(winrt::box_value(L"💳 Donate via PayPal"));
        paypalLink.NavigateUri(winrt::Windows::Foundation::Uri(L"https://paypal.me/yourpaypal"));
        content.Children().Append(paypalLink);
        
        Controls::HyperlinkButton kofiLink;
        kofiLink.Content(winrt::box_value(L"☕ Buy Me a Coffee"));
        kofiLink.NavigateUri(winrt::Windows::Foundation::Uri(L"https://ko-fi.com/yourname"));
        content.Children().Append(kofiLink);
        
        dialog.Content(content);
        dialog.ShowAsync();
    }

    void MainWindow::AboutButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        // Create about dialog
        Controls::ContentDialog dialog;
        dialog.XamlRoot(this->Content().XamlRoot());
        dialog.Title(winrt::box_value(L"About ZipSpark"));
        dialog.CloseButtonText(L"Close");
        
        // Create content
        Controls::StackPanel content;
        content.Spacing(12);
        content.Padding({ 0, 8, 0, 0 });
        
        // App name and version
        Controls::TextBlock appName;
        appName.Text(L"ZipSpark");
        appName.FontSize(28);
        appName.FontWeight(winrt::Windows::UI::Text::FontWeights::Bold());
        content.Children().Append(appName);
        
        Controls::TextBlock version;
        version.Text(L"Version 1.0.0");
        content.Children().Append(version);
        
        // Description
        Controls::TextBlock description;
        description.Text(L"A fast, modern archive extraction utility for Windows.\n\nSupports ZIP, 7z, RAR, TAR, GZ, XZ and more.");
        description.TextWrapping(TextWrapping::Wrap);
        description.Margin({ 0, 8, 0, 0 });
        content.Children().Append(description);
        
        // GitHub link
        Controls::HyperlinkButton githubLink;
        githubLink.Content(winrt::box_value(L"🔗 View on GitHub"));
        githubLink.NavigateUri(winrt::Windows::Foundation::Uri(L"https://github.com/peter14l/ZipSpark"));
        githubLink.Margin({ 0, 8, 0, 0 });
        content.Children().Append(githubLink);
        
        // Copyright
        Controls::TextBlock copyright;
        copyright.Text(L"© 2026 ZipSpark. Open-source software.");
        copyright.Margin({ 0, 8, 0, 0 });
        content.Children().Append(copyright);
        
        dialog.Content(content);
        dialog.ShowAsync();
    }
}
