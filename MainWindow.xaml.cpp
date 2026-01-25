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
#include "Utils/NotificationManager.h"
#include "Utils/RecentFiles.h"
#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.UI.Text.h>
#include <microsoft.ui.xaml.window.h>
#include <Shobjidl.h>
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;
using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;

namespace winrt::ZipSpark_New::implementation
{
    MainWindow::MainWindow()
    {
        try
        {
            LOG_INFO(L"MainWindow() constructor called (no archive path)");
            
            LOG_INFO(L"Calling InitializeComponent...");
            InitializeComponent();
            LOG_INFO(L"InitializeComponent completed");
            
            // Set window title
            LOG_INFO(L"Setting window title");
            this->Title(L"ZipSpark");
            
            // Set window size
            LOG_INFO(L"Setting window size to 720x600");
            this->AppWindow().Resize({ 720, 600 });
            
            LOG_INFO(L"MainWindow() constructor completed successfully");
        }
        catch (const winrt::hresult_error& ex)
        {
            std::wstring message = ex.message().c_str();
            LOG_ERROR(L"WinRT error in MainWindow constructor: " + message + L" (HRESULT: " + std::to_wstring(ex.code()) + L")");
            throw;
        }
        catch (const std::exception& ex)
        {
            std::string what = ex.what();
            std::wstring wwhat(what.begin(), what.end());
            LOG_ERROR(L"Exception in MainWindow constructor: " + wwhat);
            throw;
        }
        catch (...)
        {
            LOG_ERROR(L"Unknown exception in MainWindow constructor");
            throw;
        }
    }

    MainWindow::MainWindow(winrt::hstring archivePath) : MainWindow()
    {
        try
        {
            LOG_INFO(L"MainWindow(archivePath) constructor called with: " + std::wstring(archivePath));
            
            m_archivePath = archivePath.c_str();
            LOG_INFO(L"Archive path set to: " + m_archivePath);
            
            // CRITICAL FIX: Defer extraction until after window is activated
            // Calling StartExtraction during construction causes UI thread blocking
            if (!m_archivePath.empty())
            {
                LOG_INFO(L"Deferring extraction until window is activated");
                
                // Use DispatcherQueue to defer extraction until after activation
                auto strong_this = get_strong();
                DispatcherQueue().TryEnqueue([strong_this]() {
                    LOG_INFO(L"Window activated, starting deferred extraction");
                    strong_this->StartExtraction(strong_this->m_archivePath);
                });
            }
            
            LOG_INFO(L"MainWindow(archivePath) constructor completed");
        }
        catch (const std::exception& ex)
        {
            std::string what = ex.what();
            std::wstring wwhat(what.begin(), what.end());
            LOG_ERROR(L"Exception in MainWindow(archivePath) constructor: " + wwhat);
            throw;
        }
        catch (...)
        {
            LOG_ERROR(L"Unknown exception in MainWindow(archivePath) constructor");
            throw;
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

    winrt::fire_and_forget MainWindow::ExtractButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        try
        {
            LOG_INFO(L"ExtractButton_Click called");
            
            // Show file picker to select archive
            LOG_INFO(L"Creating FileOpenPicker");
            auto picker = winrt::Windows::Storage::Pickers::FileOpenPicker();
            
            // Initialize with window handle
            LOG_INFO(L"Initializing picker with window handle");
            auto initializeWithWindow = picker.as<::IInitializeWithWindow>();
            auto windowNative = this->try_as<::IWindowNative>();
            HWND hwnd;
            windowNative->get_WindowHandle(&hwnd);
            initializeWithWindow->Initialize(hwnd);
            LOG_INFO(L"Picker initialized successfully");
            
            // Set file type filters
            LOG_INFO(L"Setting file type filters");
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
            LOG_INFO(L"Showing file picker dialog");
            auto file = co_await picker.PickSingleFileAsync();
            
            if (file)
            {
                std::wstring path = file.Path().c_str();
                LOG_INFO(L"File selected: " + path);
                
                // Add to recent files
                LOG_INFO(L"Adding to recent files");
                ZipSpark::RecentFiles::GetInstance().AddFile(path);
                
                LOG_INFO(L"Starting extraction");
                StartExtraction(path);
            }
            else
            {
                LOG_INFO(L"File picker cancelled by user");
            }
        }
        catch (const winrt::hresult_error& ex)
        {
            std::wstring message = ex.message().c_str();
            LOG_ERROR(L"WinRT error in ExtractButton_Click: " + message + L" (HRESULT: " + std::to_wstring(ex.code()) + L")");
            ShowErrorDialog(L"Error", L"Failed to open file picker: " + message);
        }
        catch (const std::exception& ex)
        {
            std::string what = ex.what();
            std::wstring wwhat(what.begin(), what.end());
            LOG_ERROR(L"Error in ExtractButton_Click: " + wwhat);
            ShowErrorDialog(L"Error", L"Failed to open file picker: " + wwhat);
        }
        catch (...)
        {
            LOG_ERROR(L"Unknown error in ExtractButton_Click");
            ShowErrorDialog(L"Error", L"An unexpected error occurred while opening the file picker.");
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

    winrt::fire_and_forget MainWindow::StartExtraction(const std::wstring& archivePath)
    {
        // Capture strong reference immediately
        auto strong_this = get_strong();
        
        if (m_extracting)
        {
            LOG_WARNING(L"Extraction already in progress, ignoring request");
            co_return;
        }
        
        m_extracting = true;
        m_archivePath = archivePath;
        
        LOG_INFO(L"Starting extraction for: " + archivePath);
        
        try
        {
            // Show progress UI immediately on UI thread
            ShowExtractionProgress();
            StatusText().Text(L"Scanning archive...");
            FileProgressBar().IsIndeterminate(true);
            
            // Switch to background thread for all heavy operations
            co_await winrt::resume_background();
            
            LOG_INFO(L"Switched to background thread for extraction");
            
            // Create extraction engine
            try 
            {
                strong_this->m_engine = ZipSpark::EngineFactory::CreateEngine(archivePath);
            }
            catch (const std::exception& ex)
            {
                std::string what = ex.what();
                std::wstring wwhat(what.begin(), what.end());
                LOG_ERROR(L"Exception creating engine: " + wwhat);
                strong_this->m_engine = nullptr;
            }
            catch (...)
            {
                LOG_ERROR(L"Unknown exception creating engine");
                strong_this->m_engine = nullptr;
            }

            if (!strong_this->m_engine)
            {
                LOG_ERROR(L"Failed to create extraction engine");
                // Switch back to UI thread for error handling
                auto dispatcher = strong_this->DispatcherQueue();
                co_await winrt::resume_foreground(dispatcher);
                strong_this->OnError(ZipSpark::ErrorCode::UnsupportedFormat, L"Unsupported archive format or failed to initialize engine");
                strong_this->m_extracting = false;
                co_return;
            }
            
            // Get archive info
            ZipSpark::ArchiveInfo info;
            try 
            {
                LOG_INFO(L"Getting archive info");
                info = strong_this->m_engine->GetArchiveInfo(archivePath);
                LOG_INFO(L"Archive info retrieved successfully");
            }
            catch (const std::exception& ex)
            {
                std::string what = ex.what();
                std::wstring wwhat(what.begin(), what.end());
                LOG_ERROR(L"Exception getting archive info: " + wwhat);
                auto dispatcher = strong_this->DispatcherQueue();
                co_await winrt::resume_foreground(dispatcher);
                strong_this->OnError(ZipSpark::ErrorCode::ArchiveNotFound, L"Failed to read archive information: " + wwhat);
                strong_this->m_extracting = false;
                co_return;
            }
            catch (...)
            {
                LOG_ERROR(L"Unknown exception getting archive info");
                auto dispatcher = strong_this->DispatcherQueue();
                co_await winrt::resume_foreground(dispatcher);
                strong_this->OnError(ZipSpark::ErrorCode::ArchiveNotFound, L"Failed to read archive information");
                strong_this->m_extracting = false;
                co_return;
            }
            
            // Update UI with archive info (switch back to UI thread)
            auto dispatcher = strong_this->DispatcherQueue();
            co_await winrt::resume_foreground(dispatcher);
            
            try
            {
                // Hide empty state elements
                strong_this->DropZoneTitle().Visibility(Visibility::Collapsed);
                strong_this->SupportedFormatsPanel().Visibility(Visibility::Collapsed);
                strong_this->BrowseButton().Visibility(Visibility::Collapsed);
                
                // Show archive info
                strong_this->ArchivePathText().Text(L"Extracting: " + winrt::hstring(info.archivePath));
                strong_this->ArchivePathText().Visibility(Visibility::Visible);
                
                std::wstring sizeStr = std::to_wstring(info.totalSize / (1024 * 1024)) + L" MB";
                std::wstring fileCountStr = (info.fileCount > 0) ? std::to_wstring(info.fileCount) + L" files" : L"Scanning...";
                strong_this->ArchiveInfoText().Text(winrt::hstring(fileCountStr + L" • " + sizeStr));
                strong_this->ArchiveInfoText().Visibility(Visibility::Visible);
            }
            catch(const std::exception& ex) 
            {
                std::string what = ex.what();
                std::wstring wwhat(what.begin(), what.end());
                LOG_ERROR(L"Exception updating UI: " + wwhat);
            }
            catch(...) 
            {
                LOG_ERROR(L"Unknown exception updating UI");
            }
            
            // Switch back to background thread for extraction
            co_await winrt::resume_background();
            
            // Set extraction options
            ZipSpark::ExtractionOptions options;
            options.createSubfolder = !info.hasSingleRoot;
            options.overwritePolicy = ZipSpark::OverwritePolicy::AutoRename;
            
            LOG_INFO(L"Starting extraction with thread-safe callbacks");
            
            try
            {
                // Use thread-safe callback wrapper to marshal all callbacks to UI thread
                // This prevents Access Violations when calling WinRT object methods from background thread
                ThreadSafeCallback safeCallback(strong_this->DispatcherQueue(), strong_this->get_weak());
                strong_this->m_engine->Extract(info, options, &safeCallback);
                LOG_INFO(L"Extraction completed");
            }
            catch (const std::exception& e)
            {
                std::string what = e.what();
                std::wstring wwhat(what.begin(), what.end());
                LOG_ERROR(L"Extraction exception: " + wwhat);
                auto dispatcher2 = strong_this->DispatcherQueue();
                co_await winrt::resume_foreground(dispatcher2);
                strong_this->OnError(ZipSpark::ErrorCode::ExtractionFailed, L"Extraction failed: " + wwhat);
            }
            catch (...)
            {
                LOG_ERROR(L"Unknown extraction exception");
                auto dispatcher2 = strong_this->DispatcherQueue();
                co_await winrt::resume_foreground(dispatcher2);
                strong_this->OnError(ZipSpark::ErrorCode::ExtractionFailed, L"Extraction failed (Unknown Error)");
            }
            
            strong_this->m_extracting = false;
        }
        catch (const winrt::hresult_error& ex)
        {
            // WinRT-specific errors
            std::wstring message = ex.message().c_str();
            LOG_ERROR(L"WinRT error in StartExtraction: " + message);
            strong_this->m_extracting = false;
            auto dispatcher = strong_this->DispatcherQueue();
            co_await winrt::resume_foreground(dispatcher);
            strong_this->OnError(ZipSpark::ErrorCode::ExtractionFailed, L"WinRT Error: " + message);
        }
        catch (const std::exception& ex)
        {
            // Standard C++ exceptions
            std::string what = ex.what();
            std::wstring wwhat(what.begin(), what.end());
            LOG_ERROR(L"Exception in StartExtraction: " + wwhat);
            strong_this->m_extracting = false;
            auto dispatcher = strong_this->DispatcherQueue();
            co_await winrt::resume_foreground(dispatcher);
            strong_this->OnError(ZipSpark::ErrorCode::ExtractionFailed, L"Error: " + wwhat);
        }
        catch (...)
        {
            // Catch all other exceptions
            LOG_ERROR(L"Unknown exception in StartExtraction");
            strong_this->m_extracting = false;
            auto dispatcher = strong_this->DispatcherQueue();
            co_await winrt::resume_foreground(dispatcher);
            strong_this->OnError(ZipSpark::ErrorCode::ExtractionFailed, L"An unexpected error occurred during extraction.");
        }
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
        
        // Reset to empty state
        DropZoneTitle().Visibility(Visibility::Visible);
        SupportedFormatsPanel().Visibility(Visibility::Visible);
        BrowseButton().Visibility(Visibility::Visible);
        ArchivePathText().Text(L"");
        ArchivePathText().Visibility(Visibility::Collapsed);
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

    winrt::fire_and_forget MainWindow::Grid_Drop(IInspectable const&, DragEventArgs const& e)
    {
        try
        {
            LOG_INFO(L"Grid_Drop called - file dropped onto window");
            
            // Get dropped files
            if (e.DataView().Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::StorageItems()))
            {
                LOG_INFO(L"DataView contains storage items");
                
                // Use co_await instead of .get() to avoid blocking UI thread
                LOG_INFO(L"Getting storage items asynchronously");
                auto items = co_await e.DataView().GetStorageItemsAsync();
                LOG_INFO(L"Retrieved " + std::to_wstring(items.Size()) + L" items");
                
                if (items.Size() > 0)
                {
                    auto file = items.GetAt(0).try_as<winrt::Windows::Storage::StorageFile>();
                    if (file)
                    {
                        std::wstring path = file.Path().c_str();
                        LOG_INFO(L"File dropped: " + path);
                        
                        // Start extraction
                        LOG_INFO(L"Starting extraction for dropped file");
                        StartExtraction(path);
                    }
                    else
                    {
                        LOG_WARNING(L"Dropped item is not a file");
                    }
                }
                else
                {
                    LOG_WARNING(L"No items in dropped data");
                }
            }
            else
            {
                LOG_WARNING(L"DataView does not contain storage items");
            }
        }
        catch (const winrt::hresult_error& ex)
        {
            std::wstring message = ex.message().c_str();
            LOG_ERROR(L"WinRT error in Grid_Drop: " + message + L" (HRESULT: " + std::to_wstring(ex.code()) + L")");
        }
        catch (const std::exception& ex)
        {
            std::string what = ex.what();
            std::wstring wwhat(what.begin(), what.end());
            LOG_ERROR(L"Error in Grid_Drop: " + wwhat);
        }
        catch (...)
        {
            LOG_ERROR(L"Unknown error in Grid_Drop");
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
            
            // Calculate extraction speed
            auto now = std::chrono::steady_clock::now();
            auto timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastSpeedUpdate).count();
            
            if (timeSinceLastUpdate >= 500) // Update speed every 500ms
            {
                uint64_t bytesDelta = bytesProcessed - m_lastBytesProcessed;
                double secondsElapsed = timeSinceLastUpdate / 1000.0;
                double speedMBps = (bytesDelta / (1024.0 * 1024.0)) / secondsElapsed;
                
                SpeedText().Text(winrt::hstring(std::to_wstring(static_cast<int>(speedMBps)) + L" MB/s"));
                
                m_lastBytesProcessed = bytesProcessed;
                m_lastSpeedUpdate = now;
            }
            
            double mbProcessed = bytesProcessed / (1024.0 * 1024.0);
            double mbTotal = totalBytes / (1024.0 * 1024.0);
            StatusText().Text(winrt::hstring(
                std::to_wstring(static_cast<int>(mbProcessed)) + L" MB / " +
                std::to_wstring(static_cast<int>(mbTotal)) + L" MB"
            ));

            // Update taskbar progress (Safe here on UI thread)
            auto windowNative = this->try_as<::IWindowNative>();
            if (windowNative)
            {
                HWND hwnd;
                windowNative->get_WindowHandle(&hwnd);
                ZipSpark::NotificationManager::GetInstance().UpdateTaskbarProgress(hwnd, percent, 100);
            }
        });
    }

    // IProgressCallback implementation
    void MainWindow::OnStart(int totalFiles)
    {
        LOG_INFO(L"Extraction started, total files: " + std::to_wstring(totalFiles));
        
        // Initialize progress tracking
        m_totalFiles = totalFiles;
        m_currentFileIndex = 0;
        m_extractionStartTime = std::chrono::steady_clock::now();
        m_lastSpeedUpdate = m_extractionStartTime;
        m_lastUIUpdate = m_extractionStartTime;
        m_lastFileUIUpdate = m_extractionStartTime;
        m_lastBytesProcessed = 0;
        
        DispatcherQueue().TryEnqueue([this]() {
            StatusText().Text(L"Starting extraction...");
            FileProgressBar().IsIndeterminate(true);
        });
    }

    void MainWindow::OnProgress(int percentComplete, uint64_t bytesProcessed, uint64_t totalBytes)
    {
        // Throttle updates to ~10fps (100ms) to prevent UI thread starvation
        auto now = std::chrono::steady_clock::now();
        if (percentComplete < 100 && 
            std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastUIUpdate).count() < 100)
        {
            return;
        }
        m_lastUIUpdate = now;

        UpdateProgressUI(percentComplete, bytesProcessed, totalBytes);
    }

    void MainWindow::OnFileProgress(const std::wstring& currentFile, int fileIndex, int totalFiles)
    {
        m_currentFileIndex = fileIndex;
        
        // Throttle file progress updates too (100ms)
        auto now = std::chrono::steady_clock::now();
        if (fileIndex < totalFiles && 
            std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastFileUIUpdate).count() < 100)
        {
            return;
        }
        m_lastFileUIUpdate = now;
        
        DispatcherQueue().TryEnqueue([this, currentFile, fileIndex, totalFiles]() {
            // Show file count
            std::wstring fileCountText = L"File " + std::to_wstring(fileIndex + 1) + L" of " + std::to_wstring(totalFiles);
            CurrentFileText().Text(winrt::hstring(fileCountText + L": " + currentFile));
            FileProgressBar().IsIndeterminate(false);
            
            if (totalFiles > 0)
            {
                int filePercent = ((fileIndex + 1) * 100) / totalFiles;
                FileProgressBar().Value(filePercent);
            }
        });
    }

    void MainWindow::OnComplete(const std::wstring& destination)
    {
        LOG_INFO(L"Extraction completed: " + destination);
        
        // Dispatch UI updates and notification to UI thread
        DispatcherQueue().TryEnqueue([this, destination]() {
            // Show notification
            fs::path archivePath(m_archivePath);
            ZipSpark::NotificationManager::GetInstance().ShowExtractionComplete(
                archivePath.filename().wstring(), 
                destination
            );
            
            // Reset taskbar progress
            auto windowNative = this->try_as<::IWindowNative>();
            if (windowNative)
            {
                HWND hwnd;
                windowNative->get_WindowHandle(&hwnd);
                ZipSpark::NotificationManager::GetInstance().SetTaskbarState(hwnd, 0); // TBPF_NOPROGRESS
            }
            
            HideExtractionProgress();
            m_extracting = false;
            
            // Reset to empty state
            DropZoneTitle().Visibility(Visibility::Visible);
            SupportedFormatsPanel().Visibility(Visibility::Visible);
            BrowseButton().Visibility(Visibility::Visible);
            ArchivePathText().Text(L"");
            ArchivePathText().Visibility(Visibility::Collapsed);
            ArchiveInfoText().Visibility(Visibility::Collapsed);
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
        intlHeader.Margin({ 0, 16, 0, 0 });
        content.Children().Append(intlHeader);
        
        Controls::HyperlinkButton paypalLink;
        paypalLink.Content(winrt::box_value(L"Donate via PayPal"));
        paypalLink.NavigateUri(winrt::Windows::Foundation::Uri(L"https://paypal.me/yourpaypal"));
        content.Children().Append(paypalLink);
        
        Controls::HyperlinkButton kofiLink;
        kofiLink.Content(winrt::box_value(L"Buy Me a Coffee"));
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
        githubLink.Content(winrt::box_value(L"View on GitHub"));
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
