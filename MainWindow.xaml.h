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

        void ExtractButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void CancelButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void PreferencesButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void DonateButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void AboutButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        
        // Drag and drop handlers
        void Grid_DragOver(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::DragEventArgs const& e);
        void Grid_Drop(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::DragEventArgs const& e);

        // IProgressCallback implementation
        void OnStart(int totalFiles) override;
        void OnProgress(int percentComplete, uint64_t bytesProcessed, uint64_t totalBytes) override;
        void OnFileProgress(const std::wstring& currentFile, int fileIndex, int totalFiles) override;
        void OnComplete(const std::wstring& destination) override;
        void OnError(ZipSpark::ErrorCode errorCode, const std::wstring& message) override;

    private:
        void SetupMicaBackdrop();
        void ShowExtractionProgress();
        void HideExtractionProgress();
        void StartExtraction(const std::wstring& archivePath);
        void UpdateProgressUI(int percent, uint64_t bytesProcessed, uint64_t totalBytes);
        void ShowErrorDialog(const std::wstring& title, const std::wstring& message);
        
        std::wstring m_archivePath;
        std::unique_ptr<ZipSpark::IExtractionEngine> m_engine;
        std::atomic<bool> m_extracting{ false };
    };
}

namespace winrt::ZipSpark_New::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
