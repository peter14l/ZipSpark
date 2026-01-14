#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "Engine/EngineFactory.h"
#include "Core/ArchiveInfo.h"
#include "Core/ExtractionOptions.h"
#include "Utils/Logger.h"
#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <thread>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::ZipSpark_New::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
        
        // Set window title
        this->Title(L"ZipSpark");
        
        // Set window size
        this->AppWindow().Resize({ 720, 600 });
        
        // Try to set up Mica backdrop (may fail on some systems)
        try
        {
            SetupMicaBackdrop();
        }
        catch (...)
        {
            // Ignore Mica errors - window will still work with solid background
        }
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
        // TODO: Show file picker to select archive
        // For now, use a test file
        StartExtraction(L"C:\\test.zip");
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
        // TODO: Open PreferencesWindow in Phase 3
        StatusText().Text(L"Preferences window coming soon...");
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
            OverallProgressBar().Value(100);
            OverallProgressText().Text(L"100%");
            StatusText().Text(L"Extraction complete!");
            
            // Hide progress after 2 seconds
            std::thread([this]() {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                DispatcherQueue().TryEnqueue([this]() {
                    HideExtractionProgress();
                    ArchivePathText().Text(L"Extraction completed successfully");
                });
            }).detach();
        });
    }

    void MainWindow::OnError(ZipSpark::ErrorCode errorCode, const std::wstring& message)
    {
        LOG_ERROR(L"Extraction error: " + message);
        
        DispatcherQueue().TryEnqueue([this, message]() {
            StatusText().Text(winrt::hstring(L"Error: " + message));
            HideExtractionProgress();
            
            // TODO: Show error dialog
        });
    }
}
