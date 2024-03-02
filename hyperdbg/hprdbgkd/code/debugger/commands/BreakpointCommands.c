/**
 * @file BreakpointCommands.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Routines for breakpoint commands
 * @details
 * @version 0.1
 * @date 2021-03-12
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Check and perform actions on RFLAGS.TF
 * @param ProcessId
 * @param ThreadId
 * @param TrapSetByDebugger
 *
 * @return BOOLEAN Shows whether the #DB should be handled by the debugger or re-injected
 */
BOOLEAN
BreakpointCheckAndPerformActionsOnTrapFlags(UINT32 ProcessId, UINT32 ThreadId, BOOLEAN * TrapSetByDebugger)
{
    UINT32                              Index;
    DEBUGGER_PROCESS_THREAD_INFORMATION ProcThrdInfo = {0};
    BOOLEAN                             Result;
    BOOLEAN                             ResultToReturn;
    RFLAGS                              Rflags = {0};

    //
    // Read the RFLAGS
    //
    Rflags.AsUInt = VmFuncGetRflags();

    //
    // Form the process id and thread id into a 64-bit value
    //
    ProcThrdInfo.Fields.ProcessId = ProcessId;
    ProcThrdInfo.Fields.ThreadId  = ThreadId;

    //
    // Make sure, nobody is in the middle of modifying the list
    //
    SpinlockLock(&BreakpointCommandTrapListLock);

    //
    // *** Search the list of processes/threads for the current process's trap flag state ***
    //
    Result = BinarySearchPerformSearchItem((UINT64 *)&g_TrapFlagState.ThreadInformation[0],
                                           g_TrapFlagState.NumberOfItems,
                                           &Index,
                                           ProcThrdInfo.asUInt);

    //
    // Indicate whether the trap flag is set by the debugger or not
    //
    *TrapSetByDebugger = Result;

    //
    // We check the trap flag after the results because we might set the trap flag
    // for the thread but the thread might run 'popfq' removing our trap flag
    // so, we both check whether thread is expected to have trap flag, if not
    // we check whether the trap flag is available or not
    //
    if (!Result && !Rflags.TrapFlag)
    {
        //
        // It's not related to a TRAP FLAG, and we didn't previously set trap flag for this thread
        // So, probably other events like setting hardware debug breakpoints caused this #DB
        // which means that it should be handled by the debugger
        //
        ResultToReturn = TRUE;
        goto Return;
    }
    else if (!Result && Rflags.TrapFlag)
    {
        //
        // As it's not set by the debugger (not found in our list), it means the program or
        // a debugger already set the trap flag, we'll return FALSE
        //
        // LogInfo("Caution: The process (pid:%x, tid:%x, name:%s) is utilizing a trap flag, "
        //         "which was not previously adjusted by HyperDbg. This occurrence could indicate "
        //         "the employment of an anti-debugging technique by the process or the involvement "
        //         "of another debugger. By default, HyperDbg automatically manages these #DB events "
        //         "and halt the debugger; however, if you wish to redirect them to the debugger, "
        //         "you can utilize 'test trap off'. Alternatively, you can use the transparent-mode "
        //         "to mitigate these situations",
        //         PsGetCurrentProcessId(),
        //         PsGetCurrentThreadId(),
        //         CommonGetProcessNameFromProcessControlBlock(PsGetCurrentProcess()));

        //
        // Returning false means that it should be re-injected into the debuggee
        //
        ResultToReturn = FALSE;
        goto Return;
    }
    else
    {
        //
        // *** being here means the thread is found in the list of threads that we set TRAP FLAG on it ***
        //

        //
        // Uset or set the TRAP flag
        //
        VmFuncSetRflagTrapFlag(FALSE);

        //
        // Remove the thread/process from the list
        // We're sure the Result is TRUE
        //
        InsertionSortDeleteItem((UINT64 *)&g_TrapFlagState.ThreadInformation[0],
                                &g_TrapFlagState.NumberOfItems,
                                Index);

        //
        // Handled #DB by debugger
        //
        ResultToReturn = TRUE;
        goto Return;
    }

Return:

    //
    // Unlock the list modification lock
    //
    SpinlockUnlock(&BreakpointCommandTrapListLock);

    //
    // By default, #DBs are managed by HyperDbg
    //
    return ResultToReturn;
}

/**
 * @brief Trigger callback for breakpoint hit
 *
 * @param DbgState The state of the debugger on the current core
 * @param ProcessId
 * @param ThreadId
 *
 * @return BOOLEAN If true, it won't halt the debugger, but if false will halt the debugger
 */
BOOLEAN
BreakpointTriggerCallbacks(PROCESSOR_DEBUGGING_STATE * DbgState, UINT32 ProcessId, UINT32 ThreadId)
{
    UNREFERENCED_PARAMETER(DbgState);
    UNREFERENCED_PARAMETER(ProcessId);
    UNREFERENCED_PARAMETER(ThreadId);

    //
    // Add the process/thread to the watching list
    //
    // LogInfo("Adding to watch list: Process Id: %x, Thread Id: %x", ProcessId, ThreadId);

    //
    // By default return FALSE to set handling the breakpoint to the user to the debugger
    //
    return FALSE;
}

/**
 * @brief This function makes sure to unset the RFLAGS.TF on next trigger of #DB
 * on the target process/thread
 * @param ProcessId
 * @param ThreadId
 *
 * @return BOOLEAN
 */
BOOLEAN
BreakpointRestoreTheTrapFlagOnceTriggered(UINT32 ProcessId, UINT32 ThreadId)
{
    UINT32                              Index;
    BOOLEAN                             Result;
    BOOLEAN                             SuccessfullyStored;
    DEBUGGER_PROCESS_THREAD_INFORMATION ProcThrdInfo = {0};

    //
    // Form the process id and thread id into a 64-bit value
    //
    ProcThrdInfo.Fields.ProcessId = ProcessId;
    ProcThrdInfo.Fields.ThreadId  = ThreadId;

    //
    // Make sure, nobody is in the middle of modifying the list
    //
    SpinlockLock(&BreakpointCommandTrapListLock);

    //
    // *** Search the list of processes/threads for the current process's trap flag state ***
    //
    Result = BinarySearchPerformSearchItem((UINT64 *)&g_TrapFlagState.ThreadInformation[0],
                                           g_TrapFlagState.NumberOfItems,
                                           &Index,
                                           ProcThrdInfo.asUInt);

    if (Result)
    {
        //
        // It means that we already find this entry in the stored list
        // so, just imply that the addition was successful (no need for extra addition)
        //
        SuccessfullyStored = TRUE;
        goto Return;
    }
    else
    {
        //
        // Insert the thread into the list as the item is not already present
        //
        SuccessfullyStored = InsertionSortInsertItem((UINT64 *)&g_TrapFlagState.ThreadInformation[0],
                                                     &g_TrapFlagState.NumberOfItems,
                                                     MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_TRAPS,
                                                     ProcThrdInfo.asUInt);
        goto Return;
    }

Return:
    //
    // Unlock the list modification lock
    //
    SpinlockUnlock(&BreakpointCommandTrapListLock);

    return SuccessfullyStored;
}

/**
 * @brief Check and handle debug breakpoint exceptions
 *
 * @param CoreId
 *
 * @return BOOLEAN
 */
BOOLEAN
BreakpointCheckAndHandleDebugBreakpoint(UINT32 CoreId)
{
    BOOLEAN                     TrapSetByDebugger;
    PROCESSOR_DEBUGGING_STATE * DbgState                  = &g_DbgState[CoreId];
    BOOLEAN                     HandledByDebuggerRoutines = TRUE;

    //
    // *** Check whether anything should be changed with trap-flags
    // and also it indicates whether the debugger itself set this trap
    // flag or it's not supposed to be set by the debugger ***
    //
    if (BreakpointCheckAndPerformActionsOnTrapFlags(HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                    HANDLE_TO_UINT32(PsGetCurrentThreadId()),
                                                    &TrapSetByDebugger))
    {
        if (DbgState->ThreadOrProcessTracingDetails.DebugRegisterInterceptionState)
        {
            //
            // This check was to show whether it is because of thread change detection or not
            //

            // This way of handling has a problem, if the user set to change
            // the thread and instead of using 'g', it pressed the 'p' to
            // set or a trap happens somewhere then will be ignored
            // it because we don't know the origin of this debug breakpoint
            // and it only happens on '.thread2' command, the correct way
            // to handle it is to find the exact hw debug register that caused
            // this vm-exit, but it's a really rare case, so we left it without
            // handling this case
            //
            ThreadHandleThreadChange(DbgState);
        }
        else if (g_UserDebuggerState == TRUE &&
                 (g_IsWaitingForUserModeProcessEntryToBeCalled || g_IsWaitingForReturnAndRunFromPageFault))
        {
            //
            // Handle for user-mode attaching mechanism
            //
            AttachingHandleEntrypointInterception(DbgState);
        }
        else if (g_KernelDebuggerState == TRUE)
        {
            //
            // Here we added the handler for the kernel because we want
            // stepping routines to work, even if the debugger masks the
            // traps by using 'test trap off', so stepping still works
            //

            //
            // Handle debug events (breakpoint, traps, hardware debug register when kernel
            // debugger is attached)
            //
            KdHandleDebugEventsWhenKernelDebuggerIsAttached(DbgState, TrapSetByDebugger);
        }
        else if (UdCheckAndHandleBreakpointsAndDebugBreaks(DbgState,
                                                           DEBUGGEE_PAUSING_REASON_DEBUGGEE_GENERAL_DEBUG_BREAK,
                                                           NULL))
        {
            //
            // if the above function returns true, no need for further action
            // it's handled in the user debugger
            //
        }
        else
        {
            //
            // Here it means that the trap is supposed to be handled by
            // HyperDbg but, we couldn't find any routines that gonna
            // handle it (it's probably an error)
            //
            HandledByDebuggerRoutines = FALSE;
            LogError("Err, trap is supposed to be handled by the debugger, but none of routines handled it");
        }
    }
    else
    {
        //
        // *** it's not supposed to be handled by the debugger routines, the guest
        // or the target debuggee throws a debug break (#DB) ***
        //

        //
        // It means that it's not handled by the debugger routines
        // By default HyperDbg intercepts all #DBs and break the debugger if
        // it's attached to the debugger, otherwise injects to the guest VM
        //
        if (g_InterceptDebugBreaks)
        {
            //
            // The user explicitly told the debugger not to intercept any
            // traps (e.g., by using 'test trap off')
            //
            HandledByDebuggerRoutines = FALSE;
        }
        else if (g_KernelDebuggerState == TRUE)
        {
            //
            // Handle debug events (breakpoint, traps, hardware debug register when kernel
            // debugger is attached)
            //
            KdHandleDebugEventsWhenKernelDebuggerIsAttached(DbgState, TrapSetByDebugger);
        }
        else if (UdCheckAndHandleBreakpointsAndDebugBreaks(DbgState,
                                                           DEBUGGEE_PAUSING_REASON_DEBUGGEE_GENERAL_DEBUG_BREAK,
                                                           NULL))
        {
            //
            // if the above function returns true, no need for further action
            // it's handled in the user debugger
            //
        }
        else
        {
            //
            // Inject to back to the guest as it's not either handled by the kernel debugger
            // routines or the user debugger
            //
            HandledByDebuggerRoutines = FALSE;
        }
    }

    return HandledByDebuggerRoutines;
}

/**
 * @brief clears the 0xcc and removes the breakpoint
 * @detail this function won't remove the descriptor from the list
 * @param BreakpointDescriptor
 *
 * @return BOOLEAN
 */
BOOLEAN
BreakpointClear(PDEBUGGEE_BP_DESCRIPTOR BreakpointDescriptor)
{
    BYTE TargetMem = NULL_ZERO;

    //
    // Check if address is safe (only one byte for 0xcc)
    //
    if (!CheckAccessValidityAndSafety(BreakpointDescriptor->Address, sizeof(BYTE)))
    {
        //
        // Double check if we can access it by physical address
        //
        MemoryMapperReadMemorySafeByPhysicalAddress(BreakpointDescriptor->PhysAddress,
                                                    (UINT64)&TargetMem,
                                                    sizeof(BYTE));

        if (TargetMem != 0xcc)
        {
            return FALSE;
        }
    }

    //
    // Apply the previous byte
    //
    MemoryMapperWriteMemorySafeByPhysicalAddress(BreakpointDescriptor->PhysAddress,
                                                 (UINT64)&BreakpointDescriptor->PreviousByte,
                                                 sizeof(BYTE));

    //
    // Set breakpoint to disabled
    //
    BreakpointDescriptor->Enabled                = FALSE;
    BreakpointDescriptor->AvoidReApplyBreakpoint = TRUE;

    return TRUE;
}

/**
 * @brief Clears the breakpoint and remove the entry from the breakpoint list
 * @param
 *
 * @return VOID
 */
VOID
BreakpointClearAndDeallocateMemory(PDEBUGGEE_BP_DESCRIPTOR BreakpointDesc)
{
    //
    // Clear the breakpoint
    //
    BreakpointClear(BreakpointDesc);

    //
    // Remove breakpoint from the list of breakpoints
    //
    RemoveEntryList(&BreakpointDesc->BreakpointsList);

    //
    // Uninitialize the breakpoint descriptor (safely)
    //
    PoolManagerFreePool((UINT64)BreakpointDesc);
}

/**
 * @brief Check and reapply breakpoint
 *
 * @param CoreId
 *
 * @return BOOLEAN
 */
BOOLEAN
BreakpointCheckAndHandleReApplyingBreakpoint(UINT32 CoreId)
{
    BOOLEAN                     Result   = FALSE;
    PROCESSOR_DEBUGGING_STATE * DbgState = &g_DbgState[CoreId];

    if (DbgState->SoftwareBreakpointState != NULL)
    {
        BYTE BreakpointByte = 0xcc;

        //
        // MTF is handled
        //
        Result = TRUE;

        //
        // Restore previous breakpoint byte
        //
        MemoryMapperWriteMemorySafeByPhysicalAddress(
            DbgState->SoftwareBreakpointState->PhysAddress,
            (UINT64)&BreakpointByte,
            sizeof(BYTE));

        //
        // Check if we should re-enabled IF bit of RFLAGS or not
        //
        if (DbgState->SoftwareBreakpointState->SetRflagsIFBitOnMtf)
        {
            RFLAGS Rflags = {0};

            Rflags.AsUInt = VmFuncGetRflags();

            Rflags.InterruptEnableFlag = TRUE;

            VmFuncSetRflags(Rflags.AsUInt);

            DbgState->SoftwareBreakpointState->SetRflagsIFBitOnMtf = FALSE;
        }

        DbgState->SoftwareBreakpointState = NULL;
    }

    return Result;
}

/**
 * @brief Check if the breakpoint vm-exit relates to 'bp' command or not
 *
 * @param DbgState The state of the debugger on the current core
 * @param GuestRip
 * @param Reason
 * @param ChangeMtfState
 *
 * @return BOOLEAN
 */
BOOLEAN
BreakpointCheckAndHandleDebuggerDefinedBreakpoints(PROCESSOR_DEBUGGING_STATE * DbgState,
                                                   UINT64                      GuestRip,
                                                   DEBUGGEE_PAUSING_REASON     Reason,
                                                   BOOLEAN                     ChangeMtfState)
{
    CR3_TYPE                         GuestCr3              = {0};
    BOOLEAN                          IsHandledByBpRoutines = FALSE;
    PLIST_ENTRY                      TempList              = 0;
    UINT64                           GuestRipPhysical      = (UINT64)NULL;
    DEBUGGER_TRIGGERED_EVENT_DETAILS TargetContext         = {0};
    RFLAGS                           Rflags                = {0};
    BOOLEAN                          AvoidUnsetMtf         = FALSE;
    BOOLEAN                          IgnoreUserHandling    = FALSE;

    //
    // ***** Check breakpoint for 'bp' command *****
    //

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

    //
    // Convert breakpoint to physical address
    //
    GuestRipPhysical = VirtualAddressToPhysicalAddressByProcessCr3((PVOID)GuestRip, GuestCr3);

    //
    // Iterate through the list of breakpoints
    //
    TempList = &g_BreakpointsListHead;

    while (&g_BreakpointsListHead != TempList->Flink)
    {
        TempList                                      = TempList->Flink;
        PDEBUGGEE_BP_DESCRIPTOR CurrentBreakpointDesc = CONTAINING_RECORD(TempList, DEBUGGEE_BP_DESCRIPTOR, BreakpointsList);

        if (CurrentBreakpointDesc->PhysAddress == GuestRipPhysical)
        {
            //
            // It's a breakpoint by 'bp' command
            //
            IsHandledByBpRoutines = TRUE;

            //
            // First, we remove the breakpoint
            //
            MemoryMapperWriteMemorySafeByPhysicalAddress(GuestRipPhysical,
                                                         (UINT64)&CurrentBreakpointDesc->PreviousByte,
                                                         sizeof(BYTE));

            //
            // Now, halt the debuggee
            //
            TargetContext.Context = (PVOID)VmFuncGetLastVmexitRip(DbgState->CoreId);

            //
            // In breakpoints tag is breakpoint id, not event tag
            //
            if (Reason == DEBUGGEE_PAUSING_REASON_DEBUGGEE_SOFTWARE_BREAKPOINT_HIT)
            {
                TargetContext.Tag = CurrentBreakpointDesc->BreakpointId;
            }

            //
            // Hint the debuggee about the length
            //
            DbgState->InstructionLengthHint = CurrentBreakpointDesc->InstructionLength;

            //
            // Check constraints
            //
            if ((CurrentBreakpointDesc->Pid == DEBUGGEE_BP_APPLY_TO_ALL_PROCESSES || CurrentBreakpointDesc->Pid == HANDLE_TO_UINT32(PsGetCurrentProcessId())) &&
                (CurrentBreakpointDesc->Tid == DEBUGGEE_BP_APPLY_TO_ALL_THREADS || CurrentBreakpointDesc->Tid == HANDLE_TO_UINT32(PsGetCurrentThreadId())) &&
                (CurrentBreakpointDesc->Core == DEBUGGEE_BP_APPLY_TO_ALL_CORES || CurrentBreakpointDesc->Core == DbgState->CoreId))
            {
                //
                // Check if breakpoint should be removed after this hit or not
                //
                if (CurrentBreakpointDesc->RemoveAfterHit)
                {
                    //
                    // One hit, we have to remove it
                    //
                    BreakpointClearAndDeallocateMemory(CurrentBreakpointDesc);
                }

                //
                // Check if it needs to check for callbacks or not
                //
                if (CurrentBreakpointDesc->CheckForCallbacks)
                {
                    //
                    // check callbacks
                    //
                    IgnoreUserHandling = BreakpointTriggerCallbacks(DbgState, HANDLE_TO_UINT32(PsGetCurrentProcessId()), HANDLE_TO_UINT32(PsGetCurrentThreadId()));
                }

                //
                // Check if we need to handle the breakpoint by user or just ignore handling it
                //
                if (!IgnoreUserHandling && !g_InterceptBreakpoints && !g_InterceptBreakpointsAndEventsForCommandsInRemoteComputer)
                {
                    //
                    // *** It's not safe to access CurrentBreakpointDesc anymore as the
                    // breakpoint might be removed ***
                    //
                    KdHandleBreakpointAndDebugBreakpoints(DbgState,
                                                          Reason,
                                                          &TargetContext);
                }
            }

            //
            // Reset hint to instruction length
            //
            DbgState->InstructionLengthHint = 0;

            //
            // Check if we should re-apply the breakpoint after this instruction
            // or not (in other words, is breakpoint still valid)
            //
            if (!CurrentBreakpointDesc->AvoidReApplyBreakpoint)
            {
                //
                // We should re-apply the breakpoint on next mtf
                //
                DbgState->SoftwareBreakpointState = CurrentBreakpointDesc;

                //
                // Fire and MTF
                //
                VmFuncSetMonitorTrapFlag(TRUE);
                AvoidUnsetMtf = TRUE;

                //
                // As we want to continue debuggee, the MTF might arrive when the
                // host finish executing it's time slice; thus, a clock interrupt
                // or an IPI might be arrived and the next instruction is not what
                // we expect, because of that we check if the IF (Interrupt enable)
                // flag of RFLAGS is enabled or not, if enabled then we remove it
                // to avoid any clock-interrupt or IPI to arrive and the next
                // instruction is our next instruction in the current execution
                // context
                //
                Rflags.AsUInt = VmFuncGetRflags();

                if (Rflags.InterruptEnableFlag)
                {
                    Rflags.InterruptEnableFlag = FALSE;
                    VmFuncSetRflags(Rflags.AsUInt);

                    //
                    // An indicator to restore RFLAGS if to enabled state
                    //
                    DbgState->SoftwareBreakpointState->SetRflagsIFBitOnMtf = TRUE;
                }
            }

            //
            // Do not increment rip
            //
            VmFuncSuppressRipIncrement(DbgState->CoreId);

            //
            // No need to iterate anymore
            //
            break;
        }
    }

    if (IsHandledByBpRoutines && ChangeMtfState)
    {
        VmFuncChangeMtfUnsettingState(DbgState->CoreId, AvoidUnsetMtf);
    }

    return IsHandledByBpRoutines;
}

/**
 * @brief Handle breakpoint vm-exits (#BP)
 *
 * @param CoreId
 *
 * @return BOOLEAN
 */
BOOLEAN
BreakpointHandleBreakpoints(UINT32 CoreId)
{
    DEBUGGER_TRIGGERED_EVENT_DETAILS TargetContext = {0};
    UINT64                           GuestRip      = 0;
    PROCESSOR_DEBUGGING_STATE *      DbgState      = &g_DbgState[CoreId];

    //
    // re-inject #BP back to the guest if not handled by the hidden breakpoint
    //

    if (g_KernelDebuggerState)
    {
        //
        // Kernel debugger is attached, let's halt everything
        //
        GuestRip = VmFuncGetRip();

        //
        // A breakpoint triggered and two things might be happened,
        // first, a breakpoint is triggered randomly in the computer and
        // we shouldn't do anything on it (won't change the instruction)
        // second, the breakpoint is because of the 'bp' command, we should
        // replace it with exact byte
        //

        if (!BreakpointCheckAndHandleDebuggerDefinedBreakpoints(DbgState,
                                                                GuestRip,
                                                                DEBUGGEE_PAUSING_REASON_DEBUGGEE_SOFTWARE_BREAKPOINT_HIT,
                                                                FALSE))
        {
            //
            // To avoid the computer crash situation from the HyperDbg's breakpoint hitting while the interception is on
            // we should always call BreakpointCheckAndHandleDebuggerDefinedBreakpoints first to handle the breakpoint
            //

            if (g_InterceptBreakpoints || g_InterceptBreakpointsAndEventsForCommandsInRemoteComputer)
            {
                //
                // re-inject back to the guest as not handled if the interception is on and the breakpoint is not from the Hyperdbg's breakpoints
                //
                return FALSE;
            }

            //
            // It's a random breakpoint byte
            //
            TargetContext.Context = (PVOID)GuestRip;
            KdHandleBreakpointAndDebugBreakpoints(DbgState,
                                                  DEBUGGEE_PAUSING_REASON_DEBUGGEE_SOFTWARE_BREAKPOINT_HIT,
                                                  &TargetContext);

            //
            // Increment rip
            //
            VmFuncPerformRipIncrement(DbgState->CoreId);
        }
    }
    else
    {
        //
        // re-inject back to the guest as not handled here
        //
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief writes the 0xcc and applies the breakpoint
 * @detail this function won't remove the descriptor from the list
 *
 * @param BreakpointDescriptor
 *
 * @return BOOLEAN
 */
BOOLEAN
BreakpointWrite(PDEBUGGEE_BP_DESCRIPTOR BreakpointDescriptor)
{
    BYTE PreviousByte   = NULL_ZERO;
    BYTE BreakpointByte = 0xcc; // int 3

    //
    // Check if address is safe (only one byte for 0xcc)
    //
    if (!CheckAccessValidityAndSafety(BreakpointDescriptor->Address, sizeof(BYTE)))
    {
        return FALSE;
    }

    //
    // Read and save previous byte and save it to the descriptor
    //
    MemoryMapperReadMemorySafeOnTargetProcess(BreakpointDescriptor->Address, &PreviousByte, sizeof(BYTE));
    BreakpointDescriptor->PreviousByte = PreviousByte;

    //
    // Set breakpoint to enabled
    //
    BreakpointDescriptor->Enabled                = TRUE;
    BreakpointDescriptor->AvoidReApplyBreakpoint = FALSE;

    //
    // Apply the breakpoint
    //
    MemoryMapperWriteMemorySafeByPhysicalAddress(BreakpointDescriptor->PhysAddress,
                                                 (UINT64)&BreakpointByte,
                                                 sizeof(BYTE));

    return TRUE;
}

/**
 * @brief Remove all the breakpoints if possible
 *
 * @return VOID
 */
VOID
BreakpointRemoveAllBreakpoints()
{
    PLIST_ENTRY TempList = 0;

    //
    // Iterate through the list of breakpoints
    //
    TempList = &g_BreakpointsListHead;

    while (&g_BreakpointsListHead != TempList->Flink)
    {
        TempList                                      = TempList->Flink;
        PDEBUGGEE_BP_DESCRIPTOR CurrentBreakpointDesc = CONTAINING_RECORD(TempList, DEBUGGEE_BP_DESCRIPTOR, BreakpointsList);

        //
        // Clear and deallocate the breakpoint
        //
        BreakpointClearAndDeallocateMemory(CurrentBreakpointDesc);
    }
}

/**
 * @brief Find entry of breakpoint descriptor from list
 * of breakpoints by breakpoint id
 * @param BreakpointId
 *
 * @return PDEBUGGEE_BP_DESCRIPTOR
 */
PDEBUGGEE_BP_DESCRIPTOR
BreakpointGetEntryByBreakpointId(UINT64 BreakpointId)
{
    PLIST_ENTRY TempList = 0;

    TempList = &g_BreakpointsListHead;

    while (&g_BreakpointsListHead != TempList->Flink)
    {
        TempList                                      = TempList->Flink;
        PDEBUGGEE_BP_DESCRIPTOR CurrentBreakpointDesc = CONTAINING_RECORD(TempList, DEBUGGEE_BP_DESCRIPTOR, BreakpointsList);

        if (CurrentBreakpointDesc->BreakpointId == BreakpointId)
        {
            return CurrentBreakpointDesc;
        }
    }

    //
    // We didn't find anything, so return null
    //
    return NULL;
}

/**
 * @brief Find entry of breakpoint descriptor from list
 * of breakpoints by address
 * @param Address
 *
 * @return PDEBUGGEE_BP_DESCRIPTOR
 */
PDEBUGGEE_BP_DESCRIPTOR
BreakpointGetEntryByAddress(UINT64 Address)
{
    PLIST_ENTRY TempList = 0;

    TempList = &g_BreakpointsListHead;

    while (&g_BreakpointsListHead != TempList->Flink)
    {
        TempList                                      = TempList->Flink;
        PDEBUGGEE_BP_DESCRIPTOR CurrentBreakpointDesc = CONTAINING_RECORD(TempList, DEBUGGEE_BP_DESCRIPTOR, BreakpointsList);

        if (CurrentBreakpointDesc->Address == Address)
        {
            return CurrentBreakpointDesc;
        }
    }

    //
    // We didn't find anything, so return null
    //
    return NULL;
}

/**
 * @brief Add new breakpoints
 * @param BpDescriptor
 *
 * @return BOOLEAN
 */
BOOLEAN
BreakpointAddNew(PDEBUGGEE_BP_PACKET BpDescriptorArg)
{
    PDEBUGGEE_BP_DESCRIPTOR BreakpointDescriptor = NULL;
    CR3_TYPE                GuestCr3             = {0};
    BOOLEAN                 IsAddress32Bit       = FALSE;

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = LayoutGetCurrentProcessCr3().Flags;

    //
    // *** Validate arguments ***
    //

    //
    // Check if address is safe (only one byte for 0xcc)
    //
    if (!CheckAccessValidityAndSafety(BpDescriptorArg->Address, sizeof(BYTE)))
    {
        BpDescriptorArg->Result = DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_ADDRESS_BASED_ON_CURRENT_PROCESS;
        return FALSE;
    }

    //
    // Check if the core number is not invalid
    //
    if (BpDescriptorArg->Core != DEBUGGEE_BP_APPLY_TO_ALL_CORES &&
        !CommonValidateCoreNumber(BpDescriptorArg->Core))
    {
        //
        // Core is invalid (Set the error)
        //
        BpDescriptorArg->Result = DEBUGGER_ERROR_INVALID_CORE_ID;
        return FALSE;
    }

    //
    // Check if breakpoint already exists on list or not
    //
    if (BreakpointGetEntryByAddress(BpDescriptorArg->Address) != NULL)
    {
        //
        // Address is already on the list (Set the error)
        //
        BpDescriptorArg->Result = DEBUGGER_ERROR_BREAKPOINT_ALREADY_EXISTS_ON_THE_ADDRESS;
        return FALSE;
    }

    //
    // We won't check for process id and thread id, if these arguments are invalid
    // then the HyperDbg simply ignores the breakpoints but it makes the computer slow
    // it just won't be triggered
    //

    //
    // When we reach here means that the arguments are valid and address is
    // safe to access (put 0xcc)
    //

    //
    // Get the pre-allocated buffer
    //
    BreakpointDescriptor = (DEBUGGEE_BP_DESCRIPTOR *)PoolManagerRequestPool(BREAKPOINT_DEFINITION_STRUCTURE, TRUE, sizeof(DEBUGGEE_BP_DESCRIPTOR));

    if (BreakpointDescriptor == NULL)
    {
        //
        // No pool ! Probably the user set more than MAXIMUM_BREAKPOINTS_WITHOUT_CONTINUE
        // pools without IOCTL (continue)
        //
        BpDescriptorArg->Result = DEBUGGER_ERROR_MAXIMUM_BREAKPOINT_WITHOUT_CONTINUE;
        return FALSE;
    }

    //
    // Copy details of breakpoint to the descriptor structure
    //
    g_MaximumBreakpointId++;
    BreakpointDescriptor->BreakpointId      = g_MaximumBreakpointId;
    BreakpointDescriptor->Address           = BpDescriptorArg->Address;
    BreakpointDescriptor->PhysAddress       = VirtualAddressToPhysicalAddressByProcessCr3((PVOID)BpDescriptorArg->Address,
                                                                                    GuestCr3);
    BreakpointDescriptor->Core              = BpDescriptorArg->Core;
    BreakpointDescriptor->Pid               = BpDescriptorArg->Pid;
    BreakpointDescriptor->Tid               = BpDescriptorArg->Tid;
    BreakpointDescriptor->RemoveAfterHit    = BpDescriptorArg->RemoveAfterHit;
    BreakpointDescriptor->CheckForCallbacks = BpDescriptorArg->CheckForCallbacks;

    //
    // Check whether address is 32-bit or 64-bit
    //
    if (BpDescriptorArg->Address & 0xff00000000000000)
    {
        //
        // This is a kernel-base address and as the kernel is 64-bit, we assume it's a 64-bit address
        //
        IsAddress32Bit = FALSE;
    }
    else
    {
        //
        // The address is not a kernel address, thus, we check whether the debuggee is running on user-mode
        // or not
        //
        IsAddress32Bit = KdIsGuestOnUsermode32Bit();
    }

    //
    // Use length disassembler engine to get the instruction length
    //
    BreakpointDescriptor->InstructionLength = (UINT16)DisassemblerLengthDisassembleEngineInVmxRootOnTargetProcess(
        (PVOID)BpDescriptorArg->Address,
        IsAddress32Bit);

    //
    // Breakpoints are enabled by default
    //
    BreakpointDescriptor->Enabled = TRUE;

    //
    // Now we should add the breakpoint to the list of breakpoints (LIST_ENTRY)
    //
    InsertHeadList(&g_BreakpointsListHead, &(BreakpointDescriptor->BreakpointsList));

    //
    // Apply the breakpoint
    //
    BreakpointWrite(BreakpointDescriptor);

    //
    // Show that operation was successful
    //
    BpDescriptorArg->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    return TRUE;
}

/**
 * @brief List all breakpoints
 *
 * @return VOID
 */
VOID
BreakpointListAllBreakpoint()
{
    BOOLEAN     IsListEmpty = TRUE;
    PLIST_ENTRY TempList    = 0;

    TempList = &g_BreakpointsListHead;

    while (&g_BreakpointsListHead != TempList->Blink)
    {
        TempList                                      = TempList->Blink;
        PDEBUGGEE_BP_DESCRIPTOR CurrentBreakpointDesc = CONTAINING_RECORD(TempList, DEBUGGEE_BP_DESCRIPTOR, BreakpointsList);

        if (IsListEmpty)
        {
            Log("Id   Address           Status\n");
            Log("--   ---------------   --------");

            IsListEmpty = FALSE;
        }

        Log("\n%02x   %016llx  %s", CurrentBreakpointDesc->BreakpointId, CurrentBreakpointDesc->Address, CurrentBreakpointDesc->Enabled ? "enabled" : "disabled");

        if (CurrentBreakpointDesc->Core != DEBUGGEE_BP_APPLY_TO_ALL_CORES)
        {
            Log(" core = %x ", CurrentBreakpointDesc->Core);
        }
        if (CurrentBreakpointDesc->Pid != DEBUGGEE_BP_APPLY_TO_ALL_PROCESSES)
        {
            Log(" pid = %x ", CurrentBreakpointDesc->Pid);
        }
        if (CurrentBreakpointDesc->Tid != DEBUGGEE_BP_APPLY_TO_ALL_THREADS)
        {
            Log(" tid = %x ", CurrentBreakpointDesc->Tid);
        }
    }

    //
    // Check if the list is empty or not
    //
    if (IsListEmpty)
    {
        Log("Breakpoints list is empty");
    }
}

/**
 * @brief List of modify breakpoints
 * @param ListOrModifyBreakpoints
 *
 * @return BOOLEAN
 */
BOOLEAN
BreakpointListOrModify(PDEBUGGEE_BP_LIST_OR_MODIFY_PACKET ListOrModifyBreakpoints)
{
    PDEBUGGEE_BP_DESCRIPTOR BreakpointDescriptor = NULL;

    if (ListOrModifyBreakpoints->Request == DEBUGGEE_BREAKPOINT_MODIFICATION_REQUEST_LIST_BREAKPOINTS)
    {
        BreakpointListAllBreakpoint();
    }
    else if (ListOrModifyBreakpoints->Request == DEBUGGEE_BREAKPOINT_MODIFICATION_REQUEST_ENABLE)
    {
        BreakpointDescriptor = BreakpointGetEntryByBreakpointId(ListOrModifyBreakpoints->BreakpointId);

        if (BreakpointDescriptor == NULL)
        {
            //
            // Breakpoint id is invalid
            //
            ListOrModifyBreakpoints->Result = DEBUGGER_ERROR_BREAKPOINT_ID_NOT_FOUND;
            return FALSE;
        }

        //
        // Check to make sure that breakpoint is not already enabled
        //
        if (BreakpointDescriptor->Enabled)
        {
            ListOrModifyBreakpoints->Result = DEBUGGER_ERROR_BREAKPOINT_ALREADY_ENABLED;
            return FALSE;
        }

        //
        // Set the breakpoint (without removing from list)
        //
        BreakpointWrite(BreakpointDescriptor);
    }
    else if (ListOrModifyBreakpoints->Request == DEBUGGEE_BREAKPOINT_MODIFICATION_REQUEST_DISABLE)
    {
        BreakpointDescriptor = BreakpointGetEntryByBreakpointId(ListOrModifyBreakpoints->BreakpointId);

        if (BreakpointDescriptor == NULL)
        {
            //
            // Breakpoint id is invalid
            //
            ListOrModifyBreakpoints->Result = DEBUGGER_ERROR_BREAKPOINT_ID_NOT_FOUND;
            return FALSE;
        }

        //
        // Check to make sure that breakpoint is not already disabled
        //
        if (!BreakpointDescriptor->Enabled)
        {
            ListOrModifyBreakpoints->Result = DEBUGGER_ERROR_BREAKPOINT_ALREADY_DISABLED;
            return FALSE;
        }

        //
        // Unset the breakpoint (without removing from list)
        //
        BreakpointClear(BreakpointDescriptor);
    }
    else if (ListOrModifyBreakpoints->Request == DEBUGGEE_BREAKPOINT_MODIFICATION_REQUEST_CLEAR)
    {
        BreakpointDescriptor = BreakpointGetEntryByBreakpointId(ListOrModifyBreakpoints->BreakpointId);

        if (BreakpointDescriptor == NULL)
        {
            //
            // Breakpoint id is invalid
            //
            ListOrModifyBreakpoints->Result = DEBUGGER_ERROR_BREAKPOINT_ID_NOT_FOUND;
            return FALSE;
        }

        //
        // Clear and deallocate the breakpoint
        //
        BreakpointClearAndDeallocateMemory(BreakpointDescriptor);
    }

    //
    // Operation was successful
    //
    ListOrModifyBreakpoints->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    return TRUE;
}
