
#include <ntdef.h>
#include <wdm.h>

/**
 * @brief Main Driver Entry in the case of driver load
 *
 * @param DriverObject
 * @param RegistryPath
 * @return NTSTATUS
 */
NTSTATUS
FxDriverEntry(
    PDRIVER_OBJECT  DriverObject,
    PUNICODE_STRING RegistryPath)
{
    NTSTATUS Ntstatus = STATUS_SUCCESS;
    return Ntstatus;
}
