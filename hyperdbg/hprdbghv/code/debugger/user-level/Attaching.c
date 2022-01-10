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
 * @brief Handle the state when it reached to the entrypoint 
 * of the user-mode process 
 * 
 * @param CurrentProcessorIndex 
 * @param GuestRegs 
 * @return VOID 
 */
VOID
AttachingReachedToProcessEntrypoint(UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs)
{
    //
    // Check if we're connect to the kHyperDbg or uHyperDbg
    //
    if (g_KernelDebuggerState)
    {
        //
        // Handling state through the kernel-mode debugger
        //
        KdHandleBreakpointAndDebugBreakpoints(CurrentProcessorIndex,
                                              GuestRegs,
                                              DEBUGGEE_PAUSING_REASON_DEBUGGEE_ENTRY_POINT_REACHED,
                                              NULL);
    }
    else
    {
        //
        // Handling state through the user-mode debugger
        //
        LogInfo("Reached to the entrypoint in user-mode debugging");
    }
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
    // Check to only break on the target process id and thread id and when
    // the entrypoint is not called
    //
    if (g_UsermodeAttachingState.ProcessId == PsGetCurrentProcessId() &&
        g_UsermodeAttachingState.ThreadId == PsGetCurrentThreadId())
    {
        if (g_UsermodeAttachingState.IsWaitingForUserModeModuleEntrypointToBeCalled)
        {
            //
            // Show a message that we reached to the entrypoint
            //
            // Log("Reached to the main module entrypoint (%016llx)\n", g_GuestState[CurrentProcessorIndex].LastVmexitRip);

            //
            // Not waiting for these event anymore
            //
            g_UsermodeAttachingState.IsWaitingForUserModeModuleEntrypointToBeCalled = FALSE;

            //
            // Whenever Windows calls the start entrypoint of the target PE, initially,
            // the module is not loaded in the memory, thus a page-fault will happen and
            // the page-fault handler will bring the module content to the memory and after
            // that, the start entrypoint of PE is called again
            // It's not possible for us to handle everything here without checking for the
            // possible page-fault, it is because the first instruction is not available to be
            // read from the memory, thus, it shows a wrong instruction and the user might not
            // use the instrumentation step-in from the start up of the entrypoint.
            // Because if the user uses the instrumentation step-in command then it should go
            // the page-fault handler routine in the kernel and it's not what the user expects
            // we inject a #PF here and let the Windows handle the page-fault. Meanwhile, we
            // set the hardware debug breakpoint again at the start of the entrypoint, so, the
            // next time that the instruction is about to be fetched, a vm-exit happens as a
            // result of intercepting #DBs
            // In some cases, there is no need to inject #PF to the guest. One example is when
            // the target page is already available in the memory like the same process is also
            // open in the debuggee. In these cases, we handle the break directly without injecting
            // any page-fault
            //

            if (!CheckMemoryAccessSafety(g_UsermodeAttachingState.Entrypoint, sizeof(CHAR)))
            {
                // LogInfo("Injecting #PF for entrypoint at : %llx", g_UsermodeAttachingState.Entrypoint);

                //
                // Inject #PF
                //
                VMEXIT_INTERRUPT_INFO InterruptInfo = {0};

                //
                // We're waiting for this pointer to be called again after handling page-fault
                //
                g_UsermodeAttachingState.IsWaitingForReturnAndRunFromPageFault = TRUE;

                //
                // Configure the #PF injection
                //

                //
                // InterruptExit                 [Type: _VMEXIT_INTERRUPT_INFO]
                //
                // [+0x000 ( 7: 0)] Vector           : 0xe [Type: unsigned int]
                // [+0x000 (10: 8)] InterruptionType : 0x3 [Type: unsigned int]
                // [+0x000 (11:11)] ErrorCodeValid   : 0x1 [Type: unsigned int]
                // [+0x000 (12:12)] NmiUnblocking    : 0x0 [Type: unsigned int]
                // [+0x000 (30:13)] Reserved         : 0x0 [Type: unsigned int]
                // [+0x000 (31:31)] Valid            : 0x1 [Type: unsigned int]
                // [+0x000] Flags                    : 0x80000b0e [Type: unsigned int]
                //
                InterruptInfo.Vector           = EXCEPTION_VECTOR_PAGE_FAULT;
                InterruptInfo.InterruptionType = INTERRUPT_TYPE_HARDWARE_EXCEPTION;
                InterruptInfo.ErrorCodeValid   = TRUE;
                InterruptInfo.NmiUnblocking    = FALSE;
                InterruptInfo.Valid            = TRUE;

                IdtEmulationHandlePageFaults(CurrentProcessorIndex, InterruptInfo, g_UsermodeAttachingState.Entrypoint, 0x14);

                //
                // Re-apply the hw debug reg breakpoint
                //
                DebugRegistersSet(DEBUGGER_DEBUG_REGISTER_FOR_USER_MODE_ENTRY_POINT,
                                  BREAK_ON_INSTRUCTION_FETCH,
                                  FALSE,
                                  g_UsermodeAttachingState.Entrypoint);
            }
            else
            {
                //
                // Address is valid, probably the module is previously loaded
                // or another process with same image is currently running
                // Thus, there is no need to inject #PF, we'll handle it in debugger
                //
                AttachingReachedToProcessEntrypoint(CurrentProcessorIndex, GuestRegs);
            }
        }
        else if (g_UsermodeAttachingState.IsWaitingForReturnAndRunFromPageFault)
        {
            //
            // not waiting for a break after the page-fault anymore
            //
            g_UsermodeAttachingState.IsWaitingForReturnAndRunFromPageFault = FALSE;

            //
            // We reached here as a result of setting the second hardware debug breakpoint
            // and after injecting a page-fault
            //
            AttachingReachedToProcessEntrypoint(CurrentProcessorIndex, GuestRegs);
        }
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
 * @brief Allocate a nop-sled buffer 
 * @param ReservedBuffAddress
 * @param ProcessId
 * 
 * @return BOOLEAN 
 */
BOOLEAN
AttachingAllocateAndAdjustNopSledBuffer(UINT64 ReservedBuffAddress, UINT32 ProcessId)
{
    //
    // Initilize nop-sled page (if not already intialized)
    //
    if (!g_SteppingsNopSledState.IsNopSledInitialized)
    {
        //
        // Allocate memory for nop-slep
        //
        g_SteppingsNopSledState.NopSledVirtualAddress = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOLTAG);

        if (g_SteppingsNopSledState.NopSledVirtualAddress == NULL)
        {
            //
            // There was a problem in allocation
            //
            return FALSE;
        }

        RtlZeroMemory(g_SteppingsNopSledState.NopSledVirtualAddress, PAGE_SIZE);

        //
        // Fill the memory with nops
        //
        memset(g_SteppingsNopSledState.NopSledVirtualAddress, 0x90, PAGE_SIZE);

        //
        // Set jmps to form a loop (little endians)
        //
        // Disassembly of section .text:
        // 0000000000000000 <NopLoop>:
        // 0:  90                      nop
        // 1:  90                      nop
        // 2:  90                      nop
        // 3:  90                      nop
        // 4:  90                      nop
        // 5:  90                      nop
        // 6:  90                      nop
        // 7:  90                      nop
        // 8:  90                      nop
        // 9:  90                      nop
        // a:  eb f4                   jmp    0 <NopLoop>
        //
        *(UINT16 *)(g_SteppingsNopSledState.NopSledVirtualAddress + PAGE_SIZE - 2) = 0xf4eb;

        //
        // Convert the address to virtual address
        //
        g_SteppingsNopSledState.NopSledPhysicalAddress.QuadPart = VirtualAddressToPhysicalAddress(
            g_SteppingsNopSledState.NopSledVirtualAddress);

        //
        // Indicate that it is initialized
        //
        g_SteppingsNopSledState.IsNopSledInitialized = TRUE;
    }

    //
    // Set the entry address in the target process to the target nop-sled
    //
    MemoryMapperMapPhysicalAddressToPte(g_SteppingsNopSledState.NopSledPhysicalAddress,
                                        ReservedBuffAddress,
                                        GetCr3FromProcessId(ProcessId));

    return TRUE;
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
    // Set the Is32Bit flag of attach request
    //
    AttachRequest->Is32Bit = g_UsermodeAttachingState.Is32Bit;

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
    // allocate memory in the target user-mode process
    //
    g_UsermodeAttachingState.UsermodeReservedBuffer =
        MemoryMapperReserveUsermodeAddressInTargetProcess(AttachRequest->ProcessId, FALSE);

    if (g_UsermodeAttachingState.UsermodeReservedBuffer == NULL)
    {
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
        return;
    }

    //
    // Adjust the nop sled buffer
    //
    // if (!AttachingAllocateAndAdjustNopSledBuffer(g_UsermodeAttachingState.UsermodeReservedBuffer,
    //                                              g_UsermodeAttachingState.ProcessId))
    // {
    //     AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
    //     return;
    // }

    //
    // Log for test
    //
    // LogInfo("Reserved address on the target process: %llx\n", g_UsermodeAttachingState.UsermodeReservedBuffer);

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
 * @brief Kill the target process from kernel-mode
 * @details this function should be called in vmx-root
 * 
 * @param KillRequest 
 * @return VOID 
 */
VOID
AttachingKillProcess(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS KillRequest)
{
    BOOLEAN WasKilled = FALSE;

    //
    // Check if process exists or not
    //
    if (!IsProcessExist(KillRequest->ProcessId))
    {
        //
        // Process does not exists
        //
        KillRequest->Result = DEBUGGER_ERROR_INVALID_PROCESS_ID;
        return;
    }

    //
    // Check if we can kill it using the first method
    //
    WasKilled = KillProcess(KillRequest->ProcessId, PROCESS_KILL_METHOD_1);

    if (WasKilled)
    {
        goto Success;
    }

    //
    // Check if we can kill it using the second method
    //
    WasKilled = KillProcess(KillRequest->ProcessId, PROCESS_KILL_METHOD_2);

    if (WasKilled)
    {
        goto Success;
    }

    //
    // Check if we can kill it using the third method
    //
    WasKilled = KillProcess(KillRequest->ProcessId, PROCESS_KILL_METHOD_3);

    if (WasKilled)
    {
        goto Success;
    }

    //
    // No way we can kill the shit :(
    //
    KillRequest->Result = DEBUGGER_ERROR_UNABLE_TO_KILL_THE_PROCESS;
    return;

Success:
    KillRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
}

/**
 * @brief Dispatch and perform attaching tasks
 * @details this function should be called in vmx-root
 * 
 * @param AttachRequest 
 * @return VOID 
 */
VOID
AttachingTargetProcess(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS Request)
{
    switch (Request->Action)
    {
    case DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_ATTACH:

        AttachingSuspendedTargetProcess(Request);

        break;

    case DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_REMOVE_HOOKS:

        AttachingRemoveHooks(Request);

        break;

    case DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_KILL_PROCESS:

        AttachingKillProcess(Request);

        break;

    default:

        Request->Result = DEBUGGER_ERROR_INVALID_ACTION_TYPE;

        break;
    }
}
