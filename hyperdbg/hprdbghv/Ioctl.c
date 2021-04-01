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
#include "pch.h"

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
    PIO_STACK_LOCATION                                      IrpStack;
    PREGISTER_NOTIFY_BUFFER                                 RegisterEventRequest;
    PDEBUGGER_READ_MEMORY                                   DebuggerReadMemRequest;
    PDEBUGGER_READ_AND_WRITE_ON_MSR                         DebuggerReadOrWriteMsrRequest;
    PDEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE            DebuggerHideAndUnhideRequest;
    PDEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS               DebuggerPteRequest;
    PDEBUGGER_VA2PA_AND_PA2VA_COMMANDS                      DebuggerVa2paAndPa2vaRequest;
    PDEBUGGER_EDIT_MEMORY                                   DebuggerEditMemoryRequest;
    PDEBUGGER_SEARCH_MEMORY                                 DebuggerSearchMemoryRequest;
    PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER                   RegBufferResult;
    PDEBUGGER_GENERAL_EVENT_DETAIL                          DebuggerNewEventRequest;
    PDEBUGGER_MODIFY_EVENTS                                 DebuggerModifyEventRequest;
    PDEBUGGER_FLUSH_LOGGING_BUFFERS                         DebuggerFlushBuffersRequest;
    PDEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL        DebuggerCommandExecutionFinishedRequest;
    PDEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER            DebuggerSendUsermodeMessageRequest;
    PDEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER DebuggerSendBufferFromDebuggeeToDebuggerRequest;
    PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS               DebuggerAttachOrDetachToThreadRequest;
    PDEBUGGER_STEPPINGS                                     DebuggerSteppingsRequest;
    PDEBUGGER_PREPARE_DEBUGGEE                              DebuggeeRequest;
    PDEBUGGER_PAUSE_PACKET_RECEIVED                         DebuggerPauseKernelRequest;
    PDEBUGGER_GENERAL_ACTION                                DebuggerNewActionRequest;
    NTSTATUS                                                Status;
    ULONG                                                   InBuffLength;  // Input buffer length
    ULONG                                                   OutBuffLength; // Output buffer length
    SIZE_T                                                  ReturnSize;
    BOOLEAN                                                 DoNotChangeInformation = FALSE;
    UINT32                                                  SizeOfPrintRequestToBeDeliveredToUsermode;

    //
    // Here's the best place to see if there is any allocation pending
    // to be allcated as we're in PASSIVE_LEVEL
    //
    PoolManagerCheckAndPerformAllocationAndDeallocation();

    if (g_AllowIOCTLFromUsermode)
    {
        IrpStack = IoGetCurrentIrpStackLocation(Irp);

        switch (IrpStack->Parameters.DeviceIoControl.IoControlCode)
        {
        case IOCTL_REGISTER_EVENT:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_REGISTER_EVENT ||
                Irp->AssociatedIrp.SystemBuffer == NULL)
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
            LogSendBuffer(OPERATION_HYPERVISOR_DRIVER_END_OF_IRPS,
                          "$",
                          1);

            Status = STATUS_SUCCESS;
            break;
        case IOCTL_TERMINATE_VMX:

            //
            // Uninitialize the debugger and its sub-mechansims
            //
            DebuggerUninitialize();

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
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_READ_MEMORY ||
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
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_READ_AND_WRITE_ON_MSR ||
                Irp->AssociatedIrp.SystemBuffer == NULL)
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

            break;

        case IOCTL_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS ||
                Irp->AssociatedIrp.SystemBuffer == NULL)
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
            ExtensionCommandPte(DebuggerPteRequest);

            Irp->IoStatus.Information = SIZEOF_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

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
            DebuggerParseEventFromUsermode(DebuggerNewEventRequest,
                                           InBuffLength,
                                           (PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER)Irp->AssociatedIrp.SystemBuffer);

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
            DebuggerParseActionFromUsermode(DebuggerNewActionRequest,
                                            InBuffLength,
                                            (PDEBUGGER_EVENT_AND_ACTION_REG_BUFFER)Irp->AssociatedIrp.SystemBuffer);

            Irp->IoStatus.Information = sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER);
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_DEBUGGER_HIDE_AND_UNHIDE_TO_TRANSPARENT_THE_DEBUGGER:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE ||
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

            DebuggerHideAndUnhideRequest = (PDEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE)Irp->AssociatedIrp.SystemBuffer;

            //
            // Here we should validate whether the input parameter is
            // valid or in other words whether we recieved enough space or not
            //
            if (DebuggerHideAndUnhideRequest->TrueIfProcessIdAndFalseIfProcessName == FALSE &&
                IrpStack->Parameters.DeviceIoControl.InputBufferLength !=
                    SIZEOF_DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE + DebuggerHideAndUnhideRequest->LengthOfProcessName)
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // check if it's a !hide or !unhide command
            //
            if (DebuggerHideAndUnhideRequest->IsHide == TRUE)
            {
                //
                // It's a hide request
                //
                Status = TransparentHideDebugger(DebuggerHideAndUnhideRequest);
            }
            else
            {
                //
                // It's a unhide request
                //
                Status = TransparentUnhideDebugger();
            }

            if (Status == STATUS_SUCCESS)
            {
                //
                // Set the status
                //
                DebuggerHideAndUnhideRequest->KernelStatus = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
            }
            else
            {
                //
                // Set the status
                //
                if (DebuggerHideAndUnhideRequest->IsHide)
                {
                    DebuggerHideAndUnhideRequest->KernelStatus = DEBUGEER_ERROR_UNABLE_TO_HIDE_OR_UNHIDE_DEBUGGER;
                }
                else
                {
                    DebuggerHideAndUnhideRequest->KernelStatus = DEBUGEER_ERROR_DEBUGGER_ALREADY_UHIDE;
                }
            }

            //
            // Set size
            //
            Irp->IoStatus.Information = SIZEOF_DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_DEBUGGER_VA2PA_AND_PA2VA_COMMANDS:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_VA2PA_AND_PA2VA_COMMANDS ||
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

            DebuggerVa2paAndPa2vaRequest = (PDEBUGGER_VA2PA_AND_PA2VA_COMMANDS)Irp->AssociatedIrp.SystemBuffer;

            //
            // Both usermode and to send to usermode and the comming buffer are
            // at the same place
            //
            ExtensionCommandVa2paAndPa2va(DebuggerVa2paAndPa2vaRequest);

            //
            // Configure IRP status
            //
            Irp->IoStatus.Information = SIZEOF_DEBUGGER_VA2PA_AND_PA2VA_COMMANDS;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_DEBUGGER_EDIT_MEMORY:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_EDIT_MEMORY ||
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

            //
            // Cast buffer to understandable buffer
            //
            DebuggerEditMemoryRequest = (PDEBUGGER_EDIT_MEMORY)Irp->AssociatedIrp.SystemBuffer;

            //
            // Here we should validate whether the input parameter is
            // valid or in other words whether we recieved enough space or not
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength !=
                SIZEOF_DEBUGGER_EDIT_MEMORY + DebuggerEditMemoryRequest->CountOf64Chunks * sizeof(UINT64))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // Both usermode and to send to usermode and the comming buffer are
            // at the same place
            //
            DebuggerCommandEditMemory(DebuggerEditMemoryRequest);

            //
            // Configure IRP status
            //
            Irp->IoStatus.Information = SIZEOF_DEBUGGER_EDIT_MEMORY;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;
        case IOCTL_DEBUGGER_SEARCH_MEMORY:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_SEARCH_MEMORY ||
                Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Invalid parameter to IOCTL Dispatcher.");
                break;
            }

            InBuffLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
            OutBuffLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

            //
            // The OutBuffLength should have at least MaximumSearchResults * sizeof(UINT64)
            // free space to store the results
            //
            if (!InBuffLength || OutBuffLength < MaximumSearchResults * sizeof(UINT64))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // Cast buffer to understandable buffer
            //
            DebuggerSearchMemoryRequest = (PDEBUGGER_SEARCH_MEMORY)Irp->AssociatedIrp.SystemBuffer;

            //
            // Here we should validate whether the input parameter is
            // valid or in other words whether we recieved enough space or not
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength !=
                SIZEOF_DEBUGGER_SEARCH_MEMORY + DebuggerSearchMemoryRequest->CountOf64Chunks * sizeof(UINT64))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // Both usermode and to send to usermode and the comming buffer are
            // at the same place
            //
            if (DebuggerCommandSearchMemory(DebuggerSearchMemoryRequest) != STATUS_SUCCESS)
            {
                //
                // It is because it was not valid in any of the ways to the function
                // then we're sure that the usermode code won't interpret it's previous
                // buffer as a valid buffer and will not show it to the user
                //
                RtlZeroMemory(DebuggerSearchMemoryRequest, MaximumSearchResults * sizeof(UINT64));
            }

            //
            // Configure IRP status, and also we send the results
            // buffer, with it's null values (if any)
            //
            Irp->IoStatus.Information = MaximumSearchResults * sizeof(UINT64);
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_DEBUGGER_MODIFY_EVENTS:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DEBUGGER_MODIFY_EVENTS) ||
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

            DebuggerModifyEventRequest = (PDEBUGGER_MODIFY_EVENTS)Irp->AssociatedIrp.SystemBuffer;

            //
            // Both usermode and to send to usermode and the comming buffer are
            // at the same place
            //
            DebuggerParseEventsModificationFromUsermode(DebuggerModifyEventRequest);

            Irp->IoStatus.Information = SIZEOF_DEBUGGER_MODIFY_EVENTS;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_DEBUGGER_FLUSH_LOGGING_BUFFERS:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_FLUSH_LOGGING_BUFFERS ||
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

            //
            // Both usermode and to send to usermode and the comming buffer are
            // at the same place
            //
            DebuggerFlushBuffersRequest = (PDEBUGGER_FLUSH_LOGGING_BUFFERS)Irp->AssociatedIrp.SystemBuffer;

            //
            // Perform the flush
            //
            DebuggerCommandFlush(DebuggerFlushBuffersRequest);

            Irp->IoStatus.Information = SIZEOF_DEBUGGER_FLUSH_LOGGING_BUFFERS;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS ||
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

            //
            // Both usermode and to send to usermode and the comming buffer are
            // at the same place
            //
            DebuggerAttachOrDetachToThreadRequest = (PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS)Irp->AssociatedIrp.SystemBuffer;

            //
            // Perform the flush
            //
            SteppingsAttachOrDetachToThread(DebuggerAttachOrDetachToThreadRequest);

            Irp->IoStatus.Information = SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_DEBUGGER_STEPPINGS:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_STEPPINGS ||
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

            //
            // Both usermode and to send to usermode and the comming buffer are
            // at the same place
            //
            DebuggerSteppingsRequest = (PDEBUGGER_STEPPINGS)Irp->AssociatedIrp.SystemBuffer;

            //
            // Perform the steppings action
            //
            SteppingsPerformAction(DebuggerSteppingsRequest);

            Irp->IoStatus.Information = SIZEOF_DEBUGGER_STEPPINGS;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_PREPARE_DEBUGGEE:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_PREPARE_DEBUGGEE ||
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

            //
            // Both usermode and to send to usermode and the comming buffer are
            // at the same place
            //
            DebuggeeRequest = (PDEBUGGER_PREPARE_DEBUGGEE)Irp->AssociatedIrp.SystemBuffer;

            //
            // Perform the action
            //
            SerialConnectionPrepare(DebuggeeRequest);

            Irp->IoStatus.Information = SIZEOF_DEBUGGER_PREPARE_DEBUGGEE;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_PAUSE_PACKET_RECEIVED:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_PAUSE_PACKET_RECEIVED ||
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

            //
            // Both usermode and to send to usermode and the comming buffer are
            // at the same place
            //
            DebuggerPauseKernelRequest = (PDEBUGGER_PAUSE_PACKET_RECEIVED)Irp->AssociatedIrp.SystemBuffer;

            //
            // Perform the action
            //
            KdHaltSystem(DebuggerPauseKernelRequest);

            Irp->IoStatus.Information = SIZEOF_DEBUGGER_PAUSE_PACKET_RECEIVED;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_SEND_SIGNAL_EXECUTION_IN_DEBUGGEE_FINISHED:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL ||
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

            //
            // Both usermode and to send to usermode and the comming buffer are
            // at the same place
            //
            DebuggerCommandExecutionFinishedRequest =
                (PDEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL)Irp->AssociatedIrp.SystemBuffer;

            //
            // Perform the signal operation
            //
            DebuggerCommandSignalExecutionState(DebuggerCommandExecutionFinishedRequest);

            Irp->IoStatus.Information = SIZEOF_DEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_SEND_USERMODE_MESSAGES_TO_DEBUGGER:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER ||
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

            //
            // Both usermode and to send to usermode and the comming buffer are
            // at the same place
            //
            DebuggerSendUsermodeMessageRequest =
                (PDEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER)Irp->AssociatedIrp.SystemBuffer;

            //
            // Second validation phase
            //
            if (DebuggerSendUsermodeMessageRequest->Length == NULL ||
                IrpStack->Parameters.DeviceIoControl.InputBufferLength !=
                    SIZEOF_DEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER + DebuggerSendUsermodeMessageRequest->Length)
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // Perform the signal operation
            //
            DebuggerCommandSendMessage(DebuggerSendUsermodeMessageRequest);

            Irp->IoStatus.Information = SIZEOF_DEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_SEND_GENERAL_BUFFER_FROM_DEBUGGEE_TO_DEBUGGER:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER ||
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

            //
            // Both usermode and to send to usermode and the comming buffer are
            // at the same place
            //
            DebuggerSendBufferFromDebuggeeToDebuggerRequest =
                (PDEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER)Irp->AssociatedIrp.SystemBuffer;

            //
            // Second validation phase
            //
            if (DebuggerSendBufferFromDebuggeeToDebuggerRequest->LengthOfBuffer == NULL ||
                IrpStack->Parameters.DeviceIoControl.InputBufferLength !=
                    SIZEOF_DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER +
                        DebuggerSendBufferFromDebuggeeToDebuggerRequest->LengthOfBuffer)
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // Perform the signal operation
            //
            DebuggerCommandSendGeneralBufferToDebugger(DebuggerSendBufferFromDebuggeeToDebuggerRequest);

            Irp->IoStatus.Information = SIZEOF_DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER;
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
