#pragma once

#include "PreferencesWindow.g.h"

namespace winrt::ZipSpark_New::implementation
{
    struct PreferencesWindow : PreferencesWindowT<PreferencesWindow>
    {
        PreferencesWindow();

        void SaveButton_Click(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ResetButton_Click(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ThemeCombo_SelectionChanged(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);

    private:
        void LoadSettings();
        void SaveSettings();
        void ApplyTheme(int themeIndex);
    };
}

namespace winrt::ZipSpark_New::factory_implementation
{
    struct PreferencesWindow : PreferencesWindowT<PreferencesWindow, implementation::PreferencesWindow>
    {
    };
}
