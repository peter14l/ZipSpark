#pragma once

#include "MainWindow.g.h"

namespace winrt::ZipSpark_New::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        void ExtractButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void CancelButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void PreferencesButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        void SetupMicaBackdrop();
        void ShowExtractionProgress();
        void HideExtractionProgress();
    };
}

namespace winrt::ZipSpark_New::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
