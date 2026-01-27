#include "ExplorerCommand.h"
#include <shlwapi.h>
#include <sstream>
#include <new> // for std::bad_alloc

#pragma comment(lib, "shlwapi.lib")

extern HINSTANCE g_hInst;

// Helper macro for exception handling
#define CATCH_RETURN() \
    catch (const std::bad_alloc&) { return E_OUTOFMEMORY; } \
    catch (...) { return E_FAIL; }

ExplorerCommand::ExplorerCommand()
{
}

IFACEMETHODIMP ExplorerCommand::GetTitle(IShellItemArray* psiItemArray, LPWSTR* ppszName)
{
    try
    {
        // Main menu item title
        return SHStrDupW(L"ZipSpark", ppszName);
    }
    CATCH_RETURN();
}

IFACEMETHODIMP ExplorerCommand::GetIcon(IShellItemArray* psiItemArray, LPWSTR* ppszIcon)
{
    try
    {
        // Path to the DLL itself, or the main exe if in the same folder
        WCHAR szModule[MAX_PATH];
        if (GetModuleFileNameW(g_hInst, szModule, ARRAYSIZE(szModule)) == 0)
        {
             return E_FAIL;
        }
        return SHStrDupW(szModule, ppszIcon);
    }
    CATCH_RETURN();
}

IFACEMETHODIMP ExplorerCommand::GetToolTip(IShellItemArray* psiItemArray, LPWSTR* ppszInfotip)
{
    try
    {
        return SHStrDupW(L"ZipSpark Archive Options", ppszInfotip);
    }
    CATCH_RETURN();
}

IFACEMETHODIMP ExplorerCommand::GetCanonicalName(GUID* pguidCommandName)
{
    if (!pguidCommandName) return E_POINTER;
    try
    {
        *pguidCommandName = __uuidof(ExplorerCommand);
        return S_OK;
    }
    CATCH_RETURN();
}

IFACEMETHODIMP ExplorerCommand::GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState)
{
    if (!pCmdState) return E_POINTER;
    try
    {
        *pCmdState = ECS_ENABLED;
        return S_OK;
    }
    CATCH_RETURN();
}

IFACEMETHODIMP ExplorerCommand::Invoke(IShellItemArray* psiItemArray, IBindCtx* pbc)
{
    return S_OK; // Main item doesn't do anything, subcommands do
}

IFACEMETHODIMP ExplorerCommand::GetFlags(EXPCMDFLAGS* pFlags)
{
    if (!pFlags) return E_POINTER;
    try 
    {
        *pFlags = ECF_HASSUBCOMMANDS;
        return S_OK;
    }
    CATCH_RETURN();
}

IFACEMETHODIMP ExplorerCommand::EnumSubCommands(IEnumExplorerCommand** ppEnum)
{
    if (!ppEnum) return E_POINTER;
    *ppEnum = nullptr;
    try
    {
        std::vector<ComPtr<IExplorerCommand>> commands;
        commands.push_back(Make<SubCommand>(L"Add to archive...", L""));
        commands.push_back(Make<SubCommand>(L"Add to .zip", L".zip"));
        commands.push_back(Make<SubCommand>(L"Add to .7z", L".7z"));

        auto enumCmd = Make<EnumExplorerCommand>(commands);
        return enumCmd.CopyTo(ppEnum);
    }
    CATCH_RETURN();
}

IFACEMETHODIMP ExplorerCommand::SetSite(IUnknown* pUnkSite)
{
    try
    {
        m_site = pUnkSite;
        return S_OK;
    }
    CATCH_RETURN();
}

IFACEMETHODIMP ExplorerCommand::GetSite(REFIID riid, void** ppvSite)
{
    if (!ppvSite) return E_POINTER;
    *ppvSite = nullptr;
    try
    {
        if (!m_site) return E_NOINTERFACE;
        return m_site.CopyTo(riid, ppvSite);
    }
    CATCH_RETURN();
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
    if (!pUICommand) return E_POINTER;
    try
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
    CATCH_RETURN();
}

IFACEMETHODIMP EnumExplorerCommand::Skip(ULONG celt)
{
    try
    {
        m_current += celt;
        return (m_current <= m_commands.size()) ? S_OK : S_FALSE;
    }
    CATCH_RETURN();
}

IFACEMETHODIMP EnumExplorerCommand::Reset()
{
    m_current = 0;
    return S_OK;
}

IFACEMETHODIMP EnumExplorerCommand::Clone(IEnumExplorerCommand** ppenum)
{
    if (!ppenum) return E_POINTER;
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
    try
    {
        return SHStrDupW(m_title.c_str(), ppszName);
    }
    CATCH_RETURN();
}

IFACEMETHODIMP SubCommand::GetIcon(IShellItemArray* psiItemArray, LPWSTR* ppszIcon)
{
    if (!ppszIcon) return E_POINTER;
    *ppszIcon = nullptr; // No icon for subcommands, but must initialize output
    return E_NOTIMPL; // Return E_NOTIMPL to indicate no icon provided
}

IFACEMETHODIMP SubCommand::GetToolTip(IShellItemArray* psiItemArray, LPWSTR* ppszInfotip)
{
    try
    {
        return SHStrDupW(L"Create Archive", ppszInfotip);
    }
    CATCH_RETURN();
}

IFACEMETHODIMP SubCommand::GetCanonicalName(GUID* pguidCommandName)
{
    if (!pguidCommandName) return E_POINTER;
    try
    {
        *pguidCommandName = GUID_NULL;
        return S_OK;
    }
    CATCH_RETURN();
}

IFACEMETHODIMP SubCommand::GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState)
{
    if (!pCmdState) return E_POINTER;
    try
    {
        *pCmdState = ECS_ENABLED;
        return S_OK;
    }
    CATCH_RETURN();
}

IFACEMETHODIMP SubCommand::Invoke(IShellItemArray* psiItemArray, IBindCtx* pbc)
{
    try
    {
        // 1. Collect file paths
        DWORD count = 0;
        if (FAILED(psiItemArray->GetCount(&count))) return E_FAIL;
        
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
        if (GetTempPathW(MAX_PATH, tempPath) == 0) return E_FAIL;
        
        WCHAR tempFileName[MAX_PATH];
        if (GetTempFileNameW(tempPath, L"ZSP", 0, tempFileName) == 0) return E_FAIL;
        
        std::wstring fileListPath = tempFileName;
        FILE* f = nullptr;
        if (_wfopen_s(&f, fileListPath.c_str(), L"w, ccs=UTF-8") != 0) return E_FAIL;
        
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
                        if (fwprintf(f, L"%s\n", path) < 0) { /* Handle error? */ }
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
        
        if (!ShellExecuteExW(&sei))
        {
            // If shell execute fails
             return E_FAIL;
        }
        
        return S_OK;
    }
    CATCH_RETURN();
}

IFACEMETHODIMP SubCommand::GetFlags(EXPCMDFLAGS* pFlags)
{
    if (!pFlags) return E_POINTER;
    try
    {
        *pFlags = ECF_DEFAULT;
        return S_OK;
    }
    CATCH_RETURN();
}

IFACEMETHODIMP SubCommand::EnumSubCommands(IEnumExplorerCommand** ppEnum)
{
    if (!ppEnum) return E_POINTER;
    *ppEnum = nullptr;
    return E_NOTIMPL; // No sub-sub commands
}
