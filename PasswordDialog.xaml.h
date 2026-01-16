#pragma once

#include "PasswordDialog.g.h"

namespace winrt::ZipSpark_New::implementation
{
    struct PasswordDialog : PasswordDialogT<PasswordDialog>
    {
        PasswordDialog();
        PasswordDialog(winrt::hstring archiveName);

        void PasswordInput_KeyDown(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& e);
        
        winrt::hstring GetPassword();
        bool ShouldRememberPassword();
        void ShowError(winrt::hstring message);

    private:
        winrt::hstring m_password;
    };
}

namespace winrt::ZipSpark_New::factory_implementation
{
    struct PasswordDialog : PasswordDialogT<PasswordDialog, implementation::PasswordDialog>
    {
    };
}
