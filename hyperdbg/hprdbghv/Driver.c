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
#include <ntddk.h>
#include <wdf.h>
#include "Common.h"
#include "HypervisorRoutines.h"
#include "GlobalVariables.h"
#include "Logging.h"
#include "ExtensionCommands.h"
#include "DebuggerCommands.h"
#include "Hooks.h"
#include "Debugger.h"
#include "Trace.h"
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

    LogInfo("Hyperdbg is Loaded :)");

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

        LogInfo("Setting device major functions");
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

    LogInfo("Hyperdbg's hypervisor Started...");
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
    // Initialize Vmx
    //
    if (HvVmxInitialize())
    {
        LogInfo("Hyperdbg's hypervisor loaded successfully :)");

        //
        // Initialize the debugger
        //

        if (DebuggerInitialize())
        {
            LogInfo("Hyperdbg's debugger loaded successfully");

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

/**
 * @brief Driver IOCTL Dispatcher
 * 
 * @param DeviceObject 
 * @param Irp 
 * @return NTSTATUS 
 */
NTSTATUS
DrvDispatchIoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION                        IrpStack;
    PREGISTER_NOTIFY_BUFFER                   RegisterEventRequest;
    PDEBUGGER_READ_MEMORY                     DebuggerReadMemRequest;
    PDEBUGGER_READ_AND_WRITE_ON_MSR           DebuggerReadOrWriteMsrRequest;
    PDEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS DebuggerPteRequest;
    PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER     RegBufferResult;
    PDEBUGGER_GENERAL_EVENT_DETAIL            DebuggerNewEventRequest;
    PDEBUGGER_GENERAL_ACTION                  DebuggerNewActionRequest;
    NTSTATUS                                  Status;
    ULONG                                     InBuffLength;  // Input buffer length
    ULONG                                     OutBuffLength; // Output buffer length
    SIZE_T                                    ReturnSize;
    BOOLEAN                                   DoNotChangeInformation = FALSE;

    //
    // Here's the best place to see if there is any allocation pending
    // to be allcated as we're in PASSIVE_LEVEL
    //
    PoolManagerCheckAndPerformAllocation();

    if (g_AllowIOCTLFromUsermode)
    {
        IrpStack = IoGetCurrentIrpStackLocation(Irp);

        switch (IrpStack->Parameters.DeviceIoControl.IoControlCode)
        {
        case IOCTL_REGISTER_EVENT:
            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_REGISTER_EVENT || Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Invalid parameter to IOCTL Dispatcher.");
                break;
            }

            //
            // IRPs supply a pointer to a buffer at Irp->AssociatedIrp.SystemBuffer.
            // This buffer represents both the input buffer and the output buffer that
            // are specified in calls to DeviceIoControl
            //

            RegisterEventRequest = (PREGISTER_NOTIFY_BUFFER)Irp->AssociatedIrp.SystemBuffer;

            switch (RegisterEventRequest->Type)
            {
            case IRP_BASED:
                Status = LogRegisterIrpBasedNotification(DeviceObject, Irp);
                break;
            case EVENT_BASED:
                Status = LogRegisterEventBasedNotification(DeviceObject, Irp);
                break;
            default:
                LogError("Unknow notification type from user-mode");
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            break;
        case IOCTL_RETURN_IRP_PENDING_PACKETS_AND_DISALLOW_IOCTL:
            //
            // Dis-allow new IOCTL
            //
            g_AllowIOCTLFromUsermode = FALSE;

            //
            // Send an immediate message, and we're no longer get new IRP
            //
            LogInfoImmediate("An immediate message recieved, we no longer recieve IRPs from user-mode ");
            Status = STATUS_SUCCESS;
            break;
        case IOCTL_TERMINATE_VMX:

            //
            // terminate vmx
            //
            HvTerminateVmx();

            //
            // Uninitialize memory mapper
            //
            MemoryMapperUninitialize();

            Status = STATUS_SUCCESS;
            break;
        case IOCTL_DEBUGGER_READ_MEMORY:
            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_READ_MEMORY || Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Invalid parameter to IOCTL Dispatcher.");
                break;
            }

            InBuffLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
            OutBuffLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

            if (!InBuffLength || !OutBuffLength)
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            DebuggerReadMemRequest = (PDEBUGGER_READ_MEMORY)Irp->AssociatedIrp.SystemBuffer;

            Status = DebuggerCommandReadMemory(DebuggerReadMemRequest, DebuggerReadMemRequest, &ReturnSize);

            //
            // Set the size
            //
            if (Status == STATUS_SUCCESS)
            {
                Irp->IoStatus.Information = ReturnSize;

                //
                // Avoid zeroing it
                //
                DoNotChangeInformation = TRUE;
            }

            break;
        case IOCTL_DEBUGGER_READ_OR_WRITE_MSR:
            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_READ_AND_WRITE_ON_MSR || Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Invalid parameter to IOCTL Dispatcher.");
                break;
            }

            InBuffLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
            OutBuffLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

            if (!InBuffLength)
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            DebuggerReadOrWriteMsrRequest = (PDEBUGGER_READ_AND_WRITE_ON_MSR)Irp->AssociatedIrp.SystemBuffer;

            //
            // Only the the rdmsr needs and output buffer
            //
            if (DebuggerReadOrWriteMsrRequest->ActionType != DEBUGGER_MSR_WRITE)
            {
                if (!OutBuffLength)
                {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }
            }
            //
            // Both usermode and to send to usermode and the comming buffer are
            // at the same place
            //
            Status = DebuggerReadOrWriteMsr(DebuggerReadOrWriteMsrRequest, DebuggerReadOrWriteMsrRequest, &ReturnSize);

            //
            // Set the size
            //
            if (Status == STATUS_SUCCESS)
            {
                Irp->IoStatus.Information = ReturnSize;

                //
                // Avoid zeroing it
                //
                DoNotChangeInformation = TRUE;
            }
        case IOCTL_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS:
            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS || Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Invalid parameter to IOCTL Dispatcher.");
                break;
            }

            InBuffLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
            OutBuffLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

            if (!InBuffLength)
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            DebuggerPteRequest = (PDEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS)Irp->AssociatedIrp.SystemBuffer;

            //
            // Both usermode and to send to usermode and the comming buffer are
            // at the same place
            //
            if (ExtensionCommandPte(DebuggerPteRequest))
            {
                Irp->IoStatus.Information = SIZEOF_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS;
                Status                    = STATUS_SUCCESS;
                //
                // Avoid zeroing it
                //
                DoNotChangeInformation = TRUE;
            }
            else
            {
                Irp->IoStatus.Information = 0;
                Status                    = STATUS_UNSUCCESSFUL;
                //
                // Avoid zeroing it
                //
                DoNotChangeInformation = TRUE;
            }

            break;

        case IOCTL_DEBUGGER_REGISTER_EVENT:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DEBUGGER_GENERAL_EVENT_DETAIL) ||
                Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Invalid parameter to IOCTL Dispatcher.");
                break;
            }

            InBuffLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
            OutBuffLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

            if (!InBuffLength || !OutBuffLength)
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            DebuggerNewEventRequest = (PDEBUGGER_GENERAL_EVENT_DETAIL)Irp->AssociatedIrp.SystemBuffer;

            //
            // Both usermode and to send to usermode and the comming buffer are
            // at the same place
            //
            DebuggerParseEventFromUsermode(DebuggerNewEventRequest, InBuffLength, (PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER)Irp->AssociatedIrp.SystemBuffer);
            Irp->IoStatus.Information = sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER);
            Status                    = STATUS_SUCCESS;
            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;
        case IOCTL_DEBUGGER_ADD_ACTION_TO_EVENT:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DEBUGGER_GENERAL_ACTION) ||
                Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Invalid parameter to IOCTL Dispatcher.");
                break;
            }

            InBuffLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
            OutBuffLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

            if (!InBuffLength || !OutBuffLength)
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            DebuggerNewActionRequest = (PDEBUGGER_GENERAL_ACTION)Irp->AssociatedIrp.SystemBuffer;

            //
            // Both usermode and to send to usermode and the comming buffer are
            // at the same place
            //
            DebuggerParseActionFromUsermode(DebuggerNewActionRequest, InBuffLength, (PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER)Irp->AssociatedIrp.SystemBuffer);

            Irp->IoStatus.Information = sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER);
            Status                    = STATUS_SUCCESS;
            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;
        default:
            LogError("Unknow IOCTL");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }
    else
    {
        //
        // We're no longer serve IOCTL
        //
        Status = STATUS_SUCCESS;
    }

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        if (!DoNotChangeInformation)
        {
            Irp->IoStatus.Information = 0;
        }
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}
