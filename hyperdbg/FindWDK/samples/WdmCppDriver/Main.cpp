#include <wdm.h>
#include "WdmCppLib.h"

DRIVER_UNLOAD driverUnload;
VOID driverUnload(_In_ PDRIVER_OBJECT /*driverObject*/)
{
    DbgPrint("Driver unloaded\n");
}

extern "C" DRIVER_INITIALIZE DriverEntry;
extern "C" NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT driverObject, _In_ PUNICODE_STRING /*registryPath*/)
{
    DbgPrint("Driver loaded\n");
    DbgPrint("The answer is %wZ\n", getAnswer());

    driverObject->DriverUnload = driverUnload;
    return STATUS_SUCCESS;
}