#include "ExplorerCommand.h"
#include <shlwapi.h>
#include <sstream>

#pragma comment(lib, "shlwapi.lib")

extern HINSTANCE g_hInst;

ExplorerCommand::ExplorerCommand()
{
}

IFACEMETHODIMP ExplorerCommand::GetTitle(IShellItemArray* psiItemArray, LPWSTR* ppszName)
{
    // Main menu item title
    return SHStrDupW(L"ZipSpark", ppszName);
}

IFACEMETHODIMP ExplorerCommand::GetIcon(IShellItemArray* psiItemArray, LPWSTR* ppszIcon)
{
    // Path to the DLL itself, or the main exe if in the same folder
    WCHAR szModule[MAX_PATH];
    GetModuleFileNameW(g_hInst, szModule, ARRAYSIZE(szModule));
    return SHStrDupW(szModule, ppszIcon);
}

IFACEMETHODIMP ExplorerCommand::GetToolTip(IShellItemArray* psiItemArray, LPWSTR* ppszInfotip)
{
    return SHStrDupW(L"ZipSpark Archive Options", ppszInfotip);
}

IFACEMETHODIMP ExplorerCommand::GetCanonicalName(GUID* pguidCommandName)
{
    *pguidCommandName = __uuidof(ExplorerCommand);
    return S_OK;
}

IFACEMETHODIMP ExplorerCommand::GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState)
{
    *pCmdState = ECS_ENABLED;
    return S_OK;
}

IFACEMETHODIMP ExplorerCommand::Invoke(IShellItemArray* psiItemArray, IBindCtx* pbc)
{
    return S_OK; // Main item doesn't do anything, subcommands do
}

IFACEMETHODIMP ExplorerCommand::GetFlags(EXPCMDFLAGS* pFlags)
{
    *pFlags = ECF_HASSUBCOMMANDS;
    return S_OK;
}

IFACEMETHODIMP ExplorerCommand::EnumSubCommands(IEnumExplorerCommand** ppEnum)
{
    std::vector<ComPtr<IExplorerCommand>> commands;
    commands.push_back(Make<SubCommand>(L"Add to archive...", L""));
    commands.push_back(Make<SubCommand>(L"Add to .zip", L".zip"));
    commands.push_back(Make<SubCommand>(L"Add to .7z", L".7z"));

    auto enumCmd = Make<EnumExplorerCommand>(commands);
    return enumCmd.CopyTo(ppEnum);
}

IFACEMETHODIMP ExplorerCommand::SetSite(IUnknown* pUnkSite)
{
    m_site = pUnkSite;
    return S_OK;
}

IFACEMETHODIMP ExplorerCommand::GetSite(REFIID riid, void** ppvSite)
{
    return m_site.CopyTo(riid, ppvSite);
}

// ----------------------------------------------------------------------
// EnumExplorerCommand
// ----------------------------------------------------------------------

EnumExplorerCommand::EnumExplorerCommand(std::vector<ComPtr<IExplorerCommand>> commands)
    : m_commands(commands)
{
}

IFACEMETHODIMP EnumExplorerCommand::Next(ULONG celt, IExplorerCommand** pUICommand, ULONG* pceltFetched)
{
    ULONG fetched = 0;
    for (ULONG i = 0; i < celt && m_current < m_commands.size(); i++)
    {
        m_commands[m_current++].CopyTo(&pUICommand[i]);
        fetched++;
    }
    
    if (pceltFetched) *pceltFetched = fetched;
    return (fetched == celt) ? S_OK : S_FALSE;
}

IFACEMETHODIMP EnumExplorerCommand::Skip(ULONG celt)
{
    m_current += celt;
    return (m_current <= m_commands.size()) ? S_OK : S_FALSE;
}

IFACEMETHODIMP EnumExplorerCommand::Reset()
{
    m_current = 0;
    return S_OK;
}

IFACEMETHODIMP EnumExplorerCommand::Clone(IEnumExplorerCommand** ppenum)
{
    *ppenum = nullptr;
    return E_NOTIMPL;
}

// ----------------------------------------------------------------------
// SubCommand
// ----------------------------------------------------------------------

SubCommand::SubCommand(const std::wstring& title, const std::wstring& formatExtension)
    : m_title(title), m_formatExtension(formatExtension)
{
}

IFACEMETHODIMP SubCommand::GetTitle(IShellItemArray* psiItemArray, LPWSTR* ppszName)
{
    return SHStrDupW(m_title.c_str(), ppszName);
}

IFACEMETHODIMP SubCommand::GetIcon(IShellItemArray* psiItemArray, LPWSTR* ppszIcon)
{
    // Use standard ZIP folder icon or App icon
    // For now, reuse default
    return S_OK;
}

IFACEMETHODIMP SubCommand::GetToolTip(IShellItemArray* psiItemArray, LPWSTR* ppszInfotip)
{
    return SHStrDupW(L"Create Archive", ppszInfotip);
}

IFACEMETHODIMP SubCommand::GetCanonicalName(GUID* pguidCommandName)
{
    *pguidCommandName = GUID_NULL;
    return S_OK;
}

IFACEMETHODIMP SubCommand::GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState)
{
    *pCmdState = ECS_ENABLED;
    return S_OK;
}

IFACEMETHODIMP SubCommand::Invoke(IShellItemArray* psiItemArray, IBindCtx* pbc)
{
    // 1. Collect file paths
    DWORD count = 0;
    psiItemArray->GetCount(&count);
    
    if (count == 0) return S_OK;

    std::wstring commandLineArgs = L"zipspark:create";
    if (!m_formatExtension.empty())
    {
        commandLineArgs += L"?format=" + m_formatExtension;
    }
    else
    {
        commandLineArgs += L"?format=dialog";
    }

    // Since command line length is limited (32k chars), we might need another approach for MANY files.
    // For this version, we will write paths to a temp file and pass the temp file path.
    // This is safer and more robust.
    
    WCHAR tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    
    WCHAR tempFileName[MAX_PATH];
    GetTempFileNameW(tempPath, L"ZSP", 0, tempFileName);
    
    std::wstring fileListPath = tempFileName;
    FILE* f = nullptr;
    _wfopen_s(&f, fileListPath.c_str(), L"w, ccs=UTF-8");
    if (f)
    {
        for (DWORD i = 0; i < count; i++)
        {
            IShellItem* item;
            if (SUCCEEDED(psiItemArray->GetItemAt(i, &item)))
            {
                LPWSTR path;
                if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)))
                {
                    fwprintf(f, L"%s\n", path);
                    CoTaskMemFree(path);
                }
                item->Release();
            }
        }
        fclose(f);
    }
    
    commandLineArgs += L"&files=" + fileListPath;

    // 2. Find and Launch ZipSpark.exe
    // We assume ZipSpark.exe is in the same package (execution alias or direct path)
    // Or we rely on Protocol Activation "zipspark:"
    
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_FLAG_NO_UI;
    sei.lpVerb = L"open";
    sei.lpFile = commandLineArgs.c_str(); // Protocol activation
    sei.nShow = SW_SHOWNORMAL;
    
    ShellExecuteExW(&sei);
    
    return S_OK;
}

IFACEMETHODIMP SubCommand::GetFlags(EXPCMDFLAGS* pFlags)
{
    *pFlags = ECF_DEFAULT;
    return S_OK;
}

IFACEMETHODIMP SubCommand::EnumSubCommands(IEnumExplorerCommand** ppEnum)
{
    return E_NOTIMPL; // No sub-sub commands
}
