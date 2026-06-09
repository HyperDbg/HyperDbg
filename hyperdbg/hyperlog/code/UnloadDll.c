/**
 * @file UnloadDll.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Unloading DLL in the target Windows
 *
 * @version 0.4
 * @date 2023-07-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// We'll add these functions, so whenever HyperDbg's driver is unloaded
// DllUnload will be called to unload this dll from the memory.
// this way we can remove the HyperDbg after unloading as there is no
// other module remains loaded in the memory.
//

NTSTATUS
DllInitialize(
    _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    return STATUS_SUCCESS;
}

NTSTATUS
DllUnload(VOID)
{
    return STATUS_SUCCESS;
}
