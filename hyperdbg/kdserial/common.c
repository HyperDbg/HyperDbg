#include <ntddk.h>

//
// We'll add these functions, so whenever HyperDbg's driver is unloaded
// DllUnload will be called to unload this dll from the memory.
// this way we can remove the HyperDbg after unloading as there is no
// other module remains loaded in the memory.
//

__declspec(dllexport) NTSTATUS
    DllInitialize(
        _In_ PUNICODE_STRING RegistryPath);

NTSTATUS
DllInitialize(
    _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    return STATUS_SUCCESS;
}

NTSTATUS
DllUnload(void)
{
    return STATUS_SUCCESS;
}
