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
    CR3_TYPE                         GuestCr3;
    BOOLEAN                          IsHandledByBpRoutines = FALSE;
    PLIST_ENTRY                      TempList              = 0;
    UINT64                           GuestRipPhysical      = NULL;
    DEBUGGER_TRIGGERED_EVENT_DETAILS ContextAndTag         = {0};
    RFLAGS                           Rflags                = {0};
    ULONG                            LengthOfExitInstr     = 0;
    BYTE                             InstrByte             = NULL;
    BOOLEAN                          AvoidUnsetMtf         = FALSE;

    //
    // ***** Check breakpoint for 'bp' command *****
    //

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = GetRunningCr3OnTargetProcess().Flags;

    //
    // Convert breakpoint to physical address
    //
    GuestRipPhysical = VirtualAddressToPhysicalAddressByProcessCr3(GuestRip, GuestCr3);

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
                                                         &CurrentBreakpointDesc->PreviousByte,
                                                         sizeof(BYTE));

            //
            // Now, halt the debuggee
            //
            ContextAndTag.Context = VmFuncGetLastVmexitRip(DbgState->CoreId);

            //
            // In breakpoints tag is breakpoint id, not event tag
            //
            if (Reason == DEBUGGEE_PAUSING_REASON_DEBUGGEE_SOFTWARE_BREAKPOINT_HIT)
            {
                ContextAndTag.Tag = CurrentBreakpointDesc->BreakpointId;
            }

            //
            // Hint the debuggee about the length
            //
            DbgState->InstructionLengthHint = CurrentBreakpointDesc->InstructionLength;

            //
            // Check constraints
            //
            if ((CurrentBreakpointDesc->Pid == DEBUGGEE_BP_APPLY_TO_ALL_PROCESSES || CurrentBreakpointDesc->Pid == PsGetCurrentProcessId()) &&
                (CurrentBreakpointDesc->Tid == DEBUGGEE_BP_APPLY_TO_ALL_THREADS || CurrentBreakpointDesc->Tid == PsGetCurrentThreadId()) &&
                (CurrentBreakpointDesc->Core == DEBUGGEE_BP_APPLY_TO_ALL_CORES || CurrentBreakpointDesc->Core == DbgState->CoreId))
            {
                //
                // *** It's not safe to access CurrentBreakpointDesc anymore as the
                // breakpoint might be removed ***
                //

                KdHandleBreakpointAndDebugBreakpoints(DbgState,
                                                      Reason,
                                                      &ContextAndTag);
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
                HvSetMonitorTrapFlag(TRUE);
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
 * @return VOID
 */
VOID
BreakpointHandleBpTraps(UINT32 CoreId)
{
    DEBUGGER_TRIGGERED_EVENT_DETAILS ContextAndTag = {0};
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
        // second, the breakpoint is because of 'bp' command, we should
        // replace it with exact byte
        //

        if (!BreakpointCheckAndHandleDebuggerDefinedBreakpoints(DbgState,
                                                                GuestRip,
                                                                DEBUGGEE_PAUSING_REASON_DEBUGGEE_SOFTWARE_BREAKPOINT_HIT,
                                                                FALSE))
        {
            //
            // It's a random breakpoint byte
            //
            ContextAndTag.Context = GuestRip;
            KdHandleBreakpointAndDebugBreakpoints(DbgState,
                                                  DEBUGGEE_PAUSING_REASON_DEBUGGEE_SOFTWARE_BREAKPOINT_HIT,
                                                  &ContextAndTag);

            //
            // Increment rip
            //
            VmFuncPerformRipIncrement(DbgState->CoreId);
        }
    }
    else
    {
        //
        // Don't increment rip
        //
        VmFuncSuppressRipIncrement(DbgState->CoreId);

        //
        // Kernel debugger (debugger-mode) is not attached, re-inject the breakpoint
        //
        EventInjectBreakpoint();
    }
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
    BYTE PreviousByte   = NULL;
    BYTE BreakpointByte = 0xcc; // int 3

    //
    // Check if address is safe (only one byte for 0xcc)
    //
    if (!CheckMemoryAccessSafety(BreakpointDescriptor->Address, sizeof(BYTE)))
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
                                                 &BreakpointByte,
                                                 sizeof(BYTE));

    return TRUE;
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
    BYTE TargetMem = NULL;

    //
    // Check if address is safe (only one byte for 0xcc)
    //
    if (!CheckMemoryAccessSafety(BreakpointDescriptor->Address, sizeof(BYTE)))
    {
        //
        // Double check if we can access it by physical address
        //
        MemoryMapperReadMemorySafeByPhysicalAddress(BreakpointDescriptor->PhysAddress, &TargetMem, sizeof(BYTE));

        if (TargetMem != 0xcc)
        {
            return FALSE;
        }
    }

    //
    // Apply the previous byte
    //
    MemoryMapperWriteMemorySafeByPhysicalAddress(BreakpointDescriptor->PhysAddress,
                                                 &BreakpointDescriptor->PreviousByte,
                                                 sizeof(BYTE));

    //
    // Set breakpoint to disabled
    //
    BreakpointDescriptor->Enabled                = FALSE;
    BreakpointDescriptor->AvoidReApplyBreakpoint = TRUE;

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
        // Clear the breakpoint
        //
        BreakpointClear(CurrentBreakpointDesc);

        //
        // Remove breakpoint from the list of breakpoints
        //
        RemoveEntryList(&CurrentBreakpointDesc->BreakpointsList);

        //
        // Uninitialize the breakpoint descriptor (safely)
        //
        PoolManagerFreePool(CurrentBreakpointDesc);
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
    UINT32                  ProcessorCount;
    CR3_TYPE                GuestCr3;

    //
    // Find the current process cr3
    //
    GuestCr3.Flags = GetRunningCr3OnTargetProcess().Flags;

    //
    // *** Validate arguments ***
    //

    //
    // Check if address is safe (only one byte for 0xcc)
    //
    if (!CheckMemoryAccessSafety(BpDescriptorArg->Address, sizeof(BYTE)))
    {
        BpDescriptorArg->Result = DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_ADDRESS_BASED_ON_CURRENT_PROCESS;
        return FALSE;
    }

    //
    // Check if the core number is not invalid
    //
    ProcessorCount = KeQueryActiveProcessorCount(0);

    if (BpDescriptorArg->Core != DEBUGGEE_BP_APPLY_TO_ALL_CORES &&
        BpDescriptorArg->Core >= ProcessorCount)
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
    BreakpointDescriptor = PoolManagerRequestPool(BREAKPOINT_DEFINITION_STRUCTURE, TRUE, sizeof(DEBUGGEE_BP_DESCRIPTOR));

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
    BreakpointDescriptor->BreakpointId = g_MaximumBreakpointId;
    BreakpointDescriptor->Address      = BpDescriptorArg->Address;
    BreakpointDescriptor->PhysAddress  = VirtualAddressToPhysicalAddressByProcessCr3(BpDescriptorArg->Address,
                                                                                    GuestCr3);
    BreakpointDescriptor->Core         = BpDescriptorArg->Core;
    BreakpointDescriptor->Pid          = BpDescriptorArg->Pid;
    BreakpointDescriptor->Tid          = BpDescriptorArg->Tid;

    //
    // Use length disassembler engine to get the instruction length
    //
    BreakpointDescriptor->InstructionLength = ldisasm(BpDescriptorArg->Address, TRUE);

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
        // Unset the breakpoint
        //
        BreakpointClear(BreakpointDescriptor);

        //
        // Remove breakpoint from the list of breakpoints
        //
        RemoveEntryList(&BreakpointDescriptor->BreakpointsList);

        //
        // Uninitialize the breakpoint descriptor (safely)
        //
        PoolManagerFreePool(BreakpointDescriptor);
    }

    //
    // Operation was successful
    //
    ListOrModifyBreakpoints->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    return TRUE;
}
