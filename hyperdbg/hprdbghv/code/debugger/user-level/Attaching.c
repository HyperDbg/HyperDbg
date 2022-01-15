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
 * @brief Create user-mode debugging details for threads 
 * 
 * @param ProcessId 
 * @param ThreadId 
 * @param Is32Bit 
 * @param Eprocess 
 * @param PebAddressToMonitor 
 * @param UsermodeReservedBuffer 
 * @return UINT64 returns the unique token 
 */
UINT64
AttachingCreateThreadDebuggingDetails(UINT32    ProcessId,
                                      UINT32    ThreadId,
                                      BOOLEAN   Enabled,
                                      BOOLEAN   Is32Bit,
                                      PEPROCESS Eprocess,
                                      UINT64    PebAddressToMonitor,
                                      UINT64    UsermodeReservedBuffer)
{
    PUSERMODE_DEBUGGING_THREADS_DETAILS ThreadDebuggingDetail;

    //
    // Allocate the buffer
    //
    ThreadDebuggingDetail = (PUSERMODE_DEBUGGING_THREADS_DETAILS)
        ExAllocatePoolWithTag(NonPagedPool, sizeof(USERMODE_DEBUGGING_THREADS_DETAILS), POOLTAG);

    if (!ThreadDebuggingDetail)
    {
        return NULL;
    }

    RtlZeroMemory(ThreadDebuggingDetail, sizeof(USERMODE_DEBUGGING_THREADS_DETAILS));

    //
    // Set the unique tag and increment it
    //
    ThreadDebuggingDetail->Token = g_SeedOfUserDebuggingDetails++;

    //
    // Set the details of the created buffer
    //
    ThreadDebuggingDetail->ProcessId              = ProcessId;
    ThreadDebuggingDetail->ThreadId               = ThreadId;
    ThreadDebuggingDetail->Enabled                = Enabled;
    ThreadDebuggingDetail->Is32Bit                = Is32Bit;
    ThreadDebuggingDetail->Eprocess               = Eprocess;
    ThreadDebuggingDetail->PebAddressToMonitor    = PebAddressToMonitor;
    ThreadDebuggingDetail->UsermodeReservedBuffer = UsermodeReservedBuffer;

    //
    // Attach it to the list of active thread (LIST_ENTRY)
    //
    InsertHeadList(&g_ThreadDebuggingDetailsListHead, &(ThreadDebuggingDetail->AttachedThreadList));

    //
    // return the token
    //
    return ThreadDebuggingDetail->Token;
}

/**
 * @brief Find user-mode debugging details for threads by token
 * 
 * @param Token 
 * @return PUSERMODE_DEBUGGING_THREADS_DETAILS 
 */
PUSERMODE_DEBUGGING_THREADS_DETAILS
AttachingFindThreadDebuggingDetailsByToken(UINT64 Token)
{
    PLIST_ENTRY TempList = 0;

    TempList = &g_ThreadDebuggingDetailsListHead;

    while (&g_ThreadDebuggingDetailsListHead != TempList->Flink)
    {
        TempList = TempList->Flink;
        PUSERMODE_DEBUGGING_THREADS_DETAILS ThreadDebuggingDetails =
            CONTAINING_RECORD(TempList, USERMODE_DEBUGGING_THREADS_DETAILS, AttachedThreadList);

        //
        // Check if we found the target thread and if it's enabled
        //
        if (ThreadDebuggingDetails->Token == Token && ThreadDebuggingDetails->Enabled)
        {
            return ThreadDebuggingDetails;
        }
    }

    return NULL;
}

/**
 * @brief Find user-mode debugging details for threads by process Id
 * 
 * @param ProcessId 
 * @return PUSERMODE_DEBUGGING_THREADS_DETAILS 
 */
PUSERMODE_DEBUGGING_THREADS_DETAILS
AttachingFindThreadDebuggingDetailsByProcessId(UINT32 ProcessId)
{
    PLIST_ENTRY TempList = 0;

    TempList = &g_ThreadDebuggingDetailsListHead;

    while (&g_ThreadDebuggingDetailsListHead != TempList->Flink)
    {
        TempList = TempList->Flink;
        PUSERMODE_DEBUGGING_THREADS_DETAILS ThreadDebuggingDetails =
            CONTAINING_RECORD(TempList, USERMODE_DEBUGGING_THREADS_DETAILS, AttachedThreadList);

        //
        // Check if we found the target thread and if it's enabled
        //
        if (ThreadDebuggingDetails->ProcessId == ProcessId && ThreadDebuggingDetails->Enabled)
        {
            return ThreadDebuggingDetails;
        }
    }

    return NULL;
}

/**
 * @brief Find user-mode debugging details for threads by process Id
 * and Thread Id
 * 
 * @param ProcessId 
 * @param ThreadId 
 * @return PUSERMODE_DEBUGGING_THREADS_DETAILS 
 */
PUSERMODE_DEBUGGING_THREADS_DETAILS
AttachingFindThreadDebuggingDetailsByProcessIdAndThreadId(UINT32 ProcessId, UINT32 ThreadId)
{
    PLIST_ENTRY TempList = 0;

    TempList = &g_ThreadDebuggingDetailsListHead;

    while (&g_ThreadDebuggingDetailsListHead != TempList->Flink)
    {
        TempList = TempList->Flink;
        PUSERMODE_DEBUGGING_THREADS_DETAILS ThreadDebuggingDetails =
            CONTAINING_RECORD(TempList, USERMODE_DEBUGGING_THREADS_DETAILS, AttachedThreadList);

        //
        // Check if we found the target thread and if it's enabled
        //
        if (ThreadDebuggingDetails->ProcessId == ProcessId &&
            ThreadDebuggingDetails->ThreadId == ThreadId &&
            ThreadDebuggingDetails->Enabled)
        {
            return ThreadDebuggingDetails;
        }
    }

    return NULL;
}

/**
 * @brief Find user-mode debugging details for threads that is in
 * the start-up phase
 * 
 * @return PUSERMODE_DEBUGGING_THREADS_DETAILS 
 */
PUSERMODE_DEBUGGING_THREADS_DETAILS
AttachingFindThreadDebuggingDetailsInStartingPhase()
{
    PLIST_ENTRY TempList = 0;

    TempList = &g_ThreadDebuggingDetailsListHead;

    while (&g_ThreadDebuggingDetailsListHead != TempList->Flink)
    {
        TempList = TempList->Flink;
        PUSERMODE_DEBUGGING_THREADS_DETAILS ThreadDebuggingDetails =
            CONTAINING_RECORD(TempList, USERMODE_DEBUGGING_THREADS_DETAILS, AttachedThreadList);

        if (ThreadDebuggingDetails->IsOnTheStartingPhase)
        {
            return ThreadDebuggingDetails;
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
AttachingRemoveAndFreeAllThreadDebuggingDetails()
{
    PLIST_ENTRY TempList = 0;

    TempList = &g_ThreadDebuggingDetailsListHead;

    while (&g_ThreadDebuggingDetailsListHead != TempList->Flink)
    {
        TempList = TempList->Flink;
        PUSERMODE_DEBUGGING_THREADS_DETAILS ThreadDebuggingDetails =
            CONTAINING_RECORD(TempList, USERMODE_DEBUGGING_THREADS_DETAILS, AttachedThreadList);

        //
        // Remove thread debugging detail from the list active threads
        //
        RemoveEntryList(&ThreadDebuggingDetails->AttachedThreadList);

        //
        // Unallocate the pool
        //
        ExFreePoolWithTag(ThreadDebuggingDetails, POOLTAG);
    }
}

/**
 * @brief Remove user-mode debugging details for threads by its token
 * 
 * @param Token 
 * @return BOOLEAN 
 */
BOOLEAN
AttachingRemoveThreadDebuggingDetailsByToken(UINT64 Token)
{
    PUSERMODE_DEBUGGING_THREADS_DETAILS ThreadDebuggingDetails;

    //
    // Find the entry
    //
    ThreadDebuggingDetails = AttachingFindThreadDebuggingDetailsByToken(Token);

    if (!ThreadDebuggingDetails)
    {
        //
        // Token not found!
        //
        return FALSE;
    }

    //
    // Remove thread debugging detail from the list active threads
    //
    RemoveEntryList(&ThreadDebuggingDetails->AttachedThreadList);

    //
    // Unallocate the pool
    //
    ExFreePoolWithTag(ThreadDebuggingDetails, POOLTAG);

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
AttachingSetStartingPhaseOfThreadDebuggingDetailsByToken(BOOLEAN Set, UINT64 Token)
{
    PUSERMODE_DEBUGGING_THREADS_DETAILS ThreadDebuggingDetails;

    //
    // If it's set to TRUE, then we check to only put 1 thread at the time
    // to the starting phase
    //
    ThreadDebuggingDetails = AttachingFindThreadDebuggingDetailsInStartingPhase();

    if (Set && ThreadDebuggingDetails != NULL)
    {
        //
        // There is another thread in starting phase!
        //
        return FALSE;
    }

    //
    // Find the entry
    //
    ThreadDebuggingDetails = AttachingFindThreadDebuggingDetailsByToken(Token);

    if (!ThreadDebuggingDetails)
    {
        //
        // Token not found!
        //
        return FALSE;
    }

    //
    // Set the starting phase
    //
    ThreadDebuggingDetails->IsOnTheStartingPhase = Set;

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
    AttachingSetStartingPhaseOfThreadDebuggingDetailsByToken(FALSE, ThreadDebuggingToken);

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
    PUSERMODE_DEBUGGING_THREADS_DETAILS ThreadDebuggingDetail = NULL;

    //
    // Not increment the RIP register as no instruction is intended to go
    //
    g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;

    ThreadDebuggingDetail = AttachingFindThreadDebuggingDetailsByProcessIdAndThreadId(PsGetCurrentProcessId(),
                                                                                      PsGetCurrentThreadId());
    //
    // Check to only break on the target process id and thread id and when
    // the entrypoint is not called, if the thread debugging detail is found
    //
    if (ThreadDebuggingDetail != NULL)
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

            if (!CheckMemoryAccessSafety(ThreadDebuggingDetail->EntrypointOfMainModule, sizeof(CHAR)))
            {
                // LogInfo("Injecting #PF for entrypoint at : %llx", ThreadDebuggingDetail->EntrypointOfMainModule);

                //
                // Inject #PF
                //
                VMEXIT_INTERRUPT_INFO InterruptInfo = {0};

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

                IdtEmulationHandlePageFaults(CurrentProcessorIndex, InterruptInfo, ThreadDebuggingDetail->EntrypointOfMainModule, 0x14);

                //
                // Re-apply the hw debug reg breakpoint
                //
                DebugRegistersSet(DEBUGGER_DEBUG_REGISTER_FOR_USER_MODE_ENTRY_POINT,
                                  BREAK_ON_INSTRUCTION_FETCH,
                                  FALSE,
                                  ThreadDebuggingDetail->EntrypointOfMainModule);
            }
            else
            {
                //
                // Address is valid, probably the module is previously loaded
                // or another process with same image is currently running
                // Thus, there is no need to inject #PF, we'll handle it in debugger
                //
                AttachingReachedToProcessEntrypoint(CurrentProcessorIndex, GuestRegs, ThreadDebuggingDetail->Token);
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
            AttachingReachedToProcessEntrypoint(CurrentProcessorIndex, GuestRegs, ThreadDebuggingDetail->Token);
        }
    }
    else
    {
        //
        // Check if we can find any thread in start up phase, if yes, we'll apply
        // it to the entrypoint
        //
        ThreadDebuggingDetail = AttachingFindThreadDebuggingDetailsInStartingPhase();

        if (ThreadDebuggingDetail != NULL)
        {
            //
            // Re-apply the hw debug reg breakpoint
            //
            DebugRegistersSet(DEBUGGER_DEBUG_REGISTER_FOR_USER_MODE_ENTRY_POINT,
                              BREAK_ON_INSTRUCTION_FETCH,
                              FALSE,
                              ThreadDebuggingDetail->EntrypointOfMainModule);
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
AttachingCheckPageFaultsWithUserDebugger(UINT32                CurrentProcessorIndex,
                                         PGUEST_REGS           GuestRegs,
                                         VMEXIT_INTERRUPT_INFO InterruptExit,
                                         UINT64                Address,
                                         ULONG                 ErrorCode)
{
    PUSERMODE_DEBUGGING_THREADS_DETAILS ThreadDebuggingDetail;

    //
    // Check if thread is in user-mode
    //
    if (g_GuestState[CurrentProcessorIndex].LastVmexitRip & 0xf000000000000000)
    {
        //
        // We won't intercept threads in kernel-mode
        //
        return FALSE;
    }

    ThreadDebuggingDetail = AttachingFindThreadDebuggingDetailsByProcessId(PsGetCurrentProcessId());

    if (!ThreadDebuggingDetail)
    {
        //
        // not related to user debugger
        //
        return FALSE;
    }

    LogInfo("intercepting %x.%x", PsGetCurrentProcessId(), PsGetCurrentThreadId());

    //
    // Handling state through the user-mode debugger
    //
    /* UdCheckAndHandleBreakpointsAndDebugBreaks(CurrentProcessorIndex,
                                              GuestRegs,
                                              DEBUGGEE_PAUSING_REASON_DEBUGGEE_GENERAL_DEBUG_BREAK,
                                              NULL);
                                              */

    //
    // related to user debugger
    //
    g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;

    return TRUE;
}

/**
 * @brief Attach to the target process
 * @details this function should be called in vmx-root
 * 
 * @param AttachRequest 
 * @param IsAttachingToEntrypoint 
 * @return VOID 
 */
VOID
AttachingPerformAttachToProcess(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS AttachRequest, BOOLEAN IsAttachingToEntrypoint)
{
    PEPROCESS   SourceProcess;
    UINT64      ThreadDebuggingToken;
    UINT64      PebAddressToMonitor;
    UINT64      UsermodeReservedBuffer;
    BOOLEAN     ResultOfApplyingEvent;
    BOOLEAN     Is32Bit;
    CR3_TYPE    TargetProcessKernelCr3;
    CR3_TYPE    CurrentProcessCr3;
    PPAGE_ENTRY Pml4;

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
    if (!UserAccessIsWow64Process(AttachRequest->ProcessId, &Is32Bit))
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
        return;
    }

    //
    // allocate memory in the target user-mode process
    //
    UsermodeReservedBuffer = MemoryMapperReserveUsermodeAddressInTargetProcess(AttachRequest->ProcessId, TRUE);

    if (UsermodeReservedBuffer == NULL)
    {
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
        return;
    }

    //
    // Adjust the nop sled buffer
    //
    if (!AttachingAdjustNopSledBuffer(UsermodeReservedBuffer,
                                      AttachRequest->ProcessId))
    {
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
        return;
    }

    //
    // Log for test
    //
    // LogInfo("Reserved address on the target process: %llx\n", UsermodeReservedBuffer);

    //
    // Create the event
    //
    ThreadDebuggingToken = AttachingCreateThreadDebuggingDetails(AttachRequest->ProcessId,
                                                                 AttachRequest->ThreadId,
                                                                 TRUE,
                                                                 Is32Bit,
                                                                 SourceProcess,
                                                                 PebAddressToMonitor,
                                                                 UsermodeReservedBuffer);

    //
    // Check if we successfully get the token
    //
    if (ThreadDebuggingToken == NULL)
    {
        AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
        return;
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
        if (!AttachingSetStartingPhaseOfThreadDebuggingDetailsByToken(TRUE, ThreadDebuggingToken))
        {
            //
            // Remove the created thread debugging detail
            //
            AttachingRemoveThreadDebuggingDetailsByToken(ThreadDebuggingToken);

            g_IsWaitingForUserModeModuleEntrypointToBeCalled = FALSE;
            AttachRequest->Result                            = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
            return;
        }

        //
        // Apply monitor memory range to the PEB address
        //
        ResultOfApplyingEvent =
            DebuggerEventEnableMonitorReadAndWriteForAddress(
                PebAddressToMonitor,
                AttachRequest->ProcessId,
                TRUE,
                TRUE);

        if (!ResultOfApplyingEvent)
        {
            //
            // Remove the created thread debugging detail
            //
            AttachingRemoveThreadDebuggingDetailsByToken(ThreadDebuggingToken);

            g_IsWaitingForUserModeModuleEntrypointToBeCalled = FALSE;
            AttachRequest->Result                            = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
            return;
        }
    }
    else
    {
        //
        // *** attaching to previously running process ***
        //

        //
        // Indicate that the future #PFs should be checked with user debugger
        //
        g_CheckPageFaultsWithUserDebugger = TRUE;

        //
        // Intercept all page-faults (#PFs)
        //
        ExtensionCommandSetExceptionBitmapAllCores(EXCEPTION_VECTOR_PAGE_FAULT);

        //
        // Find the kernel cr3 based on
        //
        TargetProcessKernelCr3 = GetCr3FromProcessId(AttachRequest->ProcessId);

        //
        // Find the PML4 of the target process
        //
        Pml4 = MemoryMapperGetPteVaByCr3(NULL, PML4, TargetProcessKernelCr3);

        if (Pml4 == NULL)
        {
            AttachRequest->Result = DEBUGGER_ERROR_UNABLE_TO_ATTACH_TO_TARGET_USER_MODE_PROCESS;
            return;
        }

        //
        // Switch to the target process's memory layout
        //
        CurrentProcessCr3 = SwitchOnAnotherProcessMemoryLayoutByCr3(TargetProcessKernelCr3);

        //
        // Set the supervisor bit to zero so we can intercept every access to
        // the entire memory for this process
        //
        Pml4->Supervisor = 0;

        //
        // Return to our process
        //
        RestoreToPreviousProcess(CurrentProcessCr3);
    }

    AttachRequest->Token  = ThreadDebuggingToken;
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
    PUSERMODE_DEBUGGING_THREADS_DETAILS ThreadDebuggingDetails;

    //
    // Get the thread debugging detail
    //
    ThreadDebuggingDetails = AttachingFindThreadDebuggingDetailsByToken(AttachRequest->Token);

    //
    // Check if token is valid or not
    //
    if (!ThreadDebuggingDetails)
    {
        AttachRequest->Result = DEBUGGER_ERROR_INVALID_THREAD_DEBUGGING_TOKEN;
        return;
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
        if (!EptHookUnHookSingleAddress(ThreadDebuggingDetails->PebAddressToMonitor,
                                        NULL,
                                        ThreadDebuggingDetails->ProcessId))
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
    //
    // As we're here, we need to initialize the user-mode debugger
    //
    UdInitializeUserDebugger();

    switch (Request->Action)
    {
    case DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_ATTACH:

        AttachingPerformAttachToProcess(Request, Request->IsStartingNewProcess);

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
