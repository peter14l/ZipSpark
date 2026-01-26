#pragma once
#include <windows.h>
#include <wrl.h>
#include <wrl/implements.h>
#include <wrl/module.h>
#include <shobjidl_core.h>
#include <string>
#include <vector>

using namespace Microsoft::WRL;

// CLSID for the Shell Extension
// {E4C8ECDF-C319-4842-8349-166341235149}
class __declspec(uuid("E4C8ECDF-C319-4842-8349-166341235149")) ExplorerCommand : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IExplorerCommand, IObjectWithSite>
{
public:
    ExplorerCommand();

    // IExplorerCommand
    IFACEMETHODIMP GetTitle(IShellItemArray* psiItemArray, LPWSTR* ppszName);
    IFACEMETHODIMP GetIcon(IShellItemArray* psiItemArray, LPWSTR* ppszIcon);
    IFACEMETHODIMP GetToolTip(IShellItemArray* psiItemArray, LPWSTR* ppszInfotip);
    IFACEMETHODIMP GetCanonicalName(GUID* pguidCommandName);
    IFACEMETHODIMP GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState);
    IFACEMETHODIMP Invoke(IShellItemArray* psiItemArray, IBindCtx* pbc);
    IFACEMETHODIMP GetFlags(EXPCMDFLAGS* pFlags);
    IFACEMETHODIMP EnumSubCommands(IEnumExplorerCommand** ppEnum);

    // IObjectWithSite
    IFACEMETHODIMP SetSite(IUnknown* pUnkSite);
    IFACEMETHODIMP GetSite(REFIID riid, void** ppvSite);

private:
    ComPtr<IUnknown> m_site;
};

class EnumExplorerCommand : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IEnumExplorerCommand>
{
public:
    EnumExplorerCommand(std::vector<ComPtr<IExplorerCommand>> commands);

    // IEnumExplorerCommand
    IFACEMETHODIMP Next(ULONG celt, IExplorerCommand** pUICommand, ULONG* pceltFetched);
    IFACEMETHODIMP Skip(ULONG celt);
    IFACEMETHODIMP Reset();
    IFACEMETHODIMP Clone(IEnumExplorerCommand** ppenum);

private:
    std::vector<ComPtr<IExplorerCommand>> m_commands;
    ULONG m_current = 0;
};

class SubCommand : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IExplorerCommand>
{
public:
    SubCommand(const std::wstring& title, const std::wstring& formatExtension);

    // IExplorerCommand methods (Simplified for subcommands)
    IFACEMETHODIMP GetTitle(IShellItemArray* psiItemArray, LPWSTR* ppszName);
    IFACEMETHODIMP GetIcon(IShellItemArray* psiItemArray, LPWSTR* ppszIcon);
    IFACEMETHODIMP GetToolTip(IShellItemArray* psiItemArray, LPWSTR* ppszInfotip);
    IFACEMETHODIMP GetCanonicalName(GUID* pguidCommandName);
    IFACEMETHODIMP GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState);
    IFACEMETHODIMP Invoke(IShellItemArray* psiItemArray, IBindCtx* pbc);
    IFACEMETHODIMP GetFlags(EXPCMDFLAGS* pFlags);
    IFACEMETHODIMP EnumSubCommands(IEnumExplorerCommand** ppEnum);

private:
    std::wstring m_title;
    std::wstring m_formatExtension; // .zip, .7z, or empty for "Add to archive..."
};
