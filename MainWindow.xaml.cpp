#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::ZipSpark_New::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
        
        // Set up Mica backdrop
        SetupMicaBackdrop();
        
        // Set window size
        this->AppWindow().Resize({ 720, 600 });
        
        // Center window on screen
        auto displayArea = Microsoft::UI::Windowing::DisplayArea::GetFromWindowId(
            this->AppWindow().Id(),
            Microsoft::UI::Windowing::DisplayAreaFallback::Nearest
        );
        
        if (displayArea)
        {
            auto workArea = displayArea.WorkArea();
            auto windowSize = this->AppWindow().Size();
            
            int32_t x = (workArea.Width - windowSize.Width) / 2 + workArea.X;
            int32_t y = (workArea.Height - windowSize.Height) / 2 + workArea.Y;
            
            this->AppWindow().Move({ x, y });
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
        // TODO: Implement extraction logic in Phase 4
        // For now, just show the progress UI
        ShowExtractionProgress();
        
        // Simulate extraction progress (temporary)
        ArchivePathText().Text(L"Extracting: sample.zip");
        ArchiveInfoText().Text(L"150 files • 45.2 MB");
        ArchiveInfoText().Visibility(Visibility::Visible);
        StatusText().Text(L"Extracting files...");
    }

    void MainWindow::CancelButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        // TODO: Implement cancellation logic in Phase 4
        HideExtractionProgress();
        
        ArchivePathText().Text(L"Drop an archive or use file associations");
        ArchiveInfoText().Visibility(Visibility::Collapsed);
    }

    void MainWindow::PreferencesButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        // TODO: Open PreferencesWindow in Phase 3
        // For now, show a placeholder message
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
}
