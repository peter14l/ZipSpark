#include <wrl/module.h>
#include <wrl/implements.h>
#include "ExplorerCommand.h"

using namespace Microsoft::WRL;

// Define the module for the DLL
CoCreatableClass(ExplorerCommand);

HINSTANCE g_hInst = nullptr;

STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID* ppv)
{
    return Module<InProc>::GetModule().GetClassObject(rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow()
{
    return Module<InProc>::GetModule().Terminate() ? S_OK : S_FALSE;
}

BOOL WINAPI DllMain(_In_ HINSTANCE hInstance, _In_ DWORD reason, _In_opt_ LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_hInst = hInstance;
        DisableThreadLibraryCalls(hInstance);
        Module<InProc>::GetModule().Create();
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        Module<InProc>::GetModule().Terminate();
    }
    return TRUE;
}
