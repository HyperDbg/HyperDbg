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
 * @brief Validates amd adjusts the parameters of an IOCTL request
 * @param BufferSize The expected size of the input buffer
 * @param Irp The IRP representing the IOCTL request
 * @IrpStack The current stack location of the IRP
 * @InBuffLength Output parameter to receive the actual input buffer length
 * @OutBuffLength Output parameter to receive the actual output buffer length
 *
 * @return TRUE if the parameters are valid, FALSE otherwise
 */
BOOLEAN
DrvValidateAndAdjustIoctlParameter(UINT32             BufferSize,
                                   PVOID *            TargetBuffer,
                                   PIRP               Irp,
                                   PIO_STACK_LOCATION IrpStack,
                                   ULONG *            InBuffLength,
                                   ULONG *            OutBuffLength)
{
    //
    // First validate the parameters
    //
    if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < BufferSize || Irp->AssociatedIrp.SystemBuffer == NULL)
    {
        LogError("Err, invalid parameter to IOCTL dispatcher");
        return FALSE;
    }

    *InBuffLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    *OutBuffLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (!*InBuffLength || !*OutBuffLength)
    {
        return FALSE;
    }

    //
    // Set the target buffer to the system buffer of the IRP
    //
    *TargetBuffer = Irp->AssociatedIrp.SystemBuffer;

    //
    // Validation was successful
    //
    return TRUE;
}

/**
 * @brief Adjusts the status and output buffer size for an IOCTL request
 *
 * @param ExpectedOutputBufferSize The expected size of the output buffer
 * @param DoNotChangeInformation Output parameter to indicate whether to change the information field of the IRP's I/O status block
 * @param Irp The IRP representing the IOCTL request
 * @param Status Output parameter to receive the status to be set in the IRP's I/O status block
 *
 * @return VOID
 */
VOID
DrvAdjustStatusAndSetOutputSize(UINT32     ExpectedOutputBufferSize,
                                BOOLEAN *  DoNotChangeInformation,
                                PIRP       Irp,
                                NTSTATUS * Status)
{
    Irp->IoStatus.Information = ExpectedOutputBufferSize;
    *Status                   = STATUS_SUCCESS;

    //
    // Avoid zeroing it
    //
    *DoNotChangeInformation = TRUE;
}

/**
 * @brief Checks whether the IOCTL request is allowed based on the current state of the driver and the system
 * @param Ioctl The IOCTL code of the request
 *
 * @return BOOLEAN TRUE if the IOCTL request is allowed, FALSE otherwise
 */
BOOLEAN
IoctlCheckIoctlAllowed(ULONG Ioctl)
{
    ULONG IoctlFunction = CTL_CODE_FUNCTION(Ioctl);

    //
    // First 100 IOCTLs are about loading and initializing modules
    //
    if (IoctlFunction > IOCTL_BASIC_IOCTL && IoctlFunction <= IOCTL_BASIC_IOCTL + 0x100)
    {
        //
        // Always allow these IOCTLs even if we don't allow IOCTL from user-mode, because they are used for loading and initializing the driver and its components
        //
        return TRUE;
    }
    else if (IoctlFunction > IOCTL_KD_IOCTL && IoctlFunction <= IOCTL_KD_IOCTL + 0x100)
    {
        //
        // Allow if the KD module is initialized
        //
        return g_KdInitialized;
    }
    else if (IoctlFunction > IOCTL_VMM_IOCTL && IoctlFunction <= IOCTL_VMM_IOCTL + 0x100)
    {
        //
        // Allow if the VMM module is initialized
        //
        return g_VmmInitialized;
    }
    else if (IoctlFunction > IOCTL_HYPERTRACE_IOCTL && IoctlFunction <= IOCTL_HYPERTRACE_IOCTL + 0x100)
    {
        //
        // Allow if the HyperTrace module is initialized
        //
        return g_HyperTraceInitialized;
    }
    else
    {
        //
        // For other (unknown) IOCTLs, we don't allow them
        //
        return FALSE;
    }
}

/**
 * @brief IOCTL Dispatcher for Basic IOCTLs (initialization and event registration)
 *
 * @param Irp
 * @param IrpStack
 * @param DoNotChangeInformation
 * @return NTSTATUS
 */
NTSTATUS
DrvDispatchBasicIoControl(PIRP Irp, PIO_STACK_LOCATION IrpStack, BOOLEAN * DoNotChangeInformation)
{
    PREGISTER_NOTIFY_BUFFER          RegisterEventRequest;
    PDEBUGGER_INIT_VMM_PACKET        InitVmmRequest;
    PDEBUGGER_INIT_HYPERTRACE_PACKET InitHyperTraceRequest;
    ULONG                            InBuffLength;
    ULONG                            OutBuffLength;
    NTSTATUS                         Status = STATUS_SUCCESS;
    UINT32                           Ioctl  = IrpStack->Parameters.DeviceIoControl.IoControlCode;

    switch (Ioctl)
    {
    case IOCTL_INIT_VMM:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_INIT_VMM_PACKET,
                                                (PVOID *)&InitVmmRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Initialize the debugger and the vmm
        //
        if (LoaderInitDebuggerAndVmm(InitVmmRequest))
        {
            Status = STATUS_SUCCESS;
        }
        else
        {
            //
            // There was a problem, so not loaded
            //
            Status = STATUS_UNSUCCESSFUL;
        }

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_INIT_VMM_PACKET, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_INIT_HYPERTRACE:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_INIT_HYPERTRACE_PACKET,
                                                (PVOID *)&InitHyperTraceRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Initialize the HyperTrace (if supported by the processor)
        //
        LoaderInitHyperTrace(InitHyperTraceRequest, TRUE);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_INIT_HYPERTRACE_PACKET, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_REGISTER_EVENT:

        //
        // First validate the parameters.
        //
        if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < SIZEOF_REGISTER_EVENT || Irp->AssociatedIrp.SystemBuffer == NULL)
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

            LogRegisterIrpBasedNotification((PVOID)Irp, &Status);

            break;
        case EVENT_BASED:

            if (LogRegisterEventBasedNotification((PVOID)Irp))
            {
                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = STATUS_UNSUCCESSFUL;
            }

            break;
        default:
            LogError("Err, unknown notification type from user-mode");
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        break;

    case IOCTL_RETURN_IRP_PENDING_PACKETS_AND_DISALLOW_IOCTL:

        //
        // Send an immediate message, and we're no longer get new IRP
        //
        LogCallbackSendBuffer(OPERATION_HYPERVISOR_DRIVER_END_OF_IRPS,
                              "$",
                              sizeof(CHAR),
                              TRUE);

        Status = STATUS_SUCCESS;

        break;

    default:
        LogError("Err, unknown IOCTL");
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    return Status;
}

/**
 * @brief IOCTL Dispatcher for KD (Kernel Debugger) IOCTLs
 *
 * @param Irp
 * @param IrpStack
 * @param DoNotChangeInformation
 * @return NTSTATUS
 */
NTSTATUS
DrvDispatchKdIoControl(PIRP Irp, PIO_STACK_LOCATION IrpStack, BOOLEAN * DoNotChangeInformation)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UINT32   Ioctl  = IrpStack->Parameters.DeviceIoControl.IoControlCode;

    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(DoNotChangeInformation);

    switch (Ioctl)
    {
    default:
        LogError("Err, unknown IOCTL");
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    return Status;
}

/**
 * @brief IOCTL Dispatcher for VMM IOCTLs
 *
 * @param Irp
 * @param IrpStack
 * @param DoNotChangeInformation
 * @return NTSTATUS
 */
NTSTATUS
DrvDispatchVmmIoControl(PIRP Irp, PIO_STACK_LOCATION IrpStack, BOOLEAN * DoNotChangeInformation)
{
    PDEBUGGER_READ_MEMORY                                   DebuggerReadMemRequest;
    PDEBUGGER_READ_AND_WRITE_ON_MSR                         DebuggerReadOrWriteMsrRequest;
    PDEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE            DebuggerHideAndUnhideRequest;
    PDEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS               DebuggerPteRequest;
    PDEBUGGER_PAGE_IN_REQUEST                               DebuggerPageinRequest;
    PDEBUGGEE_PCITREE_REQUEST_RESPONSE_PACKET               PcitreeRequest;
    PDEBUGGEE_PCIDEVINFO_REQUEST_RESPONSE_PACKET            PcidevinfoRequest;
    PDEBUGGER_VA2PA_AND_PA2VA_COMMANDS                      DebuggerVa2paAndPa2vaRequest;
    PDEBUGGER_EDIT_MEMORY                                   DebuggerEditMemoryRequest;
    PDEBUGGER_SEARCH_MEMORY                                 DebuggerSearchMemoryRequest;
    PDEBUGGER_GENERAL_EVENT_DETAIL                          DebuggerNewEventRequest;
    PDEBUGGER_MODIFY_EVENTS                                 DebuggerModifyEventRequest;
    PDEBUGGER_FLUSH_LOGGING_BUFFERS                         DebuggerFlushBuffersRequest;
    PDEBUGGER_PREALLOC_COMMAND                              DebuggerReservePreallocPoolRequest;
    PDEBUGGER_PREACTIVATE_COMMAND                           DebuggerPreactivationRequest;
    PDEBUGGER_APIC_REQUEST                                  DebuggerApicRequest;
    PINTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS             DebuggerQueryIdtRequest;
    PDEBUGGEE_BP_PACKET                                     DebuggerBreakpointRequest;
    PDEBUGGER_UD_COMMAND_PACKET                             DebuggerUdCommandRequest;
    PUSERMODE_LOADED_MODULE_DETAILS                         DebuggerUsermodeModulesRequest;
    PDEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS             DebuggerUsermodeProcessOrThreadQueryRequest;
    PDEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET             GetInformationProcessRequest;
    PREVERSING_MACHINE_RECONSTRUCT_MEMORY_REQUEST           RevServiceRequest;
    PDEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET              GetInformationThreadRequest;
    PDEBUGGER_PERFORM_KERNEL_TESTS                          DebuggerKernelTestRequest;
    PDEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL        DebuggerCommandExecutionFinishedRequest;
    PDEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER            DebuggerSendUsermodeMessageRequest;
    PDEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER DebuggerSendBufferFromDebuggeeToDebuggerRequest;
    PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS               DebuggerAttachOrDetachToThreadRequest;
    PDEBUGGER_PREPARE_DEBUGGEE                              DebuggeeRequest;
    PDEBUGGER_PAUSE_PACKET_RECEIVED                         DebuggerPauseKernelRequest;
    PDEBUGGER_GENERAL_ACTION                                DebuggerNewActionRequest;
    PSMI_OPERATION_PACKETS                                  SmiOperationRequest;
    PVOID                                                   BufferToStoreThreadsAndProcessesDetails;
    ULONG                                                   InBuffLength;  // Input buffer length
    ULONG                                                   OutBuffLength; // Output buffer length
    SIZE_T                                                  ReturnSize;
    NTSTATUS                                                Status = STATUS_SUCCESS;
    UINT32                                                  Ioctl  = IrpStack->Parameters.DeviceIoControl.IoControlCode;

    switch (Ioctl)
    {
    case IOCTL_TERMINATE_VMX:

        //
        // Uninitialize the VMM and the debugger
        //
        LoaderUninitVmmAndDebugger();

        Status = STATUS_SUCCESS;

        break;

    case IOCTL_DEBUGGER_READ_MEMORY:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_READ_MEMORY,
                                                (PVOID *)&DebuggerReadMemRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (DebuggerCommandReadMemory(DebuggerReadMemRequest,
                                      ((CHAR *)DebuggerReadMemRequest) + SIZEOF_DEBUGGER_READ_MEMORY,
                                      &ReturnSize) == TRUE)
        {
            //
            // Return the header a read bytes
            //
            DrvAdjustStatusAndSetOutputSize((UINT32)(ReturnSize + SIZEOF_DEBUGGER_READ_MEMORY), DoNotChangeInformation, Irp, &Status);
        }
        else
        {
            //
            // Just return the header to the user-mode
            //
            DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_READ_MEMORY, DoNotChangeInformation, Irp, &Status);
        }

        break;

    case IOCTL_DEBUGGER_READ_OR_WRITE_MSR:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_READ_AND_WRITE_ON_MSR,
                                                (PVOID *)&DebuggerReadOrWriteMsrRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Both usermode and to send to usermode and the coming buffer are
        // at the same place
        //
        Status = DebuggerReadOrWriteMsr(DebuggerReadOrWriteMsrRequest, (UINT64 *)DebuggerReadOrWriteMsrRequest, &ReturnSize);

        //
        // Set the size
        //
        if (Status == STATUS_SUCCESS)
        {
            //
            // Adjust the status and output size
            //
            DrvAdjustStatusAndSetOutputSize((UINT32)ReturnSize, DoNotChangeInformation, Irp, &Status);
        }

        break;

    case IOCTL_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS,
                                                (PVOID *)&DebuggerPteRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Both usermode and to send to usermode and the coming buffer are
        // at the same place (it's not in vmx-root)
        //
        ExtensionCommandPte(DebuggerPteRequest, FALSE);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_DEBUGGER_REGISTER_EVENT:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_GENERAL_EVENT_DETAIL,
                                                (PVOID *)&DebuggerNewEventRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Both usermode and to send to usermode and the coming buffer are
        // at the same place (not coming from the VMX-root mode)
        //
        DebuggerParseEvent(DebuggerNewEventRequest,
                           (PDEBUGGER_EVENT_AND_ACTION_RESULT)Irp->AssociatedIrp.SystemBuffer,
                           FALSE);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(sizeof(DEBUGGER_EVENT_AND_ACTION_RESULT), DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_DEBUGGER_ADD_ACTION_TO_EVENT:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_GENERAL_ACTION,
                                                (PVOID *)&DebuggerNewActionRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Both usermode and to send to usermode and the coming buffer are
        // at the same place
        //
        DebuggerParseAction(DebuggerNewActionRequest,
                            (PDEBUGGER_EVENT_AND_ACTION_RESULT)Irp->AssociatedIrp.SystemBuffer,
                            FALSE);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(sizeof(DEBUGGER_EVENT_AND_ACTION_RESULT), DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_DEBUGGER_HIDE_AND_UNHIDE_TO_TRANSPARENT_THE_DEBUGGER:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE,
                                                (PVOID *)&DebuggerHideAndUnhideRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
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
            TransparentHideDebuggerWrapper(DebuggerHideAndUnhideRequest);
        }
        else
        {
            //
            // It's a unhide request
            //
            TransparentUnhideDebuggerWrapper(DebuggerHideAndUnhideRequest);
        }

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_DEBUGGER_VA2PA_AND_PA2VA_COMMANDS:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_VA2PA_AND_PA2VA_COMMANDS,
                                                (PVOID *)&DebuggerVa2paAndPa2vaRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Both usermode and to send to usermode and the coming buffer are
        // at the same place (we're not in vmx-root here)
        //
        ExtensionCommandVa2paAndPa2va(DebuggerVa2paAndPa2vaRequest, FALSE);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_VA2PA_AND_PA2VA_COMMANDS, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_DEBUGGER_EDIT_MEMORY:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_EDIT_MEMORY,
                                                (PVOID *)&DebuggerEditMemoryRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Here we should validate whether the input parameter is
        // valid or in other words whether we received enough space or not
        //
        if (IrpStack->Parameters.DeviceIoControl.InputBufferLength != SIZEOF_DEBUGGER_EDIT_MEMORY + DebuggerEditMemoryRequest->CountOf64Chunks * sizeof(UINT64))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Both usermode and to send to usermode and the coming buffer are
        // at the same place
        //
        DebuggerCommandEditMemory(DebuggerEditMemoryRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_EDIT_MEMORY, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_DEBUGGER_SEARCH_MEMORY:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_SEARCH_MEMORY,
                                                (PVOID *)&DebuggerSearchMemoryRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // The OutBuffLength should have at least MaximumSearchResults * sizeof(UINT64)
        // free space to store the results
        //
        if (OutBuffLength < MaximumSearchResults * sizeof(UINT64))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Here we should validate whether the input parameter is
        // valid or in other words whether we received enough space or not
        //
        if (IrpStack->Parameters.DeviceIoControl.InputBufferLength != SIZEOF_DEBUGGER_SEARCH_MEMORY + DebuggerSearchMemoryRequest->CountOf64Chunks * sizeof(UINT64))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Both usermode and to send to usermode and the coming buffer are
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
        DrvAdjustStatusAndSetOutputSize(MaximumSearchResults * sizeof(UINT64), DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_DEBUGGER_MODIFY_EVENTS:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_MODIFY_EVENTS,
                                                (PVOID *)&DebuggerModifyEventRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Both usermode and to send to usermode and the coming buffer are
        // at the same place
        //
        DebuggerParseEventsModification(DebuggerModifyEventRequest, FALSE, EnableInstantEventMechanism ? g_KernelDebuggerState : FALSE);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_MODIFY_EVENTS, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_DEBUGGER_FLUSH_LOGGING_BUFFERS:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_FLUSH_LOGGING_BUFFERS,
                                                (PVOID *)&DebuggerFlushBuffersRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the flush
        //
        DebuggerCommandFlush(DebuggerFlushBuffersRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_FLUSH_LOGGING_BUFFERS, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS,
                                                (PVOID *)&DebuggerAttachOrDetachToThreadRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the attach to the target process
        //
        AttachingTargetProcess(DebuggerAttachOrDetachToThreadRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_PREPARE_DEBUGGEE:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_PREPARE_DEBUGGEE,
                                                (PVOID *)&DebuggeeRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the action
        //
        SerialConnectionPrepare(DebuggeeRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_PREPARE_DEBUGGEE, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_PAUSE_PACKET_RECEIVED:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_PAUSE_PACKET_RECEIVED,
                                                (PVOID *)&DebuggerPauseKernelRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the action
        //
        KdHaltSystem(DebuggerPauseKernelRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_PAUSE_PACKET_RECEIVED, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_SEND_SIGNAL_EXECUTION_IN_DEBUGGEE_FINISHED:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL,
                                                (PVOID *)&DebuggerCommandExecutionFinishedRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the signal operation
        //
        DebuggerCommandSignalExecutionState(DebuggerCommandExecutionFinishedRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_SEND_USERMODE_MESSAGES_TO_DEBUGGER:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER,
                                                (PVOID *)&DebuggerSendUsermodeMessageRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Second validation phase
        //
        if (DebuggerSendUsermodeMessageRequest->Length == NULL_ZERO ||
            IrpStack->Parameters.DeviceIoControl.InputBufferLength != SIZEOF_DEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER + DebuggerSendUsermodeMessageRequest->Length)
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the signal operation
        //
        DebuggerCommandSendMessage(DebuggerSendUsermodeMessageRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_SEND_GENERAL_BUFFER_FROM_DEBUGGEE_TO_DEBUGGER:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER,
                                                (PVOID *)&DebuggerSendBufferFromDebuggeeToDebuggerRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Second validation phase
        //
        if (DebuggerSendBufferFromDebuggeeToDebuggerRequest->LengthOfBuffer == NULL_ZERO ||
            IrpStack->Parameters.DeviceIoControl.InputBufferLength != SIZEOF_DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER + DebuggerSendBufferFromDebuggeeToDebuggerRequest->LengthOfBuffer)
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the signal operation
        //
        DebuggerCommandSendGeneralBufferToDebugger(DebuggerSendBufferFromDebuggeeToDebuggerRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_PERFORM_KERNEL_SIDE_TESTS:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_PERFORM_KERNEL_TESTS,
                                                (PVOID *)&DebuggerKernelTestRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the kernel-side tests
        //
        TestKernelPerformTests(DebuggerKernelTestRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_PERFORM_KERNEL_TESTS, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_RESERVE_PRE_ALLOCATED_POOLS:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_PREALLOC_COMMAND,
                                                (PVOID *)&DebuggerReservePreallocPoolRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the reservation pools
        //
        DebuggerCommandReservePreallocatedPools(DebuggerReservePreallocPoolRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_PREALLOC_COMMAND, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_PREACTIVATE_FUNCTIONALITY:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_PREACTIVATE_COMMAND,
                                                (PVOID *)&DebuggerPreactivationRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the activation of the functionality
        //
        DebuggerCommandPreactivateFunctionality(DebuggerPreactivationRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_PREACTIVATE_COMMAND, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_PERFORM_ACTIONS_ON_APIC:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_APIC_REQUEST,
                                                (PVOID *)&DebuggerApicRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(ExtensionCommandPerformActionsForApicRequests(DebuggerApicRequest), DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_QUERY_IDT_ENTRY:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_INTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS,
                                                (PVOID *)&DebuggerQueryIdtRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the query of IDT entries (not from vmx-root)
        //
        ExtensionCommandPerformQueryIdtEntriesRequest(DebuggerQueryIdtRequest, FALSE);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_INTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_SET_BREAKPOINT_USER_DEBUGGER:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGEE_BP_PACKET,
                                                (PVOID *)&DebuggerBreakpointRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform setting the breakpoint (for the user mode debugger)
        // Switching to the target process memory is needed as we are
        // in HyperDbg's process memory layout and we need to switch to
        // the target process memory layout to set the breakpoint
        //
        BreakpointAddNew(DebuggerBreakpointRequest, TRUE);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGEE_BP_PACKET, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_PERFORM_SMI_OPERATION:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_SMI_OPERATION_PACKETS,
                                                (PVOID *)&SmiOperationRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the SMI operation (it's not from vmx-root)
        //
        VmFuncSmmPerformSmiOperation(SmiOperationRequest, FALSE);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_SMI_OPERATION_PACKETS, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_SEND_USER_DEBUGGER_COMMANDS:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_UD_COMMAND_PACKET,
                                                (PVOID *)&DebuggerUdCommandRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the dispatching of user debugger command
        //
        UdDispatchUsermodeCommands(DebuggerUdCommandRequest, InBuffLength, OutBuffLength);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(OutBuffLength, DoNotChangeInformation, Irp, &Status);

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

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(OutBuffLength, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_GET_USER_MODE_MODULE_DETAILS:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_USERMODE_LOADED_MODULE_DETAILS,
                                                (PVOID *)&DebuggerUsermodeModulesRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Getting the modules details
        //
        UserAccessGetLoadedModules(DebuggerUsermodeModulesRequest, OutBuffLength);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(OutBuffLength, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_QUERY_COUNT_OF_ACTIVE_PROCESSES_OR_THREADS:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS,
                                                (PVOID *)&DebuggerUsermodeProcessOrThreadQueryRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Getting the count result
        //
        if (DebuggerUsermodeProcessOrThreadQueryRequest->QueryType == DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_PROCESS_COUNT)
        {
            ProcessQueryCount(DebuggerUsermodeProcessOrThreadQueryRequest);
        }
        else if (DebuggerUsermodeProcessOrThreadQueryRequest->QueryType == DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_THREAD_COUNT)
        {
            ThreadQueryCount(DebuggerUsermodeProcessOrThreadQueryRequest);
        }

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_GET_LIST_OF_THREADS_AND_PROCESSES:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS,
                                                (PVOID *)&DebuggerUsermodeProcessOrThreadQueryRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Getting the list of processes or threads
        //
        if (DebuggerUsermodeProcessOrThreadQueryRequest->QueryType == DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_PROCESS_LIST)
        {
            ProcessQueryList(DebuggerUsermodeProcessOrThreadQueryRequest,
                             DebuggerUsermodeProcessOrThreadQueryRequest,
                             OutBuffLength);
        }
        else if (DebuggerUsermodeProcessOrThreadQueryRequest->QueryType == DEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS_QUERY_THREAD_LIST)
        {
            ThreadQueryList(DebuggerUsermodeProcessOrThreadQueryRequest,
                            DebuggerUsermodeProcessOrThreadQueryRequest,
                            OutBuffLength);
        }

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(OutBuffLength, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_QUERY_CURRENT_THREAD:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET,
                                                (PVOID *)&GetInformationThreadRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Get the information
        //
        ThreadQueryDetails(GetInformationThreadRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_QUERY_CURRENT_PROCESS:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET,
                                                (PVOID *)&GetInformationProcessRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Get the information
        //
        ProcessQueryDetails(GetInformationProcessRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_REQUEST_REV_MACHINE_SERVICE:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_REVERSING_MACHINE_RECONSTRUCT_MEMORY_REQUEST,
                                                (PVOID *)&RevServiceRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the service request
        //
        ConfigureInitializeExecTrapOnAllProcessors();

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_REVERSING_MACHINE_RECONSTRUCT_MEMORY_REQUEST, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_DEBUGGER_BRING_PAGES_IN:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGER_PAGE_IN_REQUEST,
                                                (PVOID *)&DebuggerPageinRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Both usermode and to send to usermode and the coming buffer are
        // at the same place (it's in VMI-mode)
        //
        DebuggerCommandBringPagein(DebuggerPageinRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGER_PAGE_IN_REQUEST, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_PCIE_ENDPOINT_ENUM:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGEE_PCITREE_REQUEST_RESPONSE_PACKET,
                                                (PVOID *)&PcitreeRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Both usermode and to send to usermode and the coming buffer are
        // at the same place (it's in VMI-mode)
        //
        ExtensionCommandPcitree(PcitreeRequest, FALSE);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGEE_PCITREE_REQUEST_RESPONSE_PACKET, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_PCIDEVINFO_ENUM:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_DEBUGGEE_PCIDEVINFO_REQUEST_RESPONSE_PACKET,
                                                (PVOID *)&PcidevinfoRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Both usermode and to send to usermode and the coming buffer are
        // at the same place (it's in VMI-mode)
        //
        ExtensionCommandPcidevinfo(PcidevinfoRequest, FALSE);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_DEBUGGEE_PCIDEVINFO_REQUEST_RESPONSE_PACKET, DoNotChangeInformation, Irp, &Status);

        break;

    default:
        LogError("Err, unknown IOCTL");
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    return Status;
}

/**
 * @brief IOCTL Dispatcher for HyperTrace IOCTLs
 *
 * @param Irp
 * @param IrpStack
 * @param DoNotChangeInformation
 * @return NTSTATUS
 */
NTSTATUS
DrvDispatchHyperTraceIoControl(PIRP Irp, PIO_STACK_LOCATION IrpStack, BOOLEAN * DoNotChangeInformation)
{
    PHYPERTRACE_LBR_OPERATION_PACKETS HyperTraceLbrOperationRequest;
    PHYPERTRACE_LBR_DUMP_PACKETS      HyperTraceLbrdumpRequest;
    PHYPERTRACE_PT_OPERATION_PACKETS  HyperTracePtOperationRequest;
    ULONG                             InBuffLength;
    ULONG                             OutBuffLength;
    NTSTATUS                          Status = STATUS_SUCCESS;
    UINT32                            Ioctl  = IrpStack->Parameters.DeviceIoControl.IoControlCode;

    switch (Ioctl)
    {
    case IOCTL_PERFORM_HYPERTRACE_LBR_OPERATION:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_HYPERTRACE_LBR_OPERATION_PACKETS,
                                                (PVOID *)&HyperTraceLbrOperationRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the HyperTrace LBR operation
        //
        HyperTraceLbrPerformOperation(HyperTraceLbrOperationRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_HYPERTRACE_LBR_OPERATION_PACKETS, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_PERFORM_HYPERTRACE_LBR_DUMP:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_HYPERTRACE_LBR_DUMP_PACKETS,
                                                (PVOID *)&HyperTraceLbrdumpRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the HyperTrace LBR dump operation
        //
        HyperTraceLbrPerformDump(HyperTraceLbrdumpRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_HYPERTRACE_LBR_DUMP_PACKETS, DoNotChangeInformation, Irp, &Status);

        break;

    case IOCTL_PERFORM_HYPERTRACE_PT_OPERATION:

        //
        // Validate and adjust the parameters, and set the target buffer to the system buffer of the IRP
        //
        if (!DrvValidateAndAdjustIoctlParameter(SIZEOF_HYPERTRACE_PT_OPERATION_PACKETS,
                                                (PVOID *)&HyperTracePtOperationRequest,
                                                Irp,
                                                IrpStack,
                                                &InBuffLength,
                                                &OutBuffLength))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Perform the HyperTrace PT operation
        //
        HyperTracePtPerformOperation(HyperTracePtOperationRequest);

        //
        // Adjust the status and output size
        //
        DrvAdjustStatusAndSetOutputSize(SIZEOF_HYPERTRACE_PT_OPERATION_PACKETS, DoNotChangeInformation, Irp, &Status);

        break;

    default:
        LogError("Err, unknown IOCTL");
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    return Status;
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
    UNREFERENCED_PARAMETER(DeviceObject);

    PIO_STACK_LOCATION IrpStack;
    NTSTATUS           Status                 = STATUS_SUCCESS;
    BOOLEAN            DoNotChangeInformation = FALSE;
    UINT32             Ioctl                  = 0;
    ULONG              IoctlFunction          = 0;

    //
    // Here's the best place to see if there is any allocation pending
    // to be allocated as we're in PASSIVE_LEVEL
    //
    PoolManagerCheckAndPerformAllocationAndDeallocation();

    //
    // Get the current stack location of the IRP to access the parameters of the IOCTL request
    //
    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the IOCTL code from the parameters
    //
    Ioctl = IrpStack->Parameters.DeviceIoControl.IoControlCode;

    //
    // If we don't allow IOCTL from user-mode, we just complete the request with success, and return
    //
    if (!IoctlCheckIoctlAllowed(Ioctl))
    {
        Irp->IoStatus.Status      = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_SUCCESS;
    }

    //
    // Dispatch to the appropriate handler based on the IOCTL range
    //
    IoctlFunction = CTL_CODE_FUNCTION(Ioctl);

    if (IoctlFunction > IOCTL_BASIC_IOCTL && IoctlFunction <= IOCTL_BASIC_IOCTL + 0x100)
    {
        Status = DrvDispatchBasicIoControl(Irp, IrpStack, &DoNotChangeInformation);
    }
    else if (IoctlFunction > IOCTL_KD_IOCTL && IoctlFunction <= IOCTL_KD_IOCTL + 0x100)
    {
        Status = DrvDispatchKdIoControl(Irp, IrpStack, &DoNotChangeInformation);
    }
    else if (IoctlFunction > IOCTL_VMM_IOCTL && IoctlFunction <= IOCTL_VMM_IOCTL + 0x100)
    {
        Status = DrvDispatchVmmIoControl(Irp, IrpStack, &DoNotChangeInformation);
    }
    else if (IoctlFunction > IOCTL_HYPERTRACE_IOCTL && IoctlFunction <= IOCTL_HYPERTRACE_IOCTL + 0x100)
    {
        Status = DrvDispatchHyperTraceIoControl(Irp, IrpStack, &DoNotChangeInformation);
    }
    else
    {
        Status = STATUS_NOT_IMPLEMENTED;
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
