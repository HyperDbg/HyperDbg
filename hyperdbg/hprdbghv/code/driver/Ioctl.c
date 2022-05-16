/**
 * @file Ioctl.c
 * @author Sina Karvandi (sina@hyperdbg.org)
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
    PDEBUGGER_PREALLOC_COMMAND                              DebuggerReservePreallocPoolRequest;
    PDEBUGGER_UD_COMMAND_PACKET                             DebuggerUdCommandRequest;
    PUSERMODE_LOADED_MODULE_DETAILS                         DebuggerUsermodeModulesRequest;
    PDEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS             DebuggerUsermodeProcessOrThreadQueryRequest;
    PDEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET             GetInformationProcessRequest;
    PDEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET              GetInformationThreadRequest;
    PDEBUGGER_PERFORM_KERNEL_TESTS                          DebuggerKernelTestRequest;
    PDEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL        DebuggerCommandExecutionFinishedRequest;
    PDEBUGGEE_KERNEL_AND_USER_TEST_INFORMATION              DebuggerKernelSideTestInformationRequest;
    PDEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER            DebuggerSendUsermodeMessageRequest;
    PDEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER DebuggerSendBufferFromDebuggeeToDebuggerRequest;
    PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS               DebuggerAttachOrDetachToThreadRequest;
    PDEBUGGER_PREPARE_DEBUGGEE                              DebuggeeRequest;
    PDEBUGGER_PAUSE_PACKET_RECEIVED                         DebuggerPauseKernelRequest;
    PDEBUGGER_GENERAL_ACTION                                DebuggerNewActionRequest;
    PVOID                                                   BufferToStoreThreadsAndProcessesDetails;
    NTSTATUS                                                Status;
    ULONG                                                   InBuffLength;  // Input buffer length
    ULONG                                                   OutBuffLength; // Output buffer length
    SIZE_T                                                  ReturnSize;
    BOOLEAN                                                 DoNotChangeInformation = FALSE;
    UINT32                                                  SizeOfPrintRequestToBeDeliveredToUsermode;
    UINT32                                                  FilledEntriesInKernelInfo;

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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
                LogError("Err, unknow notification type from user-mode");
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
                          1,
                          TRUE);

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
            VmxPerformTermination();

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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
            // at the same place (it's not in vmx-root)
            //
            ExtensionCommandPte(DebuggerPteRequest, FALSE);

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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
            // valid or in other words whether we received enough space or not
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
                DebuggerHideAndUnhideRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
            }
            else
            {
                //
                // Set the status
                //
                if (DebuggerHideAndUnhideRequest->IsHide)
                {
                    DebuggerHideAndUnhideRequest->KernelStatus = DEBUGGER_ERROR_UNABLE_TO_HIDE_OR_UNHIDE_DEBUGGER;
                }
                else
                {
                    DebuggerHideAndUnhideRequest->KernelStatus = DEBUGGER_ERROR_DEBUGGER_ALREADY_UHIDE;
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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
            // at the same place (we're not in vmx-root here)
            //
            ExtensionCommandVa2paAndPa2va(DebuggerVa2paAndPa2vaRequest, FALSE);

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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
            // valid or in other words whether we received enough space or not
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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
            // valid or in other words whether we received enough space or not
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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
            // Perform the attach to the target process
            //
            AttachingTargetProcess(DebuggerAttachOrDetachToThreadRequest);

            Irp->IoStatus.Information = SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS;
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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
                LogError("Err, invalid parameter to IOCTL dispatcher");
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

        case IOCTL_SEND_GET_KERNEL_SIDE_TEST_INFORMATION:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGEE_KERNEL_AND_USER_TEST_INFORMATION ||
                Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
            DebuggerKernelSideTestInformationRequest =
                (PDEBUGGEE_KERNEL_AND_USER_TEST_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

            //
            // Perform collecting kernel-side debug information
            //
            FilledEntriesInKernelInfo = TestKernelGetInformation(DebuggerKernelSideTestInformationRequest);

            Irp->IoStatus.Information = FilledEntriesInKernelInfo * SIZEOF_DEBUGGEE_KERNEL_AND_USER_TEST_INFORMATION;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_PERFROM_KERNEL_SIDE_TESTS:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_PERFORM_KERNEL_TESTS ||
                Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
            DebuggerKernelTestRequest = (PDEBUGGER_PERFORM_KERNEL_TESTS)Irp->AssociatedIrp.SystemBuffer;

            //
            // Perform the kernel-side tests
            //
            TestKernelPerformTests(DebuggerKernelTestRequest);

            Irp->IoStatus.Information = SIZEOF_DEBUGGER_PERFORM_KERNEL_TESTS;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_RESERVE_PRE_ALLOCATED_POOLS:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGER_PREALLOC_COMMAND ||
                Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
            DebuggerReservePreallocPoolRequest = (PDEBUGGER_PREALLOC_COMMAND)Irp->AssociatedIrp.SystemBuffer;

            //
            // Perform the reservation pools
            //
            DebuggerCommandReservePreallocatedPools(DebuggerReservePreallocPoolRequest);

            Irp->IoStatus.Information = SIZEOF_DEBUGGER_PREALLOC_COMMAND;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_SEND_USER_DEBUGGER_COMMANDS:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DEBUGGER_UD_COMMAND_PACKET) ||
                Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
            DebuggerUdCommandRequest = (PDEBUGGER_UD_COMMAND_PACKET)Irp->AssociatedIrp.SystemBuffer;

            //
            // Perform the dispatching of user debugger command
            //
            UdDispatchUsermodeCommands(DebuggerUdCommandRequest);

            Irp->IoStatus.Information = sizeof(DEBUGGER_UD_COMMAND_PACKET);
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_GET_DETAIL_OF_ACTIVE_THREADS_AND_PROCESSES:

            OutBuffLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

            if (!OutBuffLength)
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // Both usermode and to send to usermode is here
            //
            BufferToStoreThreadsAndProcessesDetails = (PVOID)Irp->AssociatedIrp.SystemBuffer;

            //
            // Perform the dispatching of user debugger command
            //
            AttachingQueryDetailsOfActiveDebuggingThreadsAndProcesses(BufferToStoreThreadsAndProcessesDetails, OutBuffLength);

            Irp->IoStatus.Information = OutBuffLength;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_GET_USER_MODE_MODULE_DETAILS:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(USERMODE_LOADED_MODULE_DETAILS) ||
                Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
            DebuggerUsermodeModulesRequest = (PUSERMODE_LOADED_MODULE_DETAILS)Irp->AssociatedIrp.SystemBuffer;

            //
            // Getting the modules details
            //
            UserAccessGetLoadedModules(DebuggerUsermodeModulesRequest, OutBuffLength);

            Irp->IoStatus.Information = OutBuffLength;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_QUERY_COUNT_OF_ACTIVE_PROCESSES_OR_THREADS:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS) ||
                Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
            DebuggerUsermodeProcessOrThreadQueryRequest = (PDEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS)Irp->AssociatedIrp.SystemBuffer;

            //
            // Getting the count result
            //
            if (DebuggerUsermodeProcessOrThreadQueryRequest->QueryType ==
                DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_PROCESS_COUNT)
            {
                ProcessQueryCount(DebuggerUsermodeProcessOrThreadQueryRequest);
            }
            else if (DebuggerUsermodeProcessOrThreadQueryRequest->QueryType ==
                     DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_THREAD_COUNT)
            {
                ThreadQueryCount(DebuggerUsermodeProcessOrThreadQueryRequest);
            }

            Irp->IoStatus.Information = SIZEOF_DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_GET_LIST_OF_THREADS_AND_PROCESSES:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS) ||
                Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
            DebuggerUsermodeProcessOrThreadQueryRequest = (PDEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS)Irp->AssociatedIrp.SystemBuffer;

            //
            // Getting the list of processes or threads
            //
            if (DebuggerUsermodeProcessOrThreadQueryRequest->QueryType ==
                DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_PROCESS_LIST)
            {
                ProcessQueryList(DebuggerUsermodeProcessOrThreadQueryRequest,
                                 DebuggerUsermodeProcessOrThreadQueryRequest,
                                 OutBuffLength);
            }
            else if (DebuggerUsermodeProcessOrThreadQueryRequest->QueryType ==
                     DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_THREAD_LIST)
            {
                ThreadQueryList(DebuggerUsermodeProcessOrThreadQueryRequest,
                                DebuggerUsermodeProcessOrThreadQueryRequest,
                                OutBuffLength);
            }

            Irp->IoStatus.Information = OutBuffLength;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_QUERY_CURRENT_THREAD:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET ||
                Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
            GetInformationThreadRequest = (PDEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET)Irp->AssociatedIrp.SystemBuffer;

            //
            // Get the information
            //
            ThreadQueryDetails(GetInformationThreadRequest);

            Irp->IoStatus.Information = SIZEOF_DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        case IOCTL_QUERY_CURRENT_PROCESS:

            //
            // First validate the parameters.
            //
            if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET ||
                Irp->AssociatedIrp.SystemBuffer == NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                LogError("Err, invalid parameter to IOCTL dispatcher");
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
            GetInformationProcessRequest = (PDEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET)Irp->AssociatedIrp.SystemBuffer;

            //
            // Get the information
            //
            ProcessQueryDetails(GetInformationProcessRequest);

            Irp->IoStatus.Information = SIZEOF_DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET;
            Status                    = STATUS_SUCCESS;

            //
            // Avoid zeroing it
            //
            DoNotChangeInformation = TRUE;

            break;

        default:
            LogError("Err, unknown IOCTL");
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
