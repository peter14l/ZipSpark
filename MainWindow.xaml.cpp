#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
#include "PreferencesWindow.xaml.h"

#include "Engine/EngineFactory.h"
#include "Engine/SevenZipEngine.h"
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

    void MainWindow::Grid_DragOver(IInspectable const&, DragEventArgs const& e)
    {
        // Accept file drops
        e.AcceptedOperation(winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation::Copy);
        e.DragUIOverride().Caption(L"Drop to extract");
    }

    winrt::fire_and_forget MainWindow::Grid_Drop(IInspectable const&, DragEventArgs const& e)
    {
        try
        {
            LOG_INFO(L"Grid_Drop called");
            
            auto dataView = e.DataView();
            if (dataView.Contains(winrt::Windows::ApplicationModel::DataTransfer::StandardDataFormats::StorageItems()))
            {
                // Get dropped files asynchronously
                auto items = co_await dataView.GetStorageItemsAsync();
                if (items.Size() > 0)
                {
                    auto item = items.GetAt(0);
                    if (auto file = item.try_as<winrt::Windows::Storage::StorageFile>())
                    {
                        std::wstring path = file.Path().c_str();
                        LOG_INFO(L"File dropped: " + path);
                        
                        // Add to recent files
                        ZipSpark::RecentFiles::GetInstance().AddFile(path);
                        
                        // Start extraction
                        StartExtraction(path);
                    }
                }
            }
        }
        catch (const winrt::hresult_error& ex)
        {
            std::wstring message = ex.message().c_str();
            LOG_ERROR(L"WinRT error in Grid_Drop: " + message);
            ShowErrorDialog(L"Error", L"Failed to process dropped file: " + message);
        }
        catch (const std::exception& ex)
        {
            std::string what = ex.what();
            std::wstring wwhat(what.begin(), what.end());
            LOG_ERROR(L"Error in Grid_Drop: " + wwhat);
            ShowErrorDialog(L"Error", L"Failed to process dropped file: " + wwhat);
        }
        catch (...)
        {
            LOG_ERROR(L"Unknown error in Grid_Drop");
            ShowErrorDialog(L"Error", L"An unexpected error occurred while processing the dropped file.");
        }
    }

    void MainWindow::ShowSuccessMessage(const std::wstring& destination)
    {
        DispatcherQueue().TryEnqueue([this, destination]() {
            try
            {
                // Show success state in the UI
                DropZoneTitle().Text(L"\u2713 Extraction Complete!");
                DropZoneTitle().Visibility(Visibility::Visible);
                
                ArchivePathText().Text(L"Files extracted to:\n" + winrt::hstring(destination));
                ArchivePathText().Visibility(Visibility::Visible);
                
                // Remove auto-open countdown
                ArchiveInfoText().Visibility(Visibility::Collapsed);
                
                // Hide progress section
                ProgressSection().Visibility(Visibility::Collapsed);
                
                // Reset to empty state after 10 seconds
                auto strong_this = get_strong();
                DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Low, [strong_this]() {
                    // Wait 10 seconds
                    std::this_thread::sleep_for(std::chrono::seconds(10));
                    
                    // Reset UI on dispatcher thread
                    strong_this->DispatcherQueue().TryEnqueue([strong_this]() {
                        // Only reset if we haven't started a new extraction
                        if (!strong_this->m_extracting)
                        {
                            strong_this->DropZoneTitle().Text(L"Drop Archive Here");
                            strong_this->DropZoneTitle().Visibility(Visibility::Visible);
                            
                            strong_this->ArchivePathText().Visibility(Visibility::Collapsed);
                            strong_this->ArchiveInfoText().Visibility(Visibility::Collapsed);
                            
                            strong_this->SupportedFormatsPanel().Visibility(Visibility::Visible);
                            strong_this->BrowseButton().Visibility(Visibility::Visible);
                            
                            strong_this->m_extracting = false;
                        }
                    });
                });
            }
            catch (const std::exception& ex)
            {
                std::string what = ex.what();
                std::wstring wwhat(what.begin(), what.end());
                LOG_ERROR(L"Error in ShowSuccessMessage: " + wwhat);
            }
            catch (...)
            {
                LOG_ERROR(L"Unknown error in ShowSuccessMessage");
            }
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
                // Fix: Use member variable m_archivePath as the reference parameter 'archivePath' 
                // might be invalid if it came from a temporary string (e.g. Drop handler)
                strong_this->m_currentEngine = ZipSpark::EngineFactory::CreateEngine(strong_this->m_archivePath);
            }
            catch (...)
            {
                LOG_ERROR(L"Exception in CreateEngine");
                strong_this->m_currentEngine = nullptr;
            }
            
            if (!strong_this->m_currentEngine)
            {
                LOG_ERROR(L"Failed to create extraction engine");
                // Switch back to UI thread for error handling
                strong_this->DispatcherQueue().TryEnqueue([strong_this]() {
                    strong_this->OnError(ZipSpark::ErrorCode::UnsupportedFormat, L"Unsupported archive format or failed to initialize engine");
                    strong_this->m_extracting = false;
                });
                co_return;
            }
            
            // Get archive info
            ZipSpark::ArchiveInfo info;
            try 
            {
                LOG_INFO(L"Getting archive info");
                // Fix: Use member variable here too
                info = strong_this->m_currentEngine->GetArchiveInfo(strong_this->m_archivePath);
                LOG_INFO(L"Archive info retrieved successfully");
            }
            catch (const std::exception& ex)
            {
                std::string what = ex.what();
                std::wstring wwhat(what.begin(), what.end());
                LOG_ERROR(L"Exception getting archive info: " + wwhat);
                strong_this->DispatcherQueue().TryEnqueue([strong_this, wwhat]() {
                    strong_this->OnError(ZipSpark::ErrorCode::ArchiveNotFound, L"Failed to read archive information: " + wwhat);
                    strong_this->m_extracting = false;
                });
                co_return;
            }
            catch (...)
            {
                LOG_ERROR(L"Unknown exception getting archive info");
                strong_this->DispatcherQueue().TryEnqueue([strong_this]() {
                    strong_this->OnError(ZipSpark::ErrorCode::ArchiveNotFound, L"Failed to read archive information");
                    strong_this->m_extracting = false;
                });
                co_return;
            }
            
            // Update UI with archive info (switch back to UI thread)
            strong_this->DispatcherQueue().TryEnqueue([strong_this, info]() {
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
            });
            
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
                strong_this->DispatcherQueue().TryEnqueue([strong_this, wwhat]() {
                    strong_this->OnError(ZipSpark::ErrorCode::ExtractionFailed, L"Extraction failed: " + wwhat);
                });
            }
            catch (...)
            {
                LOG_ERROR(L"Unknown extraction exception");
                strong_this->DispatcherQueue().TryEnqueue([strong_this]() {
                    strong_this->OnError(ZipSpark::ErrorCode::ExtractionFailed, L"Extraction failed (Unknown Error)");
                });
            }
            
            strong_this->m_extracting = false;
        }
        catch (const winrt::hresult_error& ex)
        {
            // WinRT-specific errors
            std::wstring message = ex.message().c_str();
            LOG_ERROR(L"WinRT error in StartExtraction: " + message);
            strong_this->m_extracting = false;
            strong_this->DispatcherQueue().TryEnqueue([strong_this, message]() {
                strong_this->OnError(ZipSpark::ErrorCode::ExtractionFailed, L"WinRT Error: " + message);
            });
        }
        catch (const std::exception& ex)
        {
            // Standard C++ exceptions
            std::string what = ex.what();
            std::wstring wwhat(what.begin(), what.end());
            LOG_ERROR(L"Exception in StartExtraction: " + wwhat);
            strong_this->m_extracting = false;
            strong_this->DispatcherQueue().TryEnqueue([strong_this, wwhat]() {
                strong_this->OnError(ZipSpark::ErrorCode::ExtractionFailed, L"Error: " + wwhat);
            });
        }
        catch (...)
        {
            // Catch all other exceptions
            LOG_ERROR(L"Unknown exception in StartExtraction");
            strong_this->m_extracting = false;
            strong_this->DispatcherQueue().TryEnqueue([strong_this]() {
                strong_this->OnError(ZipSpark::ErrorCode::ExtractionFailed, L"An unexpected error occurred during extraction.");
            });
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
        m_lastBytesProcessed = 0;
        
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
        m_currentFileIndex = fileIndex;
        
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
            
            // Show success message with 10-second auto-reset
            ShowSuccessMessage(destination);
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

    void MainWindow::ShowCreationUI(const std::vector<std::wstring>& files, const std::wstring& format)
    {
        // Store state
        m_creationFiles = files;
        m_creationFormat = format;
        m_isCreating = true;
        
        LOG_INFO(L"ShowCreationUI called with " + std::to_wstring(files.size()) + L" files. Format: " + format);
        
        DispatcherQueue().TryEnqueue([this]() {
             SetupCreationView();
             
             // If implicit format, start immediately
             if (!m_creationFormat.empty() && m_creationFormat != L"dialog")
             {
                 StartCreation(m_creationFormat, m_creationFiles);
             }
        });
    }

    void MainWindow::SetupCreationView()
    {
        // Hide standard UI
        DropZoneTitle().Text(L"Preparing to Archive...");
        DropZoneTitle().Visibility(Visibility::Visible);
        SupportedFormatsPanel().Visibility(Visibility::Collapsed);
        BrowseButton().Visibility(Visibility::Collapsed);
        
        // Show info
        if (!m_creationFiles.empty())
        {
            std::wstring info = L"Selected " + std::to_wstring(m_creationFiles.size()) + L" files.";
            ArchiveInfoText().Text(winrt::hstring(info));
            ArchiveInfoText().Visibility(Visibility::Visible);
        }
    }

    winrt::fire_and_forget MainWindow::StartCreation(const std::wstring& format, const std::vector<std::wstring>& files)
    {
        auto strong_this = get_strong();
        
        // Show progress UI
        strong_this->StatusText().Text(L"Creating archive...");
        strong_this->ProgressSection().Visibility(Visibility::Visible);
        strong_this->FileProgressBar().IsIndeterminate(true);
        
        co_await winrt::resume_background();
        
        try 
        {
            if (files.empty()) return;
            
            // Determine destination: Same folder as first file
            std::filesystem::path firstFile(files[0]);
            std::wstring folderName = firstFile.parent_path().filename().wstring();
            if (files.size() == 1) folderName = firstFile.stem().wstring();
            
            std::wstring destPath = (firstFile.parent_path() / (folderName + format)).wstring();
            
            // Use SevenZipEngine directly for this feature
            auto engine = std::make_unique<ZipSpark::SevenZipEngine>();
            
            // Using ThreadSafeCallback to marshal back to UI thread
            // Allocated on stack since CreateArchive is blocking and we are in a coroutine frame
            winrt::weak_ref<implementation::MainWindow> weakThis = strong_this;
            ThreadSafeCallback callback(strong_this->DispatcherQueue(), weakThis);
            
            strong_this->m_currentEngine.reset(engine.release()); // Store engine to keep it alive
            strong_this->m_currentEngine->CreateArchive(destPath, files, format, &callback);
            
            // Engine will be cleaned up by m_currentEngine unique_ptr when replaced or reset
            // But we should probably reset it when done to release file handles
            strong_this->m_currentEngine.reset();
        }
        catch (...)
        {
             strong_this->DispatcherQueue().TryEnqueue([strong_this]() {
                 strong_this->OnError(ZipSpark::ErrorCode::UnknownError, L"Creation failed unexpectedly.");
             });
        }
    }
}
