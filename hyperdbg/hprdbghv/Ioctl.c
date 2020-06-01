/**
 * @file Ioctl.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief IOCTL Functions form user mode and other parts 
 * @details 
 *
 * @version 0.1
 * @date 2020-06-01
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
