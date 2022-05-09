/**
 * @file Attaching.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Attaching and detaching for debugging user-mode processes
 * @details 
 *
 * @version 0.1
 * @date 2021-12-28
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief Initialize the attaching mechanism
 * @details as we use the functionalities for these functions, we initialize
 * them from the debugger at start up of debugger (not initialization of user-debugger)
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

        g_ZwQueryInformationProcess = (ZwQueryInformationProcess)MmGetSystemRoutineAddress(&RoutineName);

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
 * @brief Create user-mode debugging details for threads 
 * 
 * @param ProcessId 
 * @param Is32Bit 
 * @param Eprocess 
 * @param PebAddressToMonitor 
 * @param UsermodeReservedBuffer 
 * @return UINT64 returns the unique token 
 */
UINT64
AttachingCreateProcessDebuggingDetails(UINT32    ProcessId,
                                       BOOLEAN   Enabled,
                                       BOOLEAN   Is32Bit,
                                       PEPROCESS Eprocess,
                                       UINT64    PebAddressToMonitor,
                                       UINT64    UsermodeReservedBuffer)
{
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail;

    //
    // Allocate the buffer
    //
    ProcessDebuggingDetail = (PUSERMODE_DEBUGGING_PROCESS_DETAILS)
        ExAllocatePoolWithTag(NonPagedPool, sizeof(USERMODE_DEBUGGING_PROCESS_DETAILS), POOLTAG);

    if (!ProcessDebuggingDetail)
    {
        return NULL;
    }

    RtlZeroMemory(ProcessDebuggingDetail, sizeof(USERMODE_DEBUGGING_PROCESS_DETAILS));

    //
    // Set the unique tag and increment it
    //
    ProcessDebuggingDetail->Token = g_SeedOfUserDebuggingDetails++;

    //
    // Set the details of the created buffer
    //
    ProcessDebuggingDetail->ProcessId              = ProcessId;
    ProcessDebuggingDetail->Enabled                = Enabled;
    ProcessDebuggingDetail->Is32Bit                = Is32Bit;
    ProcessDebuggingDetail->Eprocess               = Eprocess;
    ProcessDebuggingDetail->PebAddressToMonitor    = PebAddressToMonitor;
    ProcessDebuggingDetail->UsermodeReservedBuffer = UsermodeReservedBuffer;

    //
    // Allocate a thread holder buffer for this process
    //
    if (!ThreadHolderAssignThreadHolderToProcessDebuggingDetails(ProcessDebuggingDetail))
    {
        ExFreePoolWithTag(ProcessDebuggingDetail, POOLTAG);
        return NULL;
    }

    //
    // Attach it to the list of active thread (LIST_ENTRY)
    //
    InsertHeadList(&g_ProcessDebuggingDetailsListHead, &(ProcessDebuggingDetail->AttachedProcessList));

    //
    // return the token
    //
    return ProcessDebuggingDetail->Token;
}

/**
 * @brief Find user-mode debugging details for threads by token
 * 
 * @param Token 
 * @return PUSERMODE_DEBUGGING_PROCESS_DETAILS 
 */
PUSERMODE_DEBUGGING_PROCESS_DETAILS
AttachingFindProcessDebuggingDetailsByToken(UINT64 Token)
{
    LIST_FOR_EACH_LINK(g_ProcessDebuggingDetailsListHead, USERMODE_DEBUGGING_PROCESS_DETAILS, AttachedProcessList, ProcessDebuggingDetails)
    {
        //
        // Check if we found the target thread and if it's enabled
        //
        if (ProcessDebuggingDetails->Token == Token && ProcessDebuggingDetails->Enabled)
        {
            return ProcessDebuggingDetails;
        }
    }

    return NULL;
}

/**
 * @brief Find user-mode debugging details for threads by process Id
 * 
 * @param ProcessId 
 * @return PUSERMODE_DEBUGGING_PROCESS_DETAILS 
 */
PUSERMODE_DEBUGGING_PROCESS_DETAILS
AttachingFindProcessDebuggingDetailsByProcessId(UINT32 ProcessId)
{
    LIST_FOR_EACH_LINK(g_ProcessDebuggingDetailsListHead, USERMODE_DEBUGGING_PROCESS_DETAILS, AttachedProcessList, ProcessDebuggingDetails)
    {
        //
        // Check if we found the target thread and if it's enabled
        //
        if (ProcessDebuggingDetails->ProcessId == ProcessId && ProcessDebuggingDetails->Enabled)
        {
            return ProcessDebuggingDetails;
        }
    }

    return NULL;
}

/**
 * @brief Find user-mode debugging details for threads that is in
 * the start-up phase
 * 
 * @return PUSERMODE_DEBUGGING_PROCESS_DETAILS 
 */
PUSERMODE_DEBUGGING_PROCESS_DETAILS
AttachingFindProcessDebuggingDetailsInStartingPhase()
{
    LIST_FOR_EACH_LINK(g_ProcessDebuggingDetailsListHead, USERMODE_DEBUGGING_PROCESS_DETAILS, AttachedProcessList, ProcessDebuggingDetails)
    {
        if (ProcessDebuggingDetails->IsOnTheStartingPhase)
        {
            return ProcessDebuggingDetails;
        }
    }

    return NULL;
}

/**
 * @brief Remove and deallocate all thread debuggig details
 * 
 * @return VOID 
 */
VOID
AttachingRemoveAndFreeAllProcessDebuggingDetails()
{
    LIST_FOR_EACH_LINK(g_ProcessDebuggingDetailsListHead, USERMODE_DEBUGGING_PROCESS_DETAILS, AttachedProcessList, ProcessDebuggingDetails)
    {
        //
        // Free the thread holding structure(s)
        //
        ThreadHolderFreeHoldingStructures(ProcessDebuggingDetails);

        //
        // Remove thread debugging detail from the list active threads
        //
        RemoveEntryList(&ProcessDebuggingDetails->AttachedProcessList);

        //
        // Unallocate the pool
        //
        ExFreePoolWithTag(ProcessDebuggingDetails, POOLTAG);
    }
}

/**
 * @brief Remove user-mode debugging details for threads by its token
 * 
 * @param Token 
 * @return BOOLEAN 
 */
BOOLEAN
AttachingRemoveProcessDebuggingDetailsByToken(UINT64 Token)
{
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetails;

    //
    // Find the entry
    //
    ProcessDebuggingDetails = AttachingFindProcessDebuggingDetailsByToken(Token);

    if (!ProcessDebuggingDetails)
    {
        //
        // Token not found!
        //
        return FALSE;
    }

    //
    // Free the thread holding structure(s)
    //
    ThreadHolderFreeHoldingStructures(ProcessDebuggingDetails);

    //
    // Remove thread debugging detail from the list active threads
    //
    RemoveEntryList(&ProcessDebuggingDetails->AttachedProcessList);

    //
    // Unallocate the pool
    //
    ExFreePoolWithTag(ProcessDebuggingDetails, POOLTAG);

    return TRUE;
}

/**
 * @brief Set the start up phase of a debugging thread buffer by its token
 * 
 * @param Set 
 * @param Token 
 * @return BOOLEAN 
 */
BOOLEAN
AttachingSetStartingPhaseOfProcessDebuggingDetailsByToken(BOOLEAN Set, UINT64 Token)
{
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetails;

    //
    // If it's set to TRUE, then we check to only put 1 thread at the time
    // to the starting phase
    //
    ProcessDebuggingDetails = AttachingFindProcessDebuggingDetailsInStartingPhase();

    if (Set && ProcessDebuggingDetails != NULL)
    {
        //
        // There is another thread in starting phase!
        //
        return FALSE;
    }

    //
    // Find the entry
    //
    ProcessDebuggingDetails = AttachingFindProcessDebuggingDetailsByToken(Token);

    if (!ProcessDebuggingDetails)
    {
        //
        // Token not found!
        //
        return FALSE;
    }

    //
    // Set the starting phase
    //
    ProcessDebuggingDetails->IsOnTheStartingPhase = Set;

    return TRUE;
}

/**
 * @brief Handle the state when it reached to the entrypoint 
 * of the user-mode process 
 * 
 * @param CurrentProcessorIndex 
 * @param GuestRegs 
 * @param ThreadDebuggingToken 
 * @return VOID 
 */
VOID
AttachingReachedToProcessEntrypoint(UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs, UINT64 ThreadDebuggingToken)
{
    //
    // Finish the starting point of the thread
    //
    AttachingSetStartingPhaseOfProcessDebuggingDetailsByToken(FALSE, ThreadDebuggingToken);

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
        UdCheckAndHandleBreakpointsAndDebugBreaks(CurrentProcessorIndex,
                                                  GuestRegs,
                                                  DEBUGGEE_PAUSING_REASON_DEBUGGEE_ENTRY_POINT_REACHED,
                                                  NULL);
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
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail = NULL;

    //
    // Not increment the RIP register as no instruction is intended to go
    //
    g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;

    ProcessDebuggingDetail = AttachingFindProcessDebuggingDetailsByProcessId(PsGetCurrentProcessId());
    //
    // Check to only break on the target process id and thread id and when
    // the entrypoint is not called, if the thread debugging detail is found
    //
    if (ProcessDebuggingDetail != NULL)
    {
        if (g_IsWaitingForUserModeModuleEntrypointToBeCalled)
        {
            //
            // Show a message that we reached to the entrypoint
            //
            // Log("Reached to the main module entrypoint (%016llx)\n", g_GuestState[CurrentProcessorIndex].LastVmexitRip);

            //
            // Not waiting for these event anymore
            //
            g_IsWaitingForUserModeModuleEntrypointToBeCalled = FALSE;

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

            if (!CheckMemoryAccessSafety(ProcessDebuggingDetail->EntrypointOfMainModule, sizeof(CHAR)))
            {
                // LogInfo("Injecting #PF for entrypoint at : %llx", ProcessDebuggingDetail->EntrypointOfMainModule);

                //
                // Inject #PF
                //
                VMEXIT_INTERRUPT_INFORMATION InterruptInfo = {0};

                //
                // We're waiting for this pointer to be called again after handling page-fault
                //
                g_IsWaitingForReturnAndRunFromPageFault = TRUE;

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

                IdtEmulationHandlePageFaults(CurrentProcessorIndex, InterruptInfo, ProcessDebuggingDetail->EntrypointOfMainModule, 0x14);

                //
                // Re-apply the hw debug reg breakpoint
                //
                DebugRegistersSet(DEBUGGER_DEBUG_REGISTER_FOR_USER_MODE_ENTRY_POINT,
                                  BREAK_ON_INSTRUCTION_FETCH,
                                  FALSE,
                                  ProcessDebuggingDetail->EntrypointOfMainModule);
            }
            else
            {
                //
                // Address is valid, probably the module is previously loaded
                // or another process with same image is currently running
                // Thus, there is no need to inject #PF, we'll handle it in debugger
                //
                AttachingReachedToProcessEntrypoint(CurrentProcessorIndex, GuestRegs, ProcessDebuggingDetail->Token);
            }
        }
        else if (g_IsWaitingForReturnAndRunFromPageFault)
        {
            //
            // not waiting for a break after the page-fault anymore
            //
            g_IsWaitingForReturnAndRunFromPageFault = FALSE;

            //
            // We reached here as a result of setting the second hardware debug breakpoint
            // and after injecting a page-fault
            //
            AttachingReachedToProcessEntrypoint(CurrentProcessorIndex, GuestRegs, ProcessDebuggingDetail->Token);
        }
    }
    else
    {
        //
        // Check if we can find any thread in start up phase, if yes, we'll apply
        // it to the entrypoint
        //
        ProcessDebuggingDetail = AttachingFindProcessDebuggingDetailsInStartingPhase();

        if (ProcessDebuggingDetail != NULL)
        {
            //
            // Re-apply the hw debug reg breakpoint
            //
            DebugRegistersSet(DEBUGGER_DEBUG_REGISTER_FOR_USER_MODE_ENTRY_POINT,
                              BREAK_ON_INSTRUCTION_FETCH,
                              FALSE,
                              ProcessDebuggingDetail->EntrypointOfMainModule);
        }
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
AttachingAdjustNopSledBuffer(UINT64 ReservedBuffAddress, UINT32 ProcessId)
{
    PEPROCESS  SourceProcess;
    KAPC_STATE State = {0};

    if (PsLookupProcessByProcessId(ProcessId, &SourceProcess) != STATUS_SUCCESS)
    {
        //
        // if the process not found
        //
        return FALSE;
    }

    __try
    {
        KeStackAttachProcess(SourceProcess, &State);

        //
        // Fill the memory with nops
        //
        memset(ReservedBuffAddress, 0x90, PAGE_SIZE);

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
        // 8:  0f a2                   cpuid
        // a:  eb f4                   jmp    0 <NopLoop>
        //
        *(UINT16 *)(ReservedBuffAddress + PAGE_SIZE - 4) = 0xa20f;
        *(UINT16 *)(ReservedBuffAddress + PAGE_SIZE - 2) = 0xf4eb;

        KeUnstackDetachProcess(&State);

        ObDereferenceObject(SourceProcess);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KeUnstackDetachProcess(&State);

        return NULL;
    }

    return TRUE;
}

/**
 * @brief Check page-faults with user-debugger
 * @param CurrentProcessorIndex
 * @param InterruptExit
 * @param Address
 * @param ErrorCode
 * 
 * @return BOOLEAN if TRUE show that the page-fault injection should be ignored
 */
BOOLEAN
AttachingCheckPageFaultsWithUserDebugger(UINT32                       CurrentProcessorIndex,
                                         PGUEST_REGS                  GuestRegs,
                                         VMEXIT_INTERRUPT_INFORMATION InterruptExit,
                                         UINT64                       Address,
                                         ULONG                        ErrorCode)
{
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail;
    VIRTUAL_MACHINE_STATE *             CurrentVmState = &g_GuestState[CurrentProcessorIndex];
    //
    // Check if thread is in user-mode
    //
    if (CurrentVmState->LastVmexitRip & 0xf000000000000000)
    {
        //
        // We won't intercept threads in kernel-mode
        //
        return FALSE;
    }

    ProcessDebuggingDetail = AttachingFindProcessDebuggingDetailsByProcessId(PsGetCurrentProcessId());

    if (!ProcessDebuggingDetail)
    {
        //
        // not related to user debugger
        //
        return FALSE;
    }

    //
    // Check if thread is in intercepting phase
    //
    if (!ProcessDebuggingDetail->IsOnThreadInterceptingPhase)
    {
        //
        // this thread is not in intercepting phase
        //
        return FALSE;
    }

    //
    // Handling state through the user-mode debugger
    //
    UdCheckAndHandleBreakpointsAndDebugBreaks(CurrentProcessorIndex,
                                              GuestRegs,
                                              DEBUGGEE_PAUSING_REASON_DEBUGGEE_GENERAL_THREAD_INTERCEPTED,
                                              NULL);

    //
    // related to user debugger
    //
    CurrentVmState->IncrementRip = FALSE;

    return TRUE;
}

/**
 * @brief Enable or disable the thread intercepting phase
 * @details this function should be called in vmx non-root
 * 
 * @param ProcessDebuggingToken 
 * @param Enable 
 * @return BOOLEAN 
 */
BOOLEAN
AttachingConfigureInterceptingThreads(UINT64 ProcessDebuggingToken, BOOLEAN Enable)
{
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail;

    //
    // Get the current process debugging detail
    //
    ProcessDebuggingDetail = AttachingFindProcessDebuggingDetailsByToken(ProcessDebuggingToken);

    if (!ProcessDebuggingDetail)
    {
        return FALSE;
    }

    //
    // If the thread is already in intercepting phase, we should return,
    //
    if (Enable && ProcessDebuggingDetail->IsOnThreadInterceptingPhase)
    {
        LogError("Err, thread is already in intercepting phase");
        return FALSE;
    }

    //
    // if the user want to disable the intercepting phase, we just ignore the
    // request without a message
    //
    if (!Enable && !ProcessDebuggingDetail->IsOnThreadInterceptingPhase)
    {
        return FALSE;
    }

    //
    // Indicate that the future #PFs should or should not be checked with user debugger
    //
    g_CheckPageFaultsAndMov2Cr3VmexitsWithUserDebugger = Enable;

    //
    // We're or we're not in thread intercepting phase now
    //
    ProcessDebuggingDetail->IsOnThreadInterceptingPhase = Enable;

    if (Enable)
    {
        //
        // Intercept all mov 2 cr3s
        //
        DebuggerEventEnableMovToCr3ExitingOnAllProcessors();
    }
    else
    {
        //
        // Removing the mov to cr3 vm-exits
        //
        DebuggerEventDisableMovToCr3ExitingOnAllProcessors();
    }

    //
    // Set the supervisor bit to 1 when we want to continue all the threads
    //
    if (!Enable)
    {
        for (size_t i = 0; i < MAX_CR3_IN_A_PROCESS; i++)
        {
            if (ProcessDebuggingDetail->InterceptedCr3[i].Flags != NULL)
            {
                //
                // This cr3 should not be intercepted on threads' user mode execution
                //
                if (!MemoryMapperSetSupervisorBitWithoutSwitchingByCr3(NULL,
                                                                       TRUE,
                                                                       PagingLevelPageMapLevel4,
                                                                       ProcessDebuggingDetail->InterceptedCr3[i]))
                {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

/**
 * @brief Attach to the target process
 * @details this function should not be called in vmx-root
 * 
 * @param AttachRequest 
 * @param IsAttachingToEntrypoint 
 * @return BOOLEAN 
 */
BOOLEAN
AttachingPerformAttachToProcess(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS AttachRequest, BOOLEAN IsAttachingToEntrypoint)
{
    PEPROCESS                           SourceProcess;
    UINT64                              ProcessDebuggingToken;
    UINT64                              PebAddressToMonitor;
    UINT64                              UsermodeReservedBuffer;
    BOOLEAN                             ResultOfApplyingEvent;
    BOOLEAN                             Is32Bit;
    PUSERMODE_DEBUGGING_PROCESS_DETAILS TempProcessDebuggingDetail;

    if (g_PsGetProcessWow64Process == NULL || g_PsGetProcessPeb == NULL)
    {
        AttachRequest->Result = DEBUGGER_ERROR_FUNCTIONS_FOR_INITIALIZING_PEB_ADDRESSES_ARE_NOT_INITIALIZED;
        return FALSE;
    }

    if (PsLookupProcessByProcessId(AttachRequest->ProcessId, &SourceProcess) != STATUS_SUCCESS)
    {
        //
        // if the process not found
        //
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
        return FALSE;
    }

    ObDereferenceObject(SourceProcess);

    //
    // Check to avoid double attaching to a process
    //
    TempProcessDebuggingDetail = AttachingFindProcessDebuggingDetailsByProcessId(AttachRequest->ProcessId);

    if (TempProcessDebuggingDetail != NULL &&
        TempProcessDebuggingDetail->Eprocess == SourceProcess)
    {
        //
        // The user already attached to this process
        //
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_AN_ALREADY_ATTACHED_PROCESS;
        return FALSE;
    }

    //
    // check whether the target process is 32-bit or 64-bit
    //
    if (!UserAccessIsWow64Process(AttachRequest->ProcessId, &Is32Bit))
    {
        //
        // Unable to detect whether it's 32-bit or 64-bit
        //
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_DETECT_32_BIT_OR_64_BIT_PROCESS;
        return FALSE;
    }

    //
    // Set the Is32Bit flag of attach request
    //
    AttachRequest->Is32Bit = Is32Bit;

    //
    // Get 32-bit or 64-bit PEB
    //
    if (Is32Bit)
    {
        PebAddressToMonitor = (PPEB32)g_PsGetProcessWow64Process(SourceProcess);
    }
    else
    {
        PebAddressToMonitor = (PPEB)g_PsGetProcessPeb(SourceProcess);
    }

    if (PebAddressToMonitor == NULL)
    {
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
        return FALSE;
    }

    //
    // allocate memory in the target user-mode process
    //
    UsermodeReservedBuffer = MemoryMapperReserveUsermodeAddressInTargetProcess(AttachRequest->ProcessId, TRUE);

    if (UsermodeReservedBuffer == NULL)
    {
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
        return FALSE;
    }

    //
    // Adjust the nop sled buffer
    //
    if (!AttachingAdjustNopSledBuffer(UsermodeReservedBuffer,
                                      AttachRequest->ProcessId))
    {
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
        return FALSE;
    }

    //
    // Log for test
    //
    // LogInfo("Reserved address on the target process: %llx\n", UsermodeReservedBuffer);

    //
    // Create the debugging detail for process
    //
    ProcessDebuggingToken = AttachingCreateProcessDebuggingDetails(AttachRequest->ProcessId,
                                                                   TRUE,
                                                                   Is32Bit,
                                                                   SourceProcess,
                                                                   PebAddressToMonitor,
                                                                   UsermodeReservedBuffer);

    //
    // Check if we successfully get the token
    //
    if (ProcessDebuggingToken == NULL)
    {
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
        return FALSE;
    }

    //
    // Check if it's attaching at entrypoint or attaching to a previously running process
    //
    if (IsAttachingToEntrypoint)
    {
        //
        // *** attaching to the entrypoint of the process ***
        //

        //
        // Waiting for #DB to be triggered
        //
        g_IsWaitingForUserModeModuleEntrypointToBeCalled = TRUE;

        //
        // Set the starting point of the thread
        //
        if (!AttachingSetStartingPhaseOfProcessDebuggingDetailsByToken(TRUE, ProcessDebuggingToken))
        {
            //
            // Remove the created thread debugging detail
            //
            AttachingRemoveProcessDebuggingDetailsByToken(ProcessDebuggingToken);

            g_IsWaitingForUserModeModuleEntrypointToBeCalled = FALSE;
            AttachRequest->Result                            = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
            return FALSE;
        }

        //
        // Apply monitor memory range to the PEB address
        //
        ResultOfApplyingEvent = DebuggerEventEnableMonitorReadAndWriteForAddress(
            PebAddressToMonitor,
            AttachRequest->ProcessId,
            TRUE,
            TRUE);

        if (!ResultOfApplyingEvent)
        {
            //
            // Remove the created thread debugging detail
            //
            AttachingRemoveProcessDebuggingDetailsByToken(ProcessDebuggingToken);

            g_IsWaitingForUserModeModuleEntrypointToBeCalled = FALSE;
            AttachRequest->Result                            = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
            return FALSE;
        }

        //
        // Operation was successful
        //
        AttachRequest->Token  = ProcessDebuggingToken;
        AttachRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
        return TRUE;
    }
    else
    {
        //
        // *** attaching to previously running process ***
        //
        if (!AttachingConfigureInterceptingThreads(ProcessDebuggingToken, TRUE))
        {
            //
            // Remove the created thread debugging details
            //
            AttachingRemoveProcessDebuggingDetailsByToken(ProcessDebuggingToken);

            AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
            return FALSE;
        }

        //
        // Operation was successful
        //
        AttachRequest->Token  = ProcessDebuggingToken;
        AttachRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
        return TRUE;
    }
}

/**
 * @brief Handle the cr3 vm-exits for thread interception
 * @details this function should be called in vmx-root
 * 
 * @param CurrentCoreIndex 
 * @param NewCr3 
 * @return BOOLEAN 
 */
BOOLEAN
AttachingHandleCr3VmexitsForThreadInterception(UINT32 CurrentCoreIndex, CR3_TYPE NewCr3)
{
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail;

    ProcessDebuggingDetail = AttachingFindProcessDebuggingDetailsByProcessId(PsGetCurrentProcessId());

    //
    // Check if process is valid or if thread is in intercepting phase
    //
    if (!ProcessDebuggingDetail || !ProcessDebuggingDetail->IsOnThreadInterceptingPhase)
    {
        //
        // not related to user debugger
        //
        HvUnsetExceptionBitmap(EXCEPTION_VECTOR_PAGE_FAULT);
        return FALSE;
    }

    //
    // Save the cr3 for future continuing the thread
    //
    for (size_t i = 0; i < MAX_CR3_IN_A_PROCESS; i++)
    {
        if (ProcessDebuggingDetail->InterceptedCr3[i].Flags == NewCr3.Flags)
        {
            //
            // We found it saved previously, no need any further action
            //
            break;
        }

        if (ProcessDebuggingDetail->InterceptedCr3[i].Flags == NULL)
        {
            //
            // Save the cr3
            //
            ProcessDebuggingDetail->InterceptedCr3[i].Flags = NewCr3.Flags;
            break;
        }
    }

    //
    // This thread should be intercepted
    //
    if (!MemoryMapperSetSupervisorBitWithoutSwitchingByCr3(NULL, FALSE, PagingLevelPageMapLevel4, NewCr3))
    {
        return FALSE;
    }

    //
    // Intercept #PFs
    //
    HvSetExceptionBitmap(EXCEPTION_VECTOR_PAGE_FAULT);

    return TRUE;
}

/**
 * @brief Clearing hooks after resuming the process
 * @details this function should not be called in vmx-root
 * 
 * @param AttachRequest 
 * @return BOOLEAN 
 */
BOOLEAN
AttachingRemoveHooks(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS AttachRequest)
{
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetails;

    //
    // Get the thread debugging detail
    //
    ProcessDebuggingDetails = AttachingFindProcessDebuggingDetailsByToken(AttachRequest->Token);

    //
    // Check if token is valid or not
    //
    if (!ProcessDebuggingDetails)
    {
        AttachRequest->Result = DEBUGGER_ERROR_INVALID_THREAD_DEBUGGING_TOKEN;
        return FALSE;
    }

    //
    // Check if the entrypoint is reached or not,
    // if it's not reached then we won't remove the hooks
    //
    if (!g_IsWaitingForUserModeModuleEntrypointToBeCalled)
    {
        //
        // The entrypoint is called, we should remove the hook
        //
        if (!EptHookUnHookSingleAddress(ProcessDebuggingDetails->PebAddressToMonitor,
                                        NULL,
                                        ProcessDebuggingDetails->ProcessId))
        {
            AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_REMOVE_HOOKS;
            return FALSE;
        }
        else
        {
            //
            // The unhooking operation was successful
            //
            AttachRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
            return TRUE;
        }
    }
    else
    {
        //
        // The entrypoint is not called, we shouldn't remove the hook
        //
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_REMOVE_HOOKS_ENTRYPOINT_NOT_REACHED;
        return FALSE;
    }
}

/**
 * @brief Pauses the target process 
 * @details this function should not be called in vmx-root
 * 
 * @param PauseRequest 
 * @return BOOLEAN 
 */
BOOLEAN
AttachingPauseProcess(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS PauseRequest)
{
    if (AttachingConfigureInterceptingThreads(PauseRequest->Token, TRUE))
    {
        //
        // The pausing operation was successful
        //
        PauseRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
        return TRUE;
    }
    else
    {
        PauseRequest->Result = DEBUGGER_ERROR_UNABLE_TO_PAUSE_THE_PROCESS_THREADS;
        return FALSE;
    }
}

/**
 * @brief Kill the target process from kernel-mode
 * @details this function should not be called in vmx-root
 * 
 * @param KillRequest 
 * @return BOOLEAN 
 */
BOOLEAN
AttachingKillProcess(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS KillRequest)
{
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail;
    BOOLEAN                             WasKilled = FALSE;

    ProcessDebuggingDetail = AttachingFindProcessDebuggingDetailsByProcessId(KillRequest->ProcessId);

    //
    // Check if process is valid in process debugging details
    //
    if (!ProcessDebuggingDetail)
    {
        //
        // not found
        //
        KillRequest->Result = DEBUGGER_ERROR_THE_USER_DEBUGGER_NOT_ATTACHED_TO_THE_PROCESS;
        return FALSE;
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
    return FALSE;

Success:

    //
    // Remove the entry from the process debugging details list
    //
    AttachingRemoveProcessDebuggingDetailsByToken(ProcessDebuggingDetail->Token);

    KillRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
    return TRUE;
}

/**
 * @brief Clearing hooks after resuming the process
 * @details this function should not be called in vmx-root
 * 
 * @param DetachRequest 
 * @return BOOLEAN 
 */
BOOLEAN
AttachingPerformDetach(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS DetachRequest)
{
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail;

    ProcessDebuggingDetail = AttachingFindProcessDebuggingDetailsByProcessId(DetachRequest->ProcessId);

    //
    // Check if process is valid in process debugging details
    //
    if (!ProcessDebuggingDetail)
    {
        //
        // not found
        //
        DetachRequest->Result = DEBUGGER_ERROR_THE_USER_DEBUGGER_NOT_ATTACHED_TO_THE_PROCESS;
        return FALSE;
    }

    //
    // If the threads are paused, we can't detach from the target process
    // before sending this request, the debuggger should continued all the
    // threads by sending a continue packet
    //
    if (ThreadHolderIsAnyPausedThreadInProcess(ProcessDebuggingDetail))
    {
        //
        // We found a thread that is still pause
        //
        DetachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_DETACH_AS_THERE_ARE_PAUSED_THREADS;
        return FALSE;
    }

    //
    // Free the reserved memory in the target process
    //
    if (!MemoryMapperFreeMemoryInTargetProcess(DetachRequest->ProcessId,
                                               ProcessDebuggingDetail->UsermodeReservedBuffer))
    {
        //
        // Still, we continue, no need to abort the operation
        //
        LogError("Err, cannot deallocate reserved buffer in the detached process");
    }

    //
    // Remove the entry from the process debugging details list
    //
    AttachingRemoveProcessDebuggingDetailsByToken(ProcessDebuggingDetail->Token);

    DetachRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
    return TRUE;
}

/**
 * @brief Switch to the target thread
 * 
 * @param SwitchRequest 
 * @return BOOLEAN 
 */
BOOLEAN
AttachingSwitchProcess(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS SwitchRequest)
{
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail;
    PUSERMODE_DEBUGGING_THREAD_DETAILS  ThreadDebuggingDetail;

    if (SwitchRequest->ProcessId != NULL)
    {
        //
        // Switch by process id
        //
        ProcessDebuggingDetail = AttachingFindProcessDebuggingDetailsByProcessId(SwitchRequest->ProcessId);
    }
    else if (SwitchRequest->ThreadId != NULL)
    {
        //
        // Switch by thread id
        //
        ProcessDebuggingDetail = ThreadHolderGetProcessDebuggingDetailsByThreadId(SwitchRequest->ThreadId);
    }
    else
    {
        SwitchRequest->Result = DEBUGGER_ERROR_UNABLE_TO_SWITCH_PROCESS_ID_OR_THREAD_ID_IS_INVALID;
        return FALSE;
    }

    //
    // Check if process is valid in process debugging details
    //
    if (!ProcessDebuggingDetail)
    {
        //
        // not found
        //
        SwitchRequest->Result = DEBUGGER_ERROR_UNABLE_TO_SWITCH_PROCESS_ID_OR_THREAD_ID_IS_INVALID;
        return FALSE;
    }

    //
    // Set the IsPaused field
    //
    if (SwitchRequest->ThreadId != NULL)
    {
        //
        // Find the thread's state
        //
        ThreadDebuggingDetail = ThreadHolderGetProcessThreadDetailsByProcessIdAndThreadId(ProcessDebuggingDetail->ProcessId,
                                                                                          SwitchRequest->ThreadId);
    }
    else
    {
        //
        // Find the first thread in the process
        //
        ThreadDebuggingDetail = ThreadHolderGetProcessFirstThreadDetailsByProcessId(ProcessDebuggingDetail->ProcessId);
    }

    //
    // Check if we find the target thread
    //
    if (!ThreadDebuggingDetail)
    {
        //
        // not found
        //
        SwitchRequest->Result = DEBUGGER_ERROR_UNABLE_TO_SWITCH_THERE_IS_NO_THREAD_ON_THE_PROCESS;
        return FALSE;
    }

    //
    // Fill the needed details by user mode
    //
    SwitchRequest->Token     = ProcessDebuggingDetail->Token;
    SwitchRequest->ProcessId = ProcessDebuggingDetail->ProcessId;
    SwitchRequest->ThreadId  = ThreadDebuggingDetail->ThreadId;
    SwitchRequest->Is32Bit   = ProcessDebuggingDetail->Is32Bit;
    SwitchRequest->IsPaused  = ThreadDebuggingDetail->IsPaused;

    SwitchRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
    return TRUE;
}

/**
 * @brief Query count of active debugging threads
 * 
 * @param QueryCountOfDebugThreadsRequest 
 * @return BOOLEAN 
 */
BOOLEAN
AttachingQueryCountOfActiveDebuggingThreadsAndProcesses(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS QueryCountOfDebugThreadsRequest)
{
    UINT32 CountOfProcessesAndThreads;

    //
    // Count the result
    //
    CountOfProcessesAndThreads = ThreadHolderQueryCountOfActiveDebuggingThreadsAndProcesses();

    //
    // Set the results
    //
    QueryCountOfDebugThreadsRequest->CountOfActiveDebuggingThreadsAndProcesses = CountOfProcessesAndThreads;
    QueryCountOfDebugThreadsRequest->Result                                    = DEBUGGER_OPERATION_WAS_SUCCESSFULL;

    return TRUE;
}

/**
 * @brief Query details of active debugging threads
 * 
 * @param BufferToStoreDetails 
 * @param BufferSize 
 * @return BOOLEAN 
 */
BOOLEAN
AttachingQueryDetailsOfActiveDebuggingThreadsAndProcesses(PVOID BufferToStoreDetails, UINT32 BufferSize)
{
    UINT32 CountOfProcessesAndThreadsToStore;

    CountOfProcessesAndThreadsToStore = BufferSize / SIZEOF_USERMODE_DEBUGGING_THREAD_OR_PROCESS_STATE_DETAILS;

    if (CountOfProcessesAndThreadsToStore == 0)
    {
        //
        // No active thread or process
        //
        return FALSE;
    }

    // LogInfo("Count of active process and threads : %x", CountOfProcessesAndThreadsToStore);

    //
    // Get the results
    //
    ThreadHolderQueryDetailsOfActiveDebuggingThreadsAndProcesses(BufferToStoreDetails, CountOfProcessesAndThreadsToStore);

    return TRUE;
}

/**
 * @brief Dispatch and perform attaching tasks
 * @details this function should not be called in vmx-root
 * 
 * @param AttachRequest 
 * @return VOID 
 */
VOID
AttachingTargetProcess(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS Request)
{
    //
    // As we're here, we need to initialize the user-mode debugger
    //
    UdInitializeUserDebugger();

    switch (Request->Action)
    {
    case DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_ATTACH:

        AttachingPerformAttachToProcess(Request, Request->IsStartingNewProcess);

        break;

    case DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_DETACH:

        AttachingPerformDetach(Request);

        break;

    case DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_REMOVE_HOOKS:

        AttachingRemoveHooks(Request);

        break;

    case DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_KILL_PROCESS:

        AttachingKillProcess(Request);

        break;

    case DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_PAUSE_PROCESS:

        AttachingPauseProcess(Request);

        break;

    case DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_SWITCH_BY_PROCESS_OR_THREAD:

        AttachingSwitchProcess(Request);

        break;

    case DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_QUERY_COUNT_OF_ACTIVE_DEBUGGING_THREADS:

        AttachingQueryCountOfActiveDebuggingThreadsAndProcesses(Request);

        break;

    default:

        Request->Result = DEBUGGER_ERROR_INVALID_ACTION_TYPE;

        break;
    }
}
