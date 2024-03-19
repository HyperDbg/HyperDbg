/**
 * @file Driver.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The project entry (RM)
 * @details
 *
 * @version 0.2
 * @date 2023-01-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Main Driver Entry in the case of driver load
 *
 * @param DriverObject
 * @param RegistryPath
 * @return NTSTATUS
 */
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT  DriverObject,
    PUNICODE_STRING RegistryPath)
{
    NTSTATUS       Ntstatus      = STATUS_SUCCESS;
    UINT64         Index         = 0;
    PDEVICE_OBJECT DeviceObject  = NULL;
    UNICODE_STRING DriverName    = RTL_CONSTANT_STRING(L"\\Device\\HyperDbgReversingMachineDevice");
    UNICODE_STRING DosDeviceName = RTL_CONSTANT_STRING(L"\\DosDevices\\HyperDbgReversingMachineDevice");

    UNREFERENCED_PARAMETER(RegistryPath);
    UNREFERENCED_PARAMETER(DriverObject);

    //
    // Opt-in to using non-executable pool memory on Windows 8 and later.
    // https://msdn.microsoft.com/en-us/library/windows/hardware/hh920402(v=vs.85).aspx
    //
    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    //
    // Creating the device for interaction with user-mode
    //
    Ntstatus = IoCreateDevice(DriverObject,
                              0,
                              &DriverName,
                              FILE_DEVICE_UNKNOWN,
                              FILE_DEVICE_SECURE_OPEN,
                              FALSE,
                              &DeviceObject);

    if (Ntstatus == STATUS_SUCCESS)
    {
        for (Index = 0; Index < IRP_MJ_MAXIMUM_FUNCTION; Index++)
            DriverObject->MajorFunction[Index] = DrvUnsupported;

        //
        // We cannot use logging mechanism of HyperDbg as it's not initialized yet
        //
        DbgPrint("Setting device major functions");

        DriverObject->MajorFunction[IRP_MJ_CLOSE]          = DrvClose;
        DriverObject->MajorFunction[IRP_MJ_CREATE]         = DrvCreate;
        DriverObject->MajorFunction[IRP_MJ_READ]           = DrvRead;
        DriverObject->MajorFunction[IRP_MJ_WRITE]          = DrvWrite;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DrvDispatchIoControl;

        DriverObject->DriverUnload = DrvUnload;
        IoCreateSymbolicLink(&DosDeviceName, &DriverName);
    }

    //
    // Establish user-buffer access method.
    //
    if (DeviceObject != NULL)
    {
        DeviceObject->Flags |= DO_BUFFERED_IO;
    }

    //
    // We cannot use logging mechanism of HyperDbg as it's not initialized yet
    //
    DbgPrint("HyperDbg's device and major functions are loaded");

    ASSERT(NT_SUCCESS(Ntstatus));
    return Ntstatus;
}

/**
 * @brief Run in the case of driver unload to unregister the devices
 *
 * @param DriverObject
 * @return VOID
 */
VOID
DrvUnload(PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING DosDeviceName;

    RtlInitUnicodeString(&DosDeviceName, L"\\DosDevices\\HyperDbgReversingMachineDevice");
    IoDeleteSymbolicLink(&DosDeviceName);
    IoDeleteDevice(DriverObject->DeviceObject);

    //
    // Unloading VMM and Debugger
    //
    LoaderUninitializeLogTracer();
}

/**
 * @brief IRP_MJ_CREATE Function handler
 *
 * @param DeviceObject
 * @param Irp
 * @return NTSTATUS
 */
NTSTATUS
DrvCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // Check for privilege
    //
    // Check for the correct security access.
    // The caller must have the SeDebugPrivilege.
    //

    LUID DebugPrivilege = {SE_DEBUG_PRIVILEGE, 0};

    if (!SeSinglePrivilegeCheck(DebugPrivilege, Irp->RequestorMode))
    {
        Irp->IoStatus.Status      = STATUS_ACCESS_DENIED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_ACCESS_DENIED;
    }

    //
    // Check to allow just one handle to the driver
    // means that only one application can get the handle
    // and new application won't allowed to create a new
    // handle unless the IRP_MJ_CLOSE called.
    //
    if (g_HandleInUse)
    {
        //
        // A driver got the handle before
        //
        Irp->IoStatus.Status      = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_SUCCESS;
    }

    //
    // Initialize the vmm and the reversing machine
    //
    if (LoaderInitVmmAndReversingMachine())
    {
        Irp->IoStatus.Status      = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_SUCCESS;
    }
    else
    {
        //
        // There was a problem, so not loaded
        //
        Irp->IoStatus.Status      = STATUS_UNSUCCESSFUL;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_UNSUCCESSFUL;
    }
}

/**
 * @brief IRP_MJ_READ Function handler
 *
 * @param DeviceObject
 * @param Irp
 * @return NTSTATUS
 */
NTSTATUS
DrvRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // Not used
    //
    DbgPrint("This function is not used");

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/**
 * @brief IRP_MJ_WRITE Function handler
 *
 * @param DeviceObject
 * @param Irp
 * @return NTSTATUS
 */
NTSTATUS
DrvWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // Not used
    //
    DbgPrint("This function is not used");

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/**
 * @brief IRP_MJ_CLOSE Function handler
 *
 * @param DeviceObject
 * @param Irp
 * @return NTSTATUS
 */
NTSTATUS
DrvClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // If the close is called means that all of the IOCTLs
    // are not in a pending state so we can safely allow
    // a new handle creation for future calls to the driver
    //
    g_HandleInUse = FALSE;

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/**
 * @brief Unsupported message for all other IRP_MJ_* handlers
 *
 * @param DeviceObject
 * @param Irp
 * @return NTSTATUS
 */
NTSTATUS
DrvUnsupported(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // Not supported
    //
    DbgPrint("This function is not supported");

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}
