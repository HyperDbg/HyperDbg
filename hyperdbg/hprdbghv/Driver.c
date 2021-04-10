/**
 * @file Driver.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief The project entry 
 * @details This file contains major functions and all the interactions
 * with usermode codes are managed from here.
 * e.g debugger commands and extension commands
 * @version 0.1
 * @date 2020-04-10
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"
#include "Driver.tmh"

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
    NTSTATUS       Ntstatus       = STATUS_SUCCESS;
    UINT64         Index          = 0;
    UINT32         ProcessorCount = 0;
    PDEVICE_OBJECT DeviceObject   = NULL;
    UNICODE_STRING DriverName     = RTL_CONSTANT_STRING(L"\\Device\\HyperdbgHypervisorDevice");
    UNICODE_STRING DosDeviceName  = RTL_CONSTANT_STRING(L"\\DosDevices\\HyperdbgHypervisorDevice");

    UNREFERENCED_PARAMETER(RegistryPath);
    UNREFERENCED_PARAMETER(DriverObject);

    //
    // Initialize WPP Tracing
    //

    WPP_INIT_TRACING(DriverObject, RegistryPath);

#if !UseDbgPrintInsteadOfUsermodeMessageTracking
    if (!LogInitialize())
    {
        DbgPrint("[*] Log buffer is not initialized !\n");
        DbgBreakPoint();
    }
#endif

    //
    // Opt-in to using non-executable pool memory on Windows 8 and later.
    // https://msdn.microsoft.com/en-us/library/windows/hardware/hh920402(v=vs.85).aspx
    //

    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    //
    // we allocate virtual machine here because
    // we want to use its state (vmx-root or vmx non-root) in logs
    //

    ProcessorCount = KeQueryActiveProcessorCount(0);

    //
    // Allocate global variable to hold Guest(s) state
    //

    g_GuestState = ExAllocatePoolWithTag(NonPagedPool, sizeof(VIRTUAL_MACHINE_STATE) * ProcessorCount, POOLTAG);
    if (!g_GuestState)
    {
        //
        // we use DbgPrint as the vmx-root or non-root is not initialized
        //

        DbgPrint("Insufficient memory\n");
        DbgBreakPoint();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Zero the memory
    //
    RtlZeroMemory(g_GuestState, sizeof(VIRTUAL_MACHINE_STATE) * ProcessorCount);

    LogDebugInfo("Hyperdbg is Loaded :)");

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

        LogDebugInfo("Setting device major functions");
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
    DeviceObject->Flags |= DO_BUFFERED_IO;

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

    RtlInitUnicodeString(&DosDeviceName, L"\\DosDevices\\HyperdbgHypervisorDevice");
    IoDeleteSymbolicLink(&DosDeviceName);
    IoDeleteDevice(DriverObject->DeviceObject);

    DbgPrint("Hyperdbg's hypervisor driver unloaded\n");

#if !UseDbgPrintInsteadOfUsermodeMessageTracking

    //
    // Uinitialize log buffer
    //
    DbgPrint("Uinitializing logs\n");
    LogUnInitialize();
#endif

    //
    // Free g_Events
    //
    ExFreePoolWithTag(g_Events, POOLTAG);

    //
    // Free g_GuestState
    //
    ExFreePoolWithTag(g_GuestState, POOLTAG);

    //
    // Stop the tracing
    //
    WPP_CLEANUP(DriverObject);
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
    int ProcessorCount;

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
    // Allow to server IOCTL
    //
    g_AllowIOCTLFromUsermode = TRUE;

    LogDebugInfo("Hyperdbg's hypervisor Started...");
    //
    // We have to zero the g_GuestState again as we want to support multiple initialization by CreateFile
    //
    ProcessorCount = KeQueryActiveProcessorCount(0);

    //
    // Zero the memory
    //
    RtlZeroMemory(g_GuestState, sizeof(VIRTUAL_MACHINE_STATE) * ProcessorCount);

    //
    // Initialize memory mapper
    //
    MemoryMapperInitialize();

    //
    // Check if processor supports TSX (RTM)
    //
    g_RtmSupport = CheckCpuSupportRtm();

    //
    // Initialize Vmx
    //
    if (HvVmxInitialize())
    {
        LogDebugInfo("Hyperdbg's hypervisor loaded successfully :)");

        //
        // Initialize the debugger
        //

        if (DebuggerInitialize())
        {
            LogDebugInfo("Hyperdbg's debugger loaded successfully");

            //
            // Set the variable so no one else can get a handle anymore
            //
            g_HandleInUse = TRUE;

            Irp->IoStatus.Status      = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            return STATUS_SUCCESS;
        }
        else
        {
            LogError("Hyperdbg's debugger was not loaded");
        }
    }
    else
    {
        LogError("Hyperdbg's hypervisor was not loaded :(");
    }

    //
    // if we didn't return by now, means that there is a problem
    //

    Irp->IoStatus.Status      = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_UNSUCCESSFUL;
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
    LogWarning("Not implemented yet :(");

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
    LogWarning("Not implemented yet :(");

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
    DbgPrint("This function is not supported :(");

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}
