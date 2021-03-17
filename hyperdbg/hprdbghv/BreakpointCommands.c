/**
 * @file BreakpointCommands.c
 * @author Sina Karvandi (sina@rayanfam.com)
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
 * @brief Check if the breakpoint vm-exit relates to !epthook command or not
 * 
 * @param CurrentProcessorIndex  
 * @param GuestRip  
 * @param GuestRegs  
 * 
 * @return BOOLEAN
 */
BOOLEAN
BreakpointCheckAndHandleEptHookBreakpoints(UINT32 CurrentProcessorIndex, ULONG GuestRip, PGUEST_REGS GuestRegs)
{
    PLIST_ENTRY TempList           = 0;
    BOOLEAN     IsHandledByEptHook = FALSE;

    //
    // ***** Check breakpoint for !epthook *****
    //

    //
    // Check whether the breakpoint was due to a !epthook command or not
    //
    TempList = &g_EptState->HookedPagesList;

    while (&g_EptState->HookedPagesList != TempList->Flink)
    {
        TempList                            = TempList->Flink;
        PEPT_HOOKED_PAGE_DETAIL HookedEntry = CONTAINING_RECORD(TempList, EPT_HOOKED_PAGE_DETAIL, PageHookList);

        if (HookedEntry->IsExecutionHook)
        {
            for (size_t i = 0; i < HookedEntry->CountOfBreakpoints; i++)
            {
                if (HookedEntry->BreakpointAddresses[i] == GuestRip)
                {
                    //
                    // We found an address that matches the details, let's trigger the event
                    //

                    //
                    // As the context to event trigger, we send the rip
                    // of where triggered this event
                    //
                    DebuggerTriggerEvents(HIDDEN_HOOK_EXEC_CC, GuestRegs, GuestRip);

                    //
                    // Restore to its orginal entry for one instruction
                    //
                    EptSetPML1AndInvalidateTLB(HookedEntry->EntryAddress, HookedEntry->OriginalEntry, INVEPT_SINGLE_CONTEXT);

                    //
                    // Next we have to save the current hooked entry to restore on the next instruction's vm-exit
                    //
                    g_GuestState[KeGetCurrentProcessorNumber()].MtfEptHookRestorePoint = HookedEntry;

                    //
                    // We have to set Monitor trap flag and give it the HookedEntry to work with
                    //
                    HvSetMonitorTrapFlag(TRUE);

                    //
                    // Indicate that we handled the ept violation
                    //
                    IsHandledByEptHook = TRUE;

                    //
                    // Get out of the loop
                    //
                    break;
                }
            }
        }
    }

    //
    // Don't increment rip
    //
    g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;

    return IsHandledByEptHook;
}

/**
 * @brief Check if the breakpoint vm-exit relates to 'bp' command or not
 * 
 * @param CurrentProcessorIndex  
 * @param GuestRip  
 * @param GuestRegs  
 * @param AvoidUnsetMtf  
 * 
 * @return BOOLEAN
 */
BOOLEAN
BreakpointCheckAndHandleDebuggerDefinedBreakpoints(UINT32                  CurrentProcessorIndex,
                                                   UINT64                  GuestRip,
                                                   DEBUGGEE_PAUSING_REASON Reason,
                                                   PGUEST_REGS             GuestRegs,
                                                   PBOOLEAN                AvoidUnsetMtf)
{
    CR3_TYPE                         GuestCr3;
    BOOLEAN                          IsHandledByBpRoutines = FALSE;
    PLIST_ENTRY                      TempList              = 0;
    UINT64                           GuestRipPhysical      = NULL;
    DEBUGGER_TRIGGERED_EVENT_DETAILS ContextAndTag         = {0};
    RFLAGS                           Rflags                = {0};
    ULONG                            LengthOfExitInstr     = 0;
    BYTE                             InstrByte             = NULL;
    *AvoidUnsetMtf                                         = FALSE;

    //
    // ***** Check breakpoint for 'bp' command *****
    //

    //
    // Find the current process cr3
    //
    NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(PsGetCurrentProcess());
    GuestCr3.Flags               = CurrentProcess->DirectoryTableBase;

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
            ContextAndTag.Context = g_GuestState[CurrentProcessorIndex].LastVmexitRip;

            //
            // In breakpoints tag is breakpoint id, not event tag
            //
            if (Reason == DEBUGGEE_PAUSING_REASON_DEBUGGEE_SOFTWARE_BREAKPOINT_HIT)
            {
                ContextAndTag.Tag = CurrentBreakpointDesc->BreakpointId;
            }

            //
            // Hint debuggee about the length
            //
            g_GuestState[CurrentProcessorIndex].DebuggingState.InstructionLengthHint = CurrentBreakpointDesc->InstructionLength;

            //
            // Check constraints
            //
            if ((CurrentBreakpointDesc->Pid == DEBUGGEE_BP_APPLY_TO_ALL_PROCESSES || CurrentBreakpointDesc->Pid == PsGetCurrentProcessId()) &&
                (CurrentBreakpointDesc->Tid == DEBUGGEE_BP_APPLY_TO_ALL_THREADS || CurrentBreakpointDesc->Tid == PsGetCurrentThreadId()) &&
                (CurrentBreakpointDesc->Core == DEBUGGEE_BP_APPLY_TO_ALL_CORES || CurrentBreakpointDesc->Core == CurrentProcessorIndex))
            {
                //
                // *** It's not safe to access CurrentBreakpointDesc anymore as the
                // breakpoint might be removed ***
                //

                KdHandleBreakpointAndDebugBreakpoints(CurrentProcessorIndex,
                                                      GuestRegs,
                                                      Reason,
                                                      &ContextAndTag);
            }

            //
            // Reset hint to instruction length
            //
            g_GuestState[CurrentProcessorIndex].DebuggingState.InstructionLengthHint = 0;

            //
            // Check if we should re-apply the breakpoint after this instruction
            // or not (in other words, is breakpoint still valid)
            //
            if (!CurrentBreakpointDesc->AvoidReApplyBreakpoint)
            {
                //
                // We should re-apply the breakpoint on next mtf
                //
                g_GuestState[CurrentProcessorIndex].DebuggingState.SoftwareBreakpointState = CurrentBreakpointDesc;

                //
                // Fire and MTF
                //
                HvSetMonitorTrapFlag(TRUE);
                *AvoidUnsetMtf = TRUE;

                //
                // As we want to continue debuggee, the MTF might arrive when the
                // host finish executing it's time slice; thus, a clock interrupt
                // or an IPI might be arrived and the next instruction is not what
                // we expect, becuase of that we check if the IF (Interrupt enable)
                // flag of RFLAGS is enabled or not, if enabled then we remove it
                // to avoid any clock-interrupt or IPI to arrive and the next
                // instruction is our next instruction in the current execution
                // context
                //
                __vmx_vmread(GUEST_RFLAGS, &Rflags);

                if (Rflags.InterruptEnableFlag)
                {
                    Rflags.InterruptEnableFlag = FALSE;
                    __vmx_vmwrite(GUEST_RFLAGS, Rflags.Value);

                    //
                    // An indicator to restore RFLAGS if to enabled state
                    //
                    g_GuestState[CurrentProcessorIndex].DebuggingState.SoftwareBreakpointState->SetRflagsIFBitOnMtf = TRUE;
                }
            }

            //
            // Do not increment rip
            //
            g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;

            //
            // No need to iterate anymore
            //
            break;
        }
    }

    return IsHandledByBpRoutines;
}

/**
 * @brief Handle breakpoint vm-exits (#BP) 
 * 
 * @param CurrentProcessorIndex  
 * @param GuestRegs  
 * 
 * @return VOID
 */
VOID
BreakpointHandleBpTraps(UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs)
{
    ULONG64                          GuestRip           = NULL;
    BOOLEAN                          IsHandledByEptHook = FALSE;
    DEBUGGER_TRIGGERED_EVENT_DETAILS ContextAndTag      = {0};
    BOOLEAN                          AvoidUnsetMtf; // not used here

    //
    // Reading guest's RIP
    //
    __vmx_vmread(GUEST_RIP, &GuestRip);

    //
    // Check if it relates to !epthook or not
    //
    IsHandledByEptHook = BreakpointCheckAndHandleEptHookBreakpoints(CurrentProcessorIndex, GuestRip, GuestRegs);

    //
    // re-inject #BP back to the guest if not handled by the hidden breakpoint
    //
    if (!IsHandledByEptHook)
    {
        if (g_KernelDebuggerState)
        {
            //
            // Kernel debugger is attached, let's halt everything
            //

            //
            // A breakpoint triggered and two things might be happened,
            // first, a breakpoint is triggered randomly in the computer and
            // we shouldn't do anything on it (won't change the instruction)
            // second, the breakpoint is because of 'bp' command, we should
            // replace it with exact byte
            //

            if (!BreakpointCheckAndHandleDebuggerDefinedBreakpoints(CurrentProcessorIndex,
                                                                    GuestRip,
                                                                    DEBUGGEE_PAUSING_REASON_DEBUGGEE_SOFTWARE_BREAKPOINT_HIT,
                                                                    GuestRegs,
                                                                    &AvoidUnsetMtf))
            {
                //
                // It's a random breakpoint byte
                //
                ContextAndTag.Context = g_GuestState[CurrentProcessorIndex].LastVmexitRip;
                KdHandleBreakpointAndDebugBreakpoints(CurrentProcessorIndex,
                                                      GuestRegs,
                                                      DEBUGGEE_PAUSING_REASON_DEBUGGEE_SOFTWARE_BREAKPOINT_HIT,
                                                      &ContextAndTag);

                //
                // Increment rip
                //
                g_GuestState[CurrentProcessorIndex].IncrementRip = TRUE;
            }
        }
        else
        {
            //
            // Don't increment rip
            //
            g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;

            //
            // Kernel debugger (debugger-mode) is not attached, re-inject the breakpoint
            //
            EventInjectBreakpoint();
        }
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
    NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(PsGetCurrentProcess());
    GuestCr3.Flags               = CurrentProcess->DirectoryTableBase;

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
        BpDescriptorArg->Result = DEBUGEER_ERROR_INVALID_CORE_ID;
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
    BpDescriptorArg->Result = DEBUGEER_OPERATION_WAS_SUCCESSFULL;

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
            Log("id   address           status\n");
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
    ListOrModifyBreakpoints->Result = DEBUGEER_OPERATION_WAS_SUCCESSFULL;

    return TRUE;
}
