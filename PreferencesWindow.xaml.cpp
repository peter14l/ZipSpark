#include "pch.h"
#include "PreferencesWindow.xaml.h"
#if __has_include("PreferencesWindow.g.cpp")
#include "PreferencesWindow.g.cpp"
#endif
#include "Utils/Settings.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::ZipSpark_New::implementation
{
    PreferencesWindow::PreferencesWindow()
    {
        InitializeComponent();
        LoadSettings();
    }

    void PreferencesWindow::LoadSettings()
    {
        auto& settings = ZipSpark::Settings::GetInstance();
        settings.Load();

        // Load extraction settings
        CreateSubfolderToggle().IsOn(settings.createSubfolder);
        PreserveTimestampsToggle().IsOn(settings.preserveTimestamps);

        // Load behavior settings
        OverwritePolicyCombo().SelectedIndex(static_cast<int>(settings.overwritePolicy));
        CloseAfterExtractionToggle().IsOn(settings.closeAfterExtraction);

        // Load appearance settings
        ThemeCombo().SelectedIndex(static_cast<int>(settings.theme));

        // Load advanced settings
        EnableLoggingToggle().IsOn(settings.enableLogging);
        BufferSizeNumber().Value(settings.bufferSize / 1024); // Convert bytes to KB
    }

    void PreferencesWindow::SaveSettings()
    {
        auto& settings = ZipSpark::Settings::GetInstance();

        // Save extraction settings
        settings.createSubfolder = CreateSubfolderToggle().IsOn();
        settings.preserveTimestamps = PreserveTimestampsToggle().IsOn();

        // Save behavior settings
        settings.overwritePolicy = static_cast<ZipSpark::OverwritePolicy>(OverwritePolicyCombo().SelectedIndex());
        settings.closeAfterExtraction = CloseAfterExtractionToggle().IsOn();

        // Save appearance settings
        settings.theme = static_cast<ElementTheme>(ThemeCombo().SelectedIndex());

        // Save advanced settings
        settings.enableLogging = EnableLoggingToggle().IsOn();
        settings.bufferSize = static_cast<uint32_t>(BufferSizeNumber().Value() * 1024); // Convert KB to bytes

        settings.Save();
    }

    void PreferencesWindow::SaveButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        SaveSettings();
        Close();
    }

    void PreferencesWindow::ResetButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        auto& settings = ZipSpark::Settings::GetInstance();
        settings.ResetToDefaults();
        LoadSettings();
    }

    void PreferencesWindow::ThemeCombo_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const&)
    {
        ApplyTheme(ThemeCombo().SelectedIndex());
    }

    void PreferencesWindow::ApplyTheme(int themeIndex)
    {
        ElementTheme theme = static_cast<ElementTheme>(themeIndex);
        
        if (auto content = this->Content())
        {
            if (auto frameworkElement = content.try_as<FrameworkElement>())
            {
                frameworkElement.RequestedTheme(theme);
            }
        }
    }
}
