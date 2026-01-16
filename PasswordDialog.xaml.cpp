#include "pch.h"
#include "PasswordDialog.xaml.h"
#if __has_include("PasswordDialog.g.cpp")
#include "PasswordDialog.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Input;

namespace winrt::ZipSpark_New::implementation
{
    PasswordDialog::PasswordDialog()
    {
        InitializeComponent();
    }

    PasswordDialog::PasswordDialog(winrt::hstring archiveName) : PasswordDialog()
    {
        ArchiveNameText().Text(archiveName);
    }

    void PasswordDialog::PasswordInput_KeyDown(IInspectable const&, KeyRoutedEventArgs const& e)
    {
        // Submit on Enter key
        if (e.Key() == Windows::System::VirtualKey::Enter)
        {
            m_password = PasswordInput().Password();
            Hide();
        }
    }

    winrt::hstring PasswordDialog::GetPassword()
    {
        return PasswordInput().Password();
    }

    bool PasswordDialog::ShouldRememberPassword()
    {
        return RememberPasswordCheckBox().IsChecked().GetBoolean();
    }

    void PasswordDialog::ShowError(winrt::hstring message)
    {
        ErrorInfoBar().Message(message);
        ErrorInfoBar().IsOpen(true);
        PasswordInput().Password(L"");
        PasswordInput().Focus(FocusState::Programmatic);
    }
}
