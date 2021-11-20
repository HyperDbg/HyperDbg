#include <ntddk.h>

//
// We'll add these functions, so whenever HyperDbg's driver is unloaded
// DllUnload will be called to unload the kdserial.dll from the memory
// this way we can remove the HyperDbg after unloading as there is no
// other module remains loaded in the memory
//

NTSTATUS
DllInitialize(
    _In_ PUNICODE_STRING RegistryPath)
{
    return STATUS_SUCCESS;
}

NTSTATUS
DllUnload(void)
{
    return STATUS_SUCCESS;
}
