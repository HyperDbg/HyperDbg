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
    // Read previous byte and save it to the descriptor
    //
    MemoryMapperReadMemorySafeOnTargetProcess(BreakpointDescriptor->Address, &PreviousByte, sizeof(BYTE));

    //
    // Set breakpoint to enabled
    //
    BreakpointDescriptor->Enabled = TRUE;

    //
    // Apply the breakpoint
    //
    MemoryMapperWriteMemorySafeByPhysicalAddress(BreakpointDescriptor->PhysAddress,
                                                 &BreakpointByte,
                                                 sizeof(BYTE),
                                                 PsGetCurrentProcessId());

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
    //
    // Check if address is safe (only one byte for 0xcc)
    //
    if (!CheckMemoryAccessSafety(BreakpointDescriptor->Address, sizeof(BYTE)))
    {
        return FALSE;
    }

    //
    // Apply the previous byte
    //
    MemoryMapperWriteMemorySafeByPhysicalAddress(BreakpointDescriptor->PhysAddress,
                                                 &BreakpointDescriptor->PreviousByte,
                                                 sizeof(BYTE),
                                                 PsGetCurrentProcessId());

    //
    // Set breakpoint to disabled
    //
    BreakpointDescriptor->Enabled = FALSE;

    return TRUE;
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
    BreakpointWrite(BreakpointDescriptor->Address, BreakpointDescriptor);

    //
    // Show that operation was successful
    //
    BpDescriptorArg->Result = DEBUGEER_OPERATION_WAS_SUCCESSFULL;

    return TRUE;
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
