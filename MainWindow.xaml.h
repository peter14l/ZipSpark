#pragma once

#include "MainWindow.g.h"
#include "Core/ExtractionProgress.h"
#include "Engine/IExtractionEngine.h"
#include <memory>

namespace winrt::ZipSpark_New::implementation
{
    struct MainWindow : MainWindowT<MainWindow>, ZipSpark::IProgressCallback
    {
        MainWindow();
        MainWindow(winrt::hstring archivePath);

        winrt::fire_and_forget ExtractButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void CancelButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void PreferencesButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void DonateButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void AboutButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        
        // Drag and drop handlers
        void Grid_DragOver(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::DragEventArgs const& e);
        winrt::fire_and_forget Grid_Drop(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::DragEventArgs const& e);

        // IProgressCallback implementation
        void OnStart(int totalFiles) override;
        void OnProgress(int percentComplete, uint64_t bytesProcessed, uint64_t totalBytes) override;
        void OnFileProgress(const std::wstring& currentFile, int fileIndex, int totalFiles) override;
        void OnComplete(const std::wstring& destination) override;
        void OnError(ZipSpark::ErrorCode errorCode, const std::wstring& message) override;

        // Creation Mode
        void ShowCreationUI(const std::vector<std::wstring>& files, const std::wstring& format);

    private:
        // Creation helpers
        void SetupCreationView();
        winrt::fire_and_forget StartCreation(const std::wstring& format, const std::vector<std::wstring>& files);

        // Thread-safe callback wrapper to marshal extraction callbacks to UI thread
        class ThreadSafeCallback : public ZipSpark::IProgressCallback
        {
        private:
            winrt::Microsoft::UI::Dispatching::DispatcherQueue m_dispatcher;
            winrt::weak_ref<implementation::MainWindow> m_weakTarget;
            
            // Throttling
            std::chrono::steady_clock::time_point m_lastUIUpdate;
            std::chrono::steady_clock::time_point m_lastFileUIUpdate;
            
        public:
            ThreadSafeCallback(
                winrt::Microsoft::UI::Dispatching::DispatcherQueue dispatcher,
                winrt::weak_ref<implementation::MainWindow> weakTarget)
                : m_dispatcher(dispatcher), m_weakTarget(weakTarget)
            {
                auto now = std::chrono::steady_clock::now();
                m_lastUIUpdate = now;
                m_lastFileUIUpdate = now;
            }
            
            void OnStart(int totalFiles) override
            {
                // Always send start
                bool enqueued = m_dispatcher.TryEnqueue([weakTarget = m_weakTarget, totalFiles]() {
                    if (auto target = weakTarget.get())
                    {
                        target->OnStart(totalFiles);
                    }
                });
                if (!enqueued)
                {
                    OutputDebugStringW(L"[ZipSpark] Failed to enqueue OnStart callback\n");
                }
            }
            
            void OnProgress(int percentComplete, uint64_t bytesProcessed, uint64_t totalBytes) override
            {
                // Throttling: 10fps (100ms)
                auto now = std::chrono::steady_clock::now();
                if (percentComplete < 100 && 
                    std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastUIUpdate).count() < 100)
                {
                    return;
                }
                m_lastUIUpdate = now;

                bool enqueued = m_dispatcher.TryEnqueue([weakTarget = m_weakTarget, percentComplete, bytesProcessed, totalBytes]() {
                    if (auto target = weakTarget.get())
                    {
                        target->OnProgress(percentComplete, bytesProcessed, totalBytes);
                    }
                });
                if (!enqueued)
                {
                    OutputDebugStringW(L"[ZipSpark] Failed to enqueue OnProgress callback\n");
                }
            }
            
            void OnFileProgress(const std::wstring& currentFile, int fileIndex, int totalFiles) override
            {
                // Throttling: 10fps (100ms)
                auto now = std::chrono::steady_clock::now();
                if (fileIndex < totalFiles && 
                    std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastFileUIUpdate).count() < 100)
                {
                    return;
                }
                m_lastFileUIUpdate = now;

                bool enqueued = m_dispatcher.TryEnqueue([weakTarget = m_weakTarget, currentFile, fileIndex, totalFiles]() {
                    if (auto target = weakTarget.get())
                    {
                        target->OnFileProgress(currentFile, fileIndex, totalFiles);
                    }
                });
                if (!enqueued)
                {
                    OutputDebugStringW(L"[ZipSpark] Failed to enqueue OnFileProgress callback\n");
                }
            }
            
            void OnComplete(const std::wstring& destination) override
            {
                // Always send complete
                bool enqueued = m_dispatcher.TryEnqueue([weakTarget = m_weakTarget, destination]() {
                    if (auto target = weakTarget.get())
                    {
                        target->OnComplete(destination);
                    }
                });
                if (!enqueued)
                {
                    OutputDebugStringW(L"[ZipSpark] Failed to enqueue OnComplete callback\n");
                }
            }
            
            void OnError(ZipSpark::ErrorCode code, const std::wstring& message) override
            {
                // Always send error
                bool enqueued = m_dispatcher.TryEnqueue([weakTarget = m_weakTarget, code, message]() {
                    if (auto target = weakTarget.get())
                    {
                        target->OnError(code, message);
                    }
                });
                if (!enqueued)
                {
                    OutputDebugStringW(L"[ZipSpark] Failed to enqueue OnError callback\n");
                }
            }
        };

    private:
        void SetupMicaBackdrop();
        void ShowExtractionProgress();
        void HideExtractionProgress();
        winrt::fire_and_forget StartExtraction(const std::wstring& archivePath);
        void UpdateProgressUI(int percent, uint64_t bytesProcessed, uint64_t totalBytes);
        void ShowErrorDialog(const std::wstring& title, const std::wstring& message);
        void ShowSuccessMessage(const std::wstring& destination);
        
        std::wstring m_archivePath;
        std::unique_ptr<ZipSpark::IExtractionEngine> m_engine;
        std::atomic<bool> m_extracting{ false };
        
        // Progress tracking
        std::chrono::steady_clock::time_point m_extractionStartTime;
        uint64_t m_lastBytesProcessed{ 0 };
        std::chrono::steady_clock::time_point m_lastSpeedUpdate;
        int m_totalFiles{ 0 };
        int m_currentFileIndex{ 0 };

        // Creation State
        std::vector<std::wstring> m_creationFiles;
        std::wstring m_creationFormat;
        bool m_isCreating{ false };

    };
}

namespace winrt::ZipSpark_New::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
