/**
 * @file Attaching.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Attaching and detaching for debugging user-mode processes
 * @details 
 *
 * @version 0.1
 * @date 2021-12-28
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "..\hprdbghv\pch.h"

/**
 * @brief Initialize the attaching mechanism
 * 
 * @return BOOLEAN
 */
BOOLEAN
AttachingInitialize()
{
    UNICODE_STRING FunctionName;

    //
    // Find address of PsGetProcessPeb
    //
    if (g_PsGetProcessPeb == NULL)
    {
        RtlInitUnicodeString(&FunctionName, L"PsGetProcessPeb");
        g_PsGetProcessPeb = (PsGetProcessPeb)MmGetSystemRoutineAddress(&FunctionName);

        if (g_PsGetProcessPeb == NULL)
        {
            LogError("Err, cannot resolve PsGetProcessPeb");

            //
            // Won't fail the entire debugger for not finding this
            //
            // return FALSE;
        }
    }

    //
    // Find address of PsGetProcessWow64Process
    //
    if (g_PsGetProcessWow64Process == NULL)
    {
        RtlInitUnicodeString(&FunctionName, L"PsGetProcessWow64Process");
        g_PsGetProcessWow64Process = (PsGetProcessWow64Process)MmGetSystemRoutineAddress(&FunctionName);

        if (g_PsGetProcessWow64Process == NULL)
        {
            LogError("Err, cannot resolve PsGetProcessPeb");

            //
            // Won't fail the entire debugger for not finding this
            //
            // return FALSE;
        }
    }

    //
    // Find address of ZwQueryInformationProcess
    //
    if (g_ZwQueryInformationProcess == NULL)
    {
        UNICODE_STRING RoutineName;

        RtlInitUnicodeString(&RoutineName, L"ZwQueryInformationProcess");

        g_ZwQueryInformationProcess =
            (ZwQueryInformationProcess)MmGetSystemRoutineAddress(&RoutineName);

        if (g_ZwQueryInformationProcess == NULL)
        {
            LogError("Err, cannot resolve ZwQueryInformationProcess");

            //
            // Won't fail the entire debugger for not finding this
            //
            // return FALSE;
        }
    }

    return TRUE;
}

/**
 * @brief Handle debug register event (#DB) for attaching to user-mode process 
 * 
 * @param CurrentProcessorIndex 
 * @param GuestRegs 
 * @return VOID 
 */
VOID
AttachingHandleEntrypointDebugBreak(UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs)
{
    //
    // Not increment the RIP register as no instruction is intended to go
    //
    g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;

    //
    // Check to only break on the target process id and thread id
    //
    if (g_UsermodeAttachingState.ProcessId == PsGetCurrentProcessId() &&
        g_UsermodeAttachingState.ThreadId == PsGetCurrentThreadId())
    {
        //
        // Show a message that we reached to the entrypoint
        //
        Log("Reached to the main module entrypoint (%016llx)\n", g_GuestState[CurrentProcessorIndex].LastVmexitRip);

        //
        // Not waiting for these event anymore
        //
        g_UsermodeAttachingState.IsWaitingForUserModeModuleEntrypointToBeCalled = FALSE;

        //
        // Temporarily handle everything in kernel debugger
        //
        KdHandleDebugEventsWhenKernelDebuggerIsAttached(CurrentProcessorIndex, GuestRegs);
    }
    else
    {
        //
        // Re-apply the hw debug reg breakpoint
        //
        DebugRegistersSet(DEBUGGER_DEBUG_REGISTER_FOR_USER_MODE_ENTRY_POINT,
                          BREAK_ON_INSTRUCTION_FETCH,
                          FALSE,
                          g_UsermodeAttachingState.Entrypoint);
    }
}

/**
 * @brief Attach to the target process
 * @details this function should be called in vmx-root
 * 
 * @param AttachRequest 
 * @return VOID 
 */
VOID
AttachingSuspendedTargetProcess(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS AttachRequest)
{
    PEPROCESS SourceProcess;
    BOOLEAN   ResultOfApplyingEvent;

    if (g_PsGetProcessWow64Process == NULL || g_PsGetProcessPeb == NULL)
    {
        AttachRequest->Result = DEBUGGER_ERROR_FUNCTIONS_FOR_INITIALIZING_PEB_ADDRESSES_ARE_NOT_INITIALIZED;
        return;
    }

    if (PsLookupProcessByProcessId(AttachRequest->ProcessId, &SourceProcess) != STATUS_SUCCESS)
    {
        //
        // if the process not found
        //
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
        return;
    }

    ObDereferenceObject(SourceProcess);

    //
    // check whether the target process is 32-bit or 64-bit
    //
    if (!UserAccessIsWow64Process(AttachRequest->ProcessId, &g_UsermodeAttachingState.Is32Bit))
    {
        //
        // Unable to detect whether it's 32-bit or 64-bit
        //
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_DETECT_32_BIT_OR_64_BIT_PROCESS;
        return;
    }

    //
    // Get 32-bit or 64-bit PEB
    //
    if (g_UsermodeAttachingState.Is32Bit)
    {
        g_UsermodeAttachingState.PebAddressToMonitor = (PPEB32)g_PsGetProcessWow64Process(SourceProcess);
    }
    else
    {
        g_UsermodeAttachingState.PebAddressToMonitor = (PPEB)g_PsGetProcessPeb(SourceProcess);
    }

    if (g_UsermodeAttachingState.PebAddressToMonitor == NULL)
    {
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
        return;
    }

    //
    // Set process details
    //
    g_UsermodeAttachingState.ProcessId = AttachRequest->ProcessId;
    g_UsermodeAttachingState.ThreadId  = AttachRequest->ThreadId;

    //
    // Waiting for module to be loaded anymore
    //
    g_UsermodeAttachingState.IsWaitingForUserModeModuleEntrypointToBeCalled = TRUE;

    //
    // Enable vm-exit on Hardware debug exceptions and breakpoints
    // so, intercept #DBs and #BP by changing exception bitmap (one core)
    //
    BroadcastEnableDbAndBpExitingAllCores();

    //
    // Apply monitor memory range to the PEB address
    //
    ResultOfApplyingEvent =
        DebuggerEventEnableMonitorReadAndWriteForAddress(
            g_UsermodeAttachingState.PebAddressToMonitor,
            AttachRequest->ProcessId,
            TRUE,
            TRUE);

    if (!ResultOfApplyingEvent)
    {
        g_UsermodeAttachingState.IsWaitingForUserModeModuleEntrypointToBeCalled = FALSE;
        g_UsermodeAttachingState.PebAddressToMonitor                            = NULL;

        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
        return;
    }

    AttachRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
}

/**
 * @brief Clearing hooks after resuming the process
 * @details this function should be called in vmx-root
 * 
 * @param AttachRequest 
 * @return VOID 
 */
VOID
AttachingRemoveHooks(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS AttachRequest)
{
    //
    // Check if the entrypoint is reached or not,
    // if it's not reached then we won't remove the hooks
    //
    if (!g_UsermodeAttachingState.IsWaitingForUserModeModuleEntrypointToBeCalled)
    {
        //
        // The entrypoint is called, we should remove the hook
        //
        if (!EptHookUnHookSingleAddress(g_UsermodeAttachingState.PebAddressToMonitor,
                                        NULL,
                                        g_UsermodeAttachingState.ProcessId))
        {
            AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_REMOVE_HOOKS;
            return;
        }
        else
        {
            //
            // The unhooking operation was successful
            //
            AttachRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
        }
    }
    else
    {
        //
        // The entrypoint is not called, we shouldn't remove the hook
        //
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_REMOVE_HOOKS_ENTRYPOINT_NOT_REACHED;
        return;
    }
}

/**
 * @brief Dispatch and perform attaching tasks
 * @details this function should be called in vmx-root
 * 
 * @param AttachRequest 
 * @return VOID 
 */
VOID
AttachingTargetProcess(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS AttachRequest)
{
    switch (AttachRequest->Action)
    {
    case DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_ATTACH:

        AttachingSuspendedTargetProcess(AttachRequest);

        break;

    case DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_REMOVE_HOOKS:

        AttachingRemoveHooks(AttachRequest);

        break;

    default:

        AttachRequest->Result = DEBUGGER_ERROR_INVALID_ACTION_TYPE;

        break;
    }
}
