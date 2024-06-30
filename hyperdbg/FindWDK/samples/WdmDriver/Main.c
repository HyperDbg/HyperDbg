#include <wdm.h>
#include "WdmLib.h"

DRIVER_UNLOAD driverUnload;
VOID driverUnload(_In_ PDRIVER_OBJECT driverObject)
{
    UNREFERENCED_PARAMETER(driverObject);

    DbgPrint("Driver unloaded\n");
}

DRIVER_INITIALIZE DriverEntry;
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT driverObject, _In_ PUNICODE_STRING registryPath)
{
    UNREFERENCED_PARAMETER(registryPath);

    DbgPrint("Driver loaded\n");
    DbgPrint("The answer is %wZ\n", getAnswer());

    driverObject->DriverUnload = driverUnload;
    return STATUS_SUCCESS;
}