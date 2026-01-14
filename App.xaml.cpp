#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include "Utils/Logger.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::ZipSpark_New::implementation
{
    /// <summary>
    /// Initializes the singleton application object.  This is the first line of authored code
    /// executed, and as such is the logical equivalent of main() or WinMain().
    /// </summary>
    App::App()
    {
        // Xaml objects should not call InitializeComponent during construction.
        // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        
        // Initialize logger
        LOG_INFO(L"ZipSpark application starting");

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
        UnhandledException([](IInspectable const&, UnhandledExceptionEventArgs const& e)
        {
            if (IsDebuggerPresent())
            {
                auto errorMessage = e.Message();
                __debugbreak();
            }
        });
#endif
    }

    /// <summary>
    /// Invoked when the application is launched.
    /// </summary>
    /// <param name="e">Details about the launch request and process.</param>
    void App::OnLaunched([[maybe_unused]] LaunchActivatedEventArgs const& e)
    {
        // Parse command-line arguments for file association
        winrt::hstring archivePath;
        
        try
        {
            // Get command-line arguments
            int argc = 0;
            LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
            
            if (argv && argc > 1)
            {
                // First argument after exe is the archive path
                archivePath = argv[1];
                LOG_INFO(L"Launched with archive: " + std::wstring(archivePath.c_str()));
            }
            
            if (argv)
                LocalFree(argv);
        }
        catch (...)
        {
            // Ignore command-line parsing errors
        }
        
        // Create main window with archive path (if provided)
        if (!archivePath.empty())
        {
            window = make<MainWindow>(archivePath);
        }
        else
        {
            window = make<MainWindow>();
        }
        
        window.Activate();
    }
}
