/**
 * @file EptHook2s.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Implementation of different EPT hidden hooks functions
 * @details All the R/W hooks, Execute hooks and hardware register simulators
 * are implemented here
 *  
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

#include <ntifs.h>
#include "Common.h"
#include "Ept.h"
#include "InlineAsm.h"
#include "Logging.h"
#include "Hooks.h"
#include "HypervisorRoutines.h"
#include "Vmcall.h"
#include "GlobalVariables.h"
#include "LengthDisassemblerEngine.h"
#include "DebuggerEvents.h"

/**
 * @brief Hook function that HooksExAllocatePoolWithTag
 * 
 * @param PoolType 
 * @param NumberOfBytes 
 * @param Tag 
 * @return PVOID 
 */
PVOID
ExAllocatePoolWithTagHook(
    POOL_TYPE PoolType,
    SIZE_T    NumberOfBytes,
    ULONG     Tag)
{
    LogInfo("ExAllocatePoolWithTag Called with : Tag = 0x%x , Number Of Bytes = %d , Pool Type = %d ", Tag, NumberOfBytes, PoolType);
    return ExAllocatePoolWithTagOrig(PoolType, NumberOfBytes, Tag);
}

/**
 * @brief The main function that performs EPT page hook with hidden breakpoint
 * @details This function returns false in VMX Non-Root Mode if the VM is already initialized
 * This function have to be called through a VMCALL in VMX Root Mode
 * 
 * @param TargetAddress The address of function or memory address to be hooked
 * @param HookFunction The function that will be called when hook triggered
 * @param ProcessId The process id to translate based on that process's cr3
 * @return BOOLEAN Returns true if the hook was successfull or false if there was an error
 */
BOOLEAN
EptHookPerformPageHook(PVOID TargetAddress, UINT32 ProcessId)
{
    EPT_PML1_ENTRY          ChangedEntry;
    INVEPT_DESCRIPTOR       Descriptor;
    SIZE_T                  PhysicalAddress;
    PVOID                   VirtualTarget;
    PVOID                   TargetBuffer;
    UINT64                  TargetAddressInSafeMemory;
    UINT64                  PageOffset;
    PEPT_PML1_ENTRY         TargetPage;
    PEPT_HOOKED_PAGE_DETAIL HookedPage;
    ULONG                   LogicalCoreIndex;
    CR3_TYPE                Cr3OfCurrentProcess;

    //
    // Check whether we are in VMX Root Mode or Not
    //
    LogicalCoreIndex = KeGetCurrentProcessorIndex();

    if (g_GuestState[LogicalCoreIndex].IsOnVmxRootMode && !g_GuestState[LogicalCoreIndex].HasLaunched)
    {
        return FALSE;
    }

    //
    // Translate the page from a physical address to virtual so we can read its memory.
    // This function will return NULL if the physical address was not already mapped in
    // virtual memory.
    //
    VirtualTarget = PAGE_ALIGN(TargetAddress);

    //
    // Here we have to change the CR3, it is because we are in SYSTEM process
    // and if the target address is not mapped in SYSTEM address space (e.g
    // user mode address of another process) then the translation is invalid
    //

    //
    // Check if process id is equal to DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES
    // or if process id is 0 then we use the cr32 of current process
    //
    if (ProcessId == DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES || ProcessId == 0)
    {
        ProcessId = PsGetCurrentProcessId();
    }

    //
    // Find cr3 of target core
    //
    PhysicalAddress = (SIZE_T)VirtualAddressToPhysicalAddressByProcessId(VirtualTarget, ProcessId);

    if (!PhysicalAddress)
    {
        LogError("Target address could not be mapped to physical memory");
        return FALSE;
    }

    //
    // Set target buffer, request buffer from pool manager,
    // we also need to allocate new page to replace the current page ASAP
    //
    TargetBuffer = PoolManagerRequestPool(SPLIT_2MB_PAGING_TO_4KB_PAGE, TRUE, sizeof(VMM_EPT_DYNAMIC_SPLIT));

    if (!TargetBuffer)
    {
        LogError("There is no pre-allocated buffer available");
        return FALSE;
    }

    if (!EptSplitLargePage(g_EptState->EptPageTable, TargetBuffer, PhysicalAddress, LogicalCoreIndex))
    {
        LogError("Could not split page for the address : 0x%llx", PhysicalAddress);
        return FALSE;
    }

    //
    // Pointer to the page entry in the page table
    //
    TargetPage = EptGetPml1Entry(g_EptState->EptPageTable, PhysicalAddress);

    //
    // Ensure the target is valid
    //
    if (!TargetPage)
    {
        LogError("Failed to get PML1 entry of the target address");
        return FALSE;
    }

    //
    // Save the original permissions of the page
    //
    ChangedEntry = *TargetPage;

    //
    // Save the detail of hooked page to keep track of it
    //
    HookedPage = PoolManagerRequestPool(TRACKING_HOOKED_PAGES, TRUE, sizeof(EPT_HOOKED_PAGE_DETAIL));

    if (!HookedPage)
    {
        LogError("There is no pre-allocated pool for saving hooked page details");
        return FALSE;
    }

    //
    // This is a hidden breakpoint
    //
    HookedPage->IsHiddenBreakpoint = TRUE;

    //
    // Save the virtual address
    //
    HookedPage->VirtualAddress = TargetAddress;

    //
    // Save the physical address
    //
    HookedPage->PhysicalBaseAddress = PhysicalAddress;

    //
    // Fake page content physical address
    //
    HookedPage->PhysicalBaseAddressOfFakePageContents = (SIZE_T)VirtualAddressToPhysicalAddress(&HookedPage->FakePageContents[0]) / PAGE_SIZE;

    //
    // Save the entry address
    //
    HookedPage->EntryAddress = TargetPage;

    //
    // Save the orginal entry
    //
    HookedPage->OriginalEntry = *TargetPage;

    //
    // Show that entry has hidden hooks for execution
    //
    HookedPage->IsExecutionHook = TRUE;

    //
    // In execution hook, we have to make sure to unset read, write because
    // an EPT violation should occur for these cases and we can swap the original page
    //
    ChangedEntry.ReadAccess    = 0;
    ChangedEntry.WriteAccess   = 0;
    ChangedEntry.ExecuteAccess = 1;

    //
    // Also set the current pfn to fake page
    //
    ChangedEntry.PageFrameNumber = HookedPage->PhysicalBaseAddressOfFakePageContents;

    //
    // Compute new offset of target offset into a safe bufferr
    // It will be used to compute the length of the detours
    // address because we might have a user mode code
    //
    TargetAddressInSafeMemory = &HookedPage->FakePageContents;
    TargetAddressInSafeMemory = PAGE_ALIGN(TargetAddressInSafeMemory);
    PageOffset                = PAGE_OFFSET(TargetAddress);
    TargetAddressInSafeMemory = TargetAddressInSafeMemory + PageOffset;

    //
    // Switch to target process
    //
    Cr3OfCurrentProcess = SwitchOnAnotherProcessMemoryLayout(ProcessId);

    //
    // Copy the content to the fake page
    // The following line can't be used in user mode addresses
    // RtlCopyBytes(&HookedPage->FakePageContents, VirtualTarget, PAGE_SIZE);
    //
    MemoryMapperReadMemorySafe(VirtualTarget, &HookedPage->FakePageContents, PAGE_SIZE);

    //
    // Set the breakpoint on the fake page
    //
    *(BYTE *)TargetAddressInSafeMemory = 0xcc;

    //
    // Restore to original process
    //
    RestoreToPreviousProcess(Cr3OfCurrentProcess);

    //
    // Save the modified entry
    //
    HookedPage->ChangedEntry = ChangedEntry;

    //
    // Add it to the list
    //
    InsertHeadList(&g_EptState->HookedPagesList, &(HookedPage->PageHookList));

    //
    // if not launched, there is no need to modify it on a safe environment
    //
    if (!g_GuestState[LogicalCoreIndex].HasLaunched)
    {
        //
        // Apply the hook to EPT
        //
        TargetPage->Flags = ChangedEntry.Flags;
    }
    else
    {
        //
        // Apply the hook to EPT
        //
        EptSetPML1AndInvalidateTLB(TargetPage, ChangedEntry, INVEPT_SINGLE_CONTEXT);
    }

    return TRUE;
}

/**
 * @brief This function allocates a buffer in VMX Non Root Mode and then invokes a VMCALL to set the hook
 *
 * @details this command uses hidden breakpoints (0xcc) to hook, THIS FUNCTION SHOULD BE CALLED WHEN THE 
 * VMLAUNCH ALREADY EXECUTED, it is because, broadcasting to enable exception bitmap for breakpoint is not
 * clear here, if we want to broadcast to enable exception bitmaps on all cores when vmlaunch is not executed 
 * then that's ok but a user might call this function when we didn't configure the vmcs, it's a problem! we 
 * can solve it by giving a hint to vmcs configure function to make it ok for future configuration but that
 * sounds stupid, I think it's better to not support this feature. Btw, debugger won't use this function in
 * the above mentioned method, so we won't have any problem with this :)
 * 
 * @param TargetAddress The address of function or memory address to be hooked
 * @param HookFunction The function that will be called when hook triggered
 * @param ProcessId The process id to translate based on that process's cr3
 * @return BOOLEAN Returns true if the hook was successfull or false if there was an error
 */
BOOLEAN
EptHook(PVOID TargetAddress, UINT32 ProcessId)
{
    PEPT_HOOKED_PAGE_DETAIL HookedPageDetail;
    ULONG                   LogicalCoreIndex;

    //
    // Check whether we are in VMX Root Mode or Not
    //
    LogicalCoreIndex = KeGetCurrentProcessorIndex();

    if (g_GuestState[LogicalCoreIndex].HasLaunched)
    {
        //
        // Broadcast to all cores to enable vm-exit for breakpoints (exception bitmaps)
        //
        DebuggerEventEnableBreakpointExitingOnExceptionBitmapAllCores();

        if (AsmVmxVmcall(VMCALL_SET_HIDDEN_CC_BREAKPOINT, TargetAddress, ProcessId, NULL) == STATUS_SUCCESS)
        {
            LogInfo("Hidden breakpoint hook applied from VMX Root Mode");

            if (!g_GuestState[LogicalCoreIndex].IsOnVmxRootMode)
            {
                //
                // Now we have to notify all the core to invalidate their EPT
                //
                HvNotifyAllToInvalidateEpt();
            }
            else
            {
                LogError("Unable to notify all cores to invalidate their TLB caches as you called hook on vmx-root mode.");
            }

            return TRUE;
        }
    }
    else
    {
        //
        // We won't support this type of breakpoint when VMLAUNCH is not executed
        // take a look at "details" about the function to see why we decide to not
        // support this feature.
        //
        return FALSE;
    }
}

/**
 * @brief Remove and Invalidate Hook in TLB
 * @warning This function won't remove entries from LIST_ENTRY,
 *  just invalidate the paging, use HvPerformPageUnHookSinglePage instead
 * 
 * 
 * @param PhysicalAddress 
 * @return BOOLEAN Return false if there was an error or returns true if it was successfull
 */
BOOLEAN
EptHookUnHookSinglePage(SIZE_T PhysicalAddress)
{
    PLIST_ENTRY TempList = 0;

    //
    // Should be called from vmx-root, for calling from vmx non-root use the corresponding VMCALL
    //
    if (!g_GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode)
    {
        return FALSE;
    }

    TempList = &g_EptState->HookedPagesList;
    while (&g_EptState->HookedPagesList != TempList->Flink)
    {
        TempList                            = TempList->Flink;
        PEPT_HOOKED_PAGE_DETAIL HookedEntry = CONTAINING_RECORD(TempList, EPT_HOOKED_PAGE_DETAIL, PageHookList);
        if (HookedEntry->PhysicalBaseAddress == PAGE_ALIGN(PhysicalAddress))
        {
            //
            // Undo the hook on the EPT table
            //
            EptSetPML1AndInvalidateTLB(HookedEntry->EntryAddress, HookedEntry->OriginalEntry, INVEPT_SINGLE_CONTEXT);
            return TRUE;
        }
    }
    //
    // Nothing found, probably the list is not found
    //
    return FALSE;
}

/**
 * @brief Remove and Invalidate Hook in TLB
 * @warning This function won't remove entries from LIST_ENTRY, just invalidate the paging, use HvPerformPageUnHookAllPages instead
 * 
 * @return VOID 
 */
VOID
EptHookUnHookAllPages()
{
    PLIST_ENTRY TempList = 0;

    //
    // Should be called from vmx-root, for calling from vmx non-root use the corresponding VMCALL
    //
    if (!g_GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode)
    {
        return FALSE;
    }

    TempList = &g_EptState->HookedPagesList;
    while (&g_EptState->HookedPagesList != TempList->Flink)
    {
        TempList                            = TempList->Flink;
        PEPT_HOOKED_PAGE_DETAIL HookedEntry = CONTAINING_RECORD(TempList, EPT_HOOKED_PAGE_DETAIL, PageHookList);

        //
        // Undo the hook on the EPT table
        //
        EptSetPML1AndInvalidateTLB(HookedEntry->EntryAddress, HookedEntry->OriginalEntry, INVEPT_SINGLE_CONTEXT);
    }
}

/**
 * @brief Write an absolute x64 jump to an arbitrary address to a buffer
 * 
 * @param TargetBuffer 
 * @param TargetAddress 
 * @return VOID 
 */
VOID
EptHookWriteAbsoluteJump(PCHAR TargetBuffer, SIZE_T TargetAddress)
{
    //
    // call $ + 5 ; A 64-bit call instruction is still 5 bytes wide!
    //
    TargetBuffer[0] = 0xe8;
    TargetBuffer[1] = 0x00;
    TargetBuffer[2] = 0x00;
    TargetBuffer[3] = 0x00;
    TargetBuffer[4] = 0x00;

    //
    // mov r11, Target
    //
    TargetBuffer[5] = 0x49;
    TargetBuffer[6] = 0xBB;

    //
    // Target
    //
    *((PSIZE_T)&TargetBuffer[7]) = TargetAddress;

    //
    // push r11
    //
    TargetBuffer[15] = 0x41;
    TargetBuffer[16] = 0x53;

    //
    // ret
    //
    TargetBuffer[17] = 0xC3;
}

/**
 * @brief Write an absolute x64 jump to an arbitrary address to a buffer
 * 
 * @param TargetBuffer 
 * @param TargetAddress 
 * @return VOID 
 */
VOID
EptHookWriteAbsoluteJump2(PCHAR TargetBuffer, SIZE_T TargetAddress)
{
    //
    // mov r11, Target
    //
    TargetBuffer[0] = 0x49;
    TargetBuffer[1] = 0xBB;

    //
    // Target
    //
    *((PSIZE_T)&TargetBuffer[2]) = TargetAddress;

    //
    // push r11
    //
    TargetBuffer[10] = 0x41;
    TargetBuffer[11] = 0x53;

    //
    // ret
    //
    TargetBuffer[12] = 0xC3;
}

/**
 * @brief Hook ins
 * 
 * @param Hook The details of hooked pages
 * @param ProcessId The target Process Id
 * @param TargetFunction Target function that needs to be hooked
 * @param TargetFunctionInSafeMemory Target content in the safe memory (used in Length Disassembler Engine)
 * @param HookFunction The function that will be called when hook triggered
 * @return BOOLEAN Returns true if the hook was successfull or returns false if it was not successfull
 */
BOOLEAN
EptHookInstructionMemory(PEPT_HOOKED_PAGE_DETAIL Hook, UINT32 ProcessId, PVOID TargetFunction, PVOID TargetFunctionInSafeMemory, PVOID HookFunction)
{
    PHIDDEN_HOOKS_DETOUR_DETAILS DetourHookDetails;
    SIZE_T                       SizeOfHookedInstructions;
    SIZE_T                       OffsetIntoPage;
    CR3_TYPE                     Cr3OfCurrentProcess;

    OffsetIntoPage = ADDRMASK_EPT_PML1_OFFSET((SIZE_T)TargetFunction);
    LogInfo("OffsetIntoPage: 0x%llx", OffsetIntoPage);

    if ((OffsetIntoPage + 18) > PAGE_SIZE - 1)
    {
        LogError("Function extends past a page boundary. We just don't have the technology to solve this.....");
        return FALSE;
    }

    //
    // Determine the number of instructions necessary to overwrite using Length Disassembler Engine
    //
    for (SizeOfHookedInstructions = 0;
         SizeOfHookedInstructions < 18;
         SizeOfHookedInstructions += ldisasm(((UINT64)TargetFunctionInSafeMemory + SizeOfHookedInstructions), TRUE))
    {
        //
        // Get the full size of instructions necessary to copy
        //
    }
    LogInfo("Number of bytes of instruction mem: %d", SizeOfHookedInstructions);

    //
    // Build a trampoline
    //

    //
    // Allocate some executable memory for the trampoline
    //
    Hook->Trampoline = PoolManagerRequestPool(EXEC_TRAMPOLINE, TRUE, MAX_EXEC_TRAMPOLINE_SIZE);

    if (!Hook->Trampoline)
    {
        LogError("Could not allocate trampoline function buffer.");
        return FALSE;
    }

    //
    // Copy the trampoline instructions in
    //

    // Switch to target process
    //
    Cr3OfCurrentProcess = SwitchOnAnotherProcessMemoryLayout(ProcessId);

    //
    // The following line can't be used in user mode addresses
    // RtlCopyMemory(Hook->Trampoline, TargetFunction, SizeOfHookedInstructions);
    //
    MemoryMapperReadMemorySafe(TargetFunction, Hook->Trampoline, SizeOfHookedInstructions);

    //
    // Restore to original process
    //
    RestoreToPreviousProcess(Cr3OfCurrentProcess);

    //
    // Add the absolute jump back to the original function
    //
    EptHookWriteAbsoluteJump2(&Hook->Trampoline[SizeOfHookedInstructions], (SIZE_T)TargetFunction + SizeOfHookedInstructions);

    LogInfo("Trampoline: 0x%llx", Hook->Trampoline);
    LogInfo("HookFunction: 0x%llx", HookFunction);

    //
    // Let the hook function call the original function
    //
    // *OrigFunction = Hook->Trampoline;

    //
    // Create the structure to return for the debugger, we do it here because it's the first
    // function that changes the original function and if our structure is no ready after this
    // fucntion then we probably see BSOD on other cores
    //
    DetourHookDetails                        = PoolManagerRequestPool(DETOUR_HOOK_DETAILS, TRUE, sizeof(HIDDEN_HOOKS_DETOUR_DETAILS));
    DetourHookDetails->HookedFunctionAddress = TargetFunction;
    DetourHookDetails->ReturnAddress         = Hook->Trampoline;

    //
    // Insert it to the list of hooked pages
    //
    InsertHeadList(&g_EptHook2sDetourListHead, &(DetourHookDetails->OtherHooksList));

    //
    // Write the absolute jump to our shadow page memory to jump to our hook
    //
    EptHookWriteAbsoluteJump(&Hook->FakePageContents[OffsetIntoPage], (SIZE_T)HookFunction);

    return TRUE;
}

/**
 * @brief The main function that performs EPT page hook with hidden detours and monitor
 * @details This function returns false in VMX Non-Root Mode if the VM is already initialized
 * This function have to be called through a VMCALL in VMX Root Mode
 * 
 * @param TargetAddress The address of function or memory address to be hooked
 * @param HookFunction The function that will be called when hook triggered
 * @param ProcessId The process id to translate based on that process's cr3
 * @param UnsetRead Hook READ Access
 * @param UnsetWrite Hook WRITE Access
 * @param UnsetExecute Hook EXECUTE Access
 * @return BOOLEAN Returns true if the hook was successfull or false if there was an error
 */
BOOLEAN
EptHookPerformPageHook2(PVOID TargetAddress, PVOID HookFunction, UINT32 ProcessId, BOOLEAN UnsetRead, BOOLEAN UnsetWrite, BOOLEAN UnsetExecute)
{
    EPT_PML1_ENTRY          ChangedEntry;
    INVEPT_DESCRIPTOR       Descriptor;
    SIZE_T                  PhysicalAddress;
    PVOID                   VirtualTarget;
    PVOID                   TargetBuffer;
    UINT64                  TargetAddressInSafeMemory;
    UINT64                  PageOffset;
    PEPT_PML1_ENTRY         TargetPage;
    PEPT_HOOKED_PAGE_DETAIL HookedPage;
    ULONG                   LogicalCoreIndex;
    CR3_TYPE                Cr3OfCurrentProcess;

    //
    // Check whether we are in VMX Root Mode or Not
    //
    LogicalCoreIndex = KeGetCurrentProcessorIndex();

    if (g_GuestState[LogicalCoreIndex].IsOnVmxRootMode && !g_GuestState[LogicalCoreIndex].HasLaunched)
    {
        return FALSE;
    }

    //
    // Translate the page from a physical address to virtual so we can read its memory.
    // This function will return NULL if the physical address was not already mapped in
    // virtual memory.
    //
    VirtualTarget = PAGE_ALIGN(TargetAddress);

    //
    // Here we have to change the CR3, it is because we are in SYSTEM process
    // and if the target address is not mapped in SYSTEM address space (e.g
    // user mode address of another process) then the translation is invalid
    //

    //
    // Check if process id is equal to DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES
    // or if process id is 0 then we use the cr32 of current process
    //
    if (ProcessId == DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES || ProcessId == 0)
    {
        ProcessId = PsGetCurrentProcessId();
    }

    //
    // Find cr3 of target core
    //
    PhysicalAddress = (SIZE_T)VirtualAddressToPhysicalAddressByProcessId(VirtualTarget, ProcessId);

    if (!PhysicalAddress)
    {
        LogError("Target address could not be mapped to physical memory");
        return FALSE;
    }

    //
    // Set target buffer, request buffer from pool manager,
    // we also need to allocate new page to replace the current page ASAP
    //
    TargetBuffer = PoolManagerRequestPool(SPLIT_2MB_PAGING_TO_4KB_PAGE, TRUE, sizeof(VMM_EPT_DYNAMIC_SPLIT));

    if (!TargetBuffer)
    {
        LogError("There is no pre-allocated buffer available");
        return FALSE;
    }

    if (!EptSplitLargePage(g_EptState->EptPageTable, TargetBuffer, PhysicalAddress, LogicalCoreIndex))
    {
        LogError("Could not split page for the address : 0x%llx", PhysicalAddress);
        return FALSE;
    }

    //
    // Pointer to the page entry in the page table
    //
    TargetPage = EptGetPml1Entry(g_EptState->EptPageTable, PhysicalAddress);

    //
    // Ensure the target is valid
    //
    if (!TargetPage)
    {
        LogError("Failed to get PML1 entry of the target address");
        return FALSE;
    }

    //
    // Save the original permissions of the page
    //
    ChangedEntry = *TargetPage;

    //
    // Execution is treated differently
    //
    if (UnsetRead)
        ChangedEntry.ReadAccess = 0;
    else
        ChangedEntry.ReadAccess = 1;

    if (UnsetWrite)
        ChangedEntry.WriteAccess = 0;
    else
        ChangedEntry.WriteAccess = 1;

    //
    // Save the detail of hooked page to keep track of it
    //
    HookedPage = PoolManagerRequestPool(TRACKING_HOOKED_PAGES, TRUE, sizeof(EPT_HOOKED_PAGE_DETAIL));

    if (!HookedPage)
    {
        LogError("There is no pre-allocated pool for saving hooked page details");
        return FALSE;
    }

    //
    // Save the virtual address
    //
    HookedPage->VirtualAddress = TargetAddress;

    //
    // Save the physical address
    //
    HookedPage->PhysicalBaseAddress = PhysicalAddress;

    //
    // Fake page content physical address
    //
    HookedPage->PhysicalBaseAddressOfFakePageContents = (SIZE_T)VirtualAddressToPhysicalAddress(&HookedPage->FakePageContents[0]) / PAGE_SIZE;

    //
    // Save the entry address
    //
    HookedPage->EntryAddress = TargetPage;

    //
    // Save the orginal entry
    //
    HookedPage->OriginalEntry = *TargetPage;

    //
    // If it's Execution hook then we have to set extra fields
    //
    if (UnsetExecute)
    {
        //
        // Show that entry has hidden hooks for execution
        //
        HookedPage->IsExecutionHook = TRUE;

        //
        // In execution hook, we have to make sure to unset read, write because
        // an EPT violation should occur for these cases and we can swap the original page
        //
        ChangedEntry.ReadAccess    = 0;
        ChangedEntry.WriteAccess   = 0;
        ChangedEntry.ExecuteAccess = 1;

        //
        // Also set the current pfn to fake page
        //
        ChangedEntry.PageFrameNumber = HookedPage->PhysicalBaseAddressOfFakePageContents;

        //
        // Switch to target process
        //
        Cr3OfCurrentProcess = SwitchOnAnotherProcessMemoryLayout(ProcessId);

        //
        // Copy the content to the fake page
        // The following line can't be used in user mode addresses
        // RtlCopyBytes(&HookedPage->FakePageContents, VirtualTarget, PAGE_SIZE);
        //
        MemoryMapperReadMemorySafe(VirtualTarget, &HookedPage->FakePageContents, PAGE_SIZE);

        //
        // Restore to original process
        //
        RestoreToPreviousProcess(Cr3OfCurrentProcess);

        //
        // Compute new offset of target offset into a safe bufferr
        // It will be used to compute the length of the detours
        // address because we might have a user mode code
        //
        TargetAddressInSafeMemory = &HookedPage->FakePageContents;
        TargetAddressInSafeMemory = PAGE_ALIGN(TargetAddressInSafeMemory);
        PageOffset                = PAGE_OFFSET(TargetAddress);
        TargetAddressInSafeMemory = TargetAddressInSafeMemory + PageOffset;

        //
        // Create Hook
        //
        if (!EptHookInstructionMemory(HookedPage, ProcessId, TargetAddress, TargetAddressInSafeMemory, HookFunction))
        {
            LogError("Could not build the hook.");
            return FALSE;
        }
    }

    //
    // Save the modified entry
    //
    HookedPage->ChangedEntry = ChangedEntry;

    //
    // Add it to the list
    //
    InsertHeadList(&g_EptState->HookedPagesList, &(HookedPage->PageHookList));

    //
    // if not launched, there is no need to modify it on a safe environment
    //
    if (!g_GuestState[LogicalCoreIndex].HasLaunched)
    {
        //
        // Apply the hook to EPT
        //
        TargetPage->Flags = ChangedEntry.Flags;
    }
    else
    {
        //
        // Apply the hook to EPT
        //
        EptSetPML1AndInvalidateTLB(TargetPage, ChangedEntry, INVEPT_SINGLE_CONTEXT);
    }

    return TRUE;
}

/**
 * @brief This function allocates a buffer in VMX Non Root Mode and then invokes a VMCALL to set the hook
 * @details this command uses hidden detours
 *
 * @param TargetAddress The address of function or memory address to be hooked
 * @param HookFunction The function that will be called when hook triggered
 * @param ProcessId The process id to translate based on that process's cr3
 * @param SetHookForRead Hook READ Access
 * @param SetHookForWrite Hook WRITE Access
 * @param SetHookForExec Hook EXECUTE Access
 * @return BOOLEAN Returns true if the hook was successfull or false if there was an error
 */
BOOLEAN
EptHook2(PVOID TargetAddress, PVOID HookFunction, UINT32 ProcessId, BOOLEAN SetHookForRead, BOOLEAN SetHookForWrite, BOOLEAN SetHookForExec)
{
    UINT32 PageHookMask = 0;
    ULONG  LogicalCoreIndex;

    //
    // Check for the features to avoid EPT Violation problems
    //
    if (SetHookForExec && !g_ExecuteOnlySupport)
    {
        //
        // In the current design of hyperdbg we use execute-only pages to implement hidden hooks for exec page,
        // so your processor doesn't have this feature and you have to implment it in other ways :(
        //
        return FALSE;
    }

    if (SetHookForWrite && !SetHookForRead)
    {
        //
        // The hidden hook with Write Enable and Read Disabled will cause EPT violation!
        //
        return FALSE;
    }

    //
    // Check whether we are in VMX Root Mode or Not
    //
    LogicalCoreIndex = KeGetCurrentProcessorIndex();

    if (SetHookForRead)
    {
        PageHookMask |= PAGE_ATTRIB_READ;
    }
    if (SetHookForWrite)
    {
        PageHookMask |= PAGE_ATTRIB_WRITE;
    }
    if (SetHookForExec)
    {
        PageHookMask |= PAGE_ATTRIB_EXEC;
    }

    if (PageHookMask == 0)
    {
        //
        // nothing to hook
        //
        return FALSE;
    }

    if (g_GuestState[LogicalCoreIndex].HasLaunched)
    {
        //
        // Move Attribute Mask to the upper 32 bits of the VMCALL Number
        //
        UINT64 VmcallNumber = ((UINT64)PageHookMask) << 32 | VMCALL_CHANGE_PAGE_ATTRIB;

        if (AsmVmxVmcall(VmcallNumber, TargetAddress, HookFunction, ProcessId) == STATUS_SUCCESS)
        {
            LogInfo("Hook applied from VMX Root Mode");
            if (!g_GuestState[LogicalCoreIndex].IsOnVmxRootMode)
            {
                //
                // Now we have to notify all the core to invalidate their EPT
                //
                HvNotifyAllToInvalidateEpt();
            }
            else
            {
                LogError("Unable to notify all cores to invalidate their TLB caches as you called hook on vmx-root mode.");
            }

            return TRUE;
        }
    }
    else
    {
        if (EptHookPerformPageHook2(TargetAddress, HookFunction, ProcessId, SetHookForRead, SetHookForWrite, SetHookForExec) == TRUE)
        {
            LogInfo("[*] Hook applied (VM has not launched)");
            return TRUE;
        }
    }
    LogWarning("Hook not applied");

    return FALSE;
}

/**
 * @brief Handles page hooks
 * 
 * @param HookedEntryDetails The entry that describes the hooked page
 * @param ViolationQualification The exit qualification of vm-exit
 * @param PhysicalAddress The physical address that cause this vm-exit
 * @return BOOLEAN Returns TRUE if the function was hook was handled or returns false 
 * if there was an unexpected ept violation
 */
BOOLEAN
EptHookHandleHookedPage(PGUEST_REGS Regs, EPT_HOOKED_PAGE_DETAIL * HookedEntryDetails, VMX_EXIT_QUALIFICATION_EPT_VIOLATION ViolationQualification, SIZE_T PhysicalAddress)
{
    ULONG64 GuestRip;
    ULONG64 ExactAccessedAddress;
    ULONG64 AlignedVirtualAddress;
    ULONG64 AlignedPhysicalAddress;

    //
    // Get alignment
    //
    AlignedVirtualAddress  = PAGE_ALIGN(HookedEntryDetails->VirtualAddress);
    AlignedPhysicalAddress = PAGE_ALIGN(PhysicalAddress);

    //
    // Let's read the exact address that was accesses
    //
    ExactAccessedAddress = AlignedVirtualAddress + PhysicalAddress - AlignedPhysicalAddress;

    //
    // Reading guest's RIP
    //
    __vmx_vmread(GUEST_RIP, &GuestRip);

    if (!ViolationQualification.EptExecutable && ViolationQualification.ExecuteAccess)
    {
        //
        // Generally, we should never reach here, we didn't implement HyperDbg like this ;)
        //
        LogError("Guest RIP : 0x%llx tries to execute the page at : 0x%llx", GuestRip, ExactAccessedAddress);
    }
    else if (!ViolationQualification.EptWriteable && ViolationQualification.WriteAccess)
    {
        //
        // Test
        //

        //
        // LogInfo("Guest RIP : 0x%llx tries to write on the page at :0x%llx", GuestRip, ExactAccessedAddress);
        //

        //
        // Trigger the event related to Monitor Write
        //
        DebuggerTriggerEvents(HIDDEN_HOOK_WRITE, Regs, PhysicalAddress);

        //
        // And also search the read/write event
        //
        DebuggerTriggerEvents(HIDDEN_HOOK_READ_AND_WRITE, Regs, PhysicalAddress);
    }
    else if (!ViolationQualification.EptReadable && ViolationQualification.ReadAccess)
    {
        //
        // Test
        //

        //
        // LogInfo("Guest RIP : 0x%llx tries to read the page at :0x%llx", GuestRip, ExactAccessedAddress);
        //

        //
        // Trigger the event related to Monitor Read
        //
        DebuggerTriggerEvents(HIDDEN_HOOK_READ, Regs, PhysicalAddress);

        //
        // And also search the read/write event
        //
        DebuggerTriggerEvents(HIDDEN_HOOK_READ_AND_WRITE, Regs, PhysicalAddress);
    }
    else
    {
        //
        // there was an unexpected ept violation
        //
        return FALSE;
    }

    //
    // Restore to its orginal entry for one instruction
    //
    EptSetPML1AndInvalidateTLB(HookedEntryDetails->EntryAddress, HookedEntryDetails->OriginalEntry, INVEPT_SINGLE_CONTEXT);

    //
    // Means that restore the Entry to the previous state after current instruction executed in the guest
    //
    return TRUE;
}
