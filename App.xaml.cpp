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
        try
        {
            LOG_INFO(L"=== ZipSpark Application Starting ===");
            LOG_INFO(L"App constructor called");
            
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
            UnhandledException([](IInspectable const&, UnhandledExceptionEventArgs const& e)
            {
                auto errorMessage = e.Message();
                LOG_ERROR(L"Unhandled exception in App: " + std::wstring(errorMessage));
                
                if (IsDebuggerPresent())
                {
                    __debugbreak();
                }
            });
#else
            UnhandledException([](IInspectable const&, UnhandledExceptionEventArgs const& e)
            {
                auto errorMessage = e.Message();
                LOG_ERROR(L"Unhandled exception in App (Release): " + std::wstring(errorMessage));
            });
#endif
            
            LOG_INFO(L"App constructor completed successfully");
        }
        catch (const winrt::hresult_error& ex)
        {
            std::wstring message = ex.message().c_str();
            LOG_ERROR(L"WinRT error in App constructor: " + message + L" (HRESULT: " + std::to_wstring(ex.code()) + L")");
            throw;
        }
        catch (const std::exception& ex)
        {
            std::string what = ex.what();
            std::wstring wwhat(what.begin(), what.end());
            LOG_ERROR(L"Exception in App constructor: " + wwhat);
            throw;
        }
        catch (...)
        {
            LOG_ERROR(L"Unknown exception in App constructor");
            throw;
        }
    }

    /// <summary>
    /// Invoked when the application is launched.
    /// </summary>
    /// <param name="e">Details about the launch request and process.</param>
    void App::OnLaunched([[maybe_unused]] LaunchActivatedEventArgs const& e)
    {
        try
        {
            LOG_INFO(L"OnLaunched called");
            
            // Parse command-line arguments for file association and context menu verbs
            winrt::hstring archivePath;
            bool extractHere = false;
            bool extractTo = false;
            
            // Creation mode variables
            bool isCreationMode = false;
            std::wstring creationFormat;
            std::vector<std::wstring> creationFiles;
            
            try
            {
                // Get command-line arguments
                int argc = 0;
                LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
                
                LOG_INFO(L"Command-line argument count: " + std::to_wstring(argc));
                
                if (argv)
                {
                    for (int i = 1; i < argc; i++)
                    {
                        std::wstring arg = argv[i];
                        LOG_INFO(L"Command-line arg[" + std::to_wstring(i) + L"]: " + arg);
                        
                        // Protocol Activation: zipspark:create?format=...&files=...
                        if (arg.find(L"zipspark:") == 0)
                        {
                            LOG_INFO(L"Protocol activation detected");
                            
                            // Check action
                            if (arg.find(L"create") != std::wstring::npos)
                            {
                                isCreationMode = true;
                                
                                // Parse format
                                size_t formatPos = arg.find(L"format=");
                                if (formatPos != std::wstring::npos)
                                {
                                    size_t ampPos = arg.find(L"&", formatPos);
                                    if (ampPos == std::wstring::npos) ampPos = arg.length();
                                    creationFormat = arg.substr(formatPos + 7, ampPos - (formatPos + 7));
                                }
                                
                                // Parse files temp path
                                size_t filesPos = arg.find(L"files=");
                                if (filesPos != std::wstring::npos)
                                {
                                    size_t ampPos = arg.find(L"&", filesPos);
                                    if (ampPos == std::wstring::npos) ampPos = arg.length();
                                    std::wstring tempFileListPath = arg.substr(filesPos + 6, ampPos - (filesPos + 6));
                                    
                                    LOG_INFO(L"Reading file list from: " + tempFileListPath);
                                    
                                    // Read files from temp file
                                    std::wifstream file(tempFileListPath.c_str());
                                    if (file.is_open())
                                    {
                                        // Set locale to handle UTF-8 if possible, but wifstream defaults usually work for simple paths
                                        // Ideally we use _wfopen and generic helper, but simple approach for now:
                                        std::wstring line;
                                        while (std::getline(file, line))
                                        {
                                            if (!line.empty())
                                            {
                                                 creationFiles.push_back(line);
                                            }
                                        }
                                        file.close();
                                        
                                        // Optional: Delete temp file
                                        // DeleteFileW(tempFileListPath.c_str());
                                    }
                                }
                            }
                        }
                        else if (arg == L"--extract-here")
                        {
                            extractHere = true;
                            LOG_INFO(L"Extract Here mode enabled");
                        }
                        else if (arg == L"--extract-to")
                        {
                            extractTo = true;
                            LOG_INFO(L"Extract To mode enabled");
                        }
                        else if (arg.find(L".zip") != std::wstring::npos || 
                                 arg.find(L".7z") != std::wstring::npos ||
                                 arg.find(L".rar") != std::wstring::npos)
                        {
                            archivePath = arg;
                            LOG_INFO(L"Archive path from command-line: " + std::wstring(archivePath));
                        }
                    }
                    
                    LocalFree(argv);
                }
            }
            catch (const std::exception& ex)
            {
                std::string what = ex.what();
                std::wstring wwhat(what.begin(), what.end());
                LOG_ERROR(L"Exception parsing command-line: " + wwhat);
            }
            catch (...)
            {
                LOG_ERROR(L"Unknown exception parsing command-line");
            }
            
            // Create main window
            LOG_INFO(L"Creating main window...");
            
            if (isCreationMode)
            {
                 LOG_INFO(L"Initializing Creation Mode with " + std::to_wstring(creationFiles.size()) + L" files");
                 window = make<MainWindow>();
            }
            else if (!archivePath.empty())
            {
                LOG_INFO(L"Creating MainWindow with archive path: " + std::wstring(archivePath));
                window = make<MainWindow>(archivePath);
            }
            else
            {
                LOG_INFO(L"Creating MainWindow without archive path");
                window = make<MainWindow>();
            }
            
            LOG_INFO(L"Activating main window...");
            window.Activate();
            
            // Post-activation setup for creation mode
            if (isCreationMode)
            {
                try 
                {
                    auto mainWindow = window.as<implementation::MainWindow>();
                    if (mainWindow)
                    {
                        mainWindow->ShowCreationUI(creationFiles, creationFormat);
                    }
                }
                catch(...)
                {
                    LOG_ERROR(L"Failed to initialize creation UI");
                }
            }
            LOG_INFO(L"Main window activated successfully");
            LOG_INFO(L"Log file location: " + ZipSpark::Logger::GetInstance().GetLogFilePath());
        }
        catch (const winrt::hresult_error& ex)
        {
            std::wstring message = ex.message().c_str();
            LOG_ERROR(L"WinRT error in OnLaunched: " + message + L" (HRESULT: " + std::to_wstring(ex.code()) + L")");
        }
        catch (const std::exception& ex)
        {
            std::string what = ex.what();
            std::wstring wwhat(what.begin(), what.end());
            LOG_ERROR(L"Exception in OnLaunched: " + wwhat);
        }
        catch (...)
        {
            LOG_ERROR(L"Unknown exception in OnLaunched");
        }
    }
}
