/**
 * @file EptHook.c
 * @author Sina Karvandi (sina@hyperdbg.org)
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
#include "pch.h"

typedef enum _VMX_EXECUTION_MODE
{
    VmxExecutionModeRoot,
    VmxExecutionModeNonRoot,
} VMX_EXECUTION_MODE;

_Must_inspect_result_
inline static VMX_EXECUTION_MODE
GetCurrentVmxExecutionMode()
{
    ULONG                   CurrentCore    = KeGetCurrentProcessorIndex();
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CurrentCore];

    return CurrentVmState->IsOnVmxRootMode ? VmxExecutionModeRoot : VmxExecutionModeNonRoot;
}

/**
 * @brief Check whether the desired PhysicalAddress is already in the g_EptState->HookedPagesList hooks or not
 * 
 * @param PhysicalBaseAddress 
 * @param HookedEntry A pointer to corresponding hook entry in the g_EptState->HookedPagesList
 * 
 * @return TRUE if the address was already hooked, or FALSE  
 */
_Must_inspect_result_
_Success_(return == TRUE)
static BOOLEAN
EptHookFindByPhysAddress(_In_ UINT64 PhysicalBaseAddress,
                         _Out_opt_ EPT_HOOKED_PAGE_DETAIL * HookedEntry)
{
    LIST_FOR_EACH_LINK(g_EptState->HookedPagesList, EPT_HOOKED_PAGE_DETAIL, PageHookList, CurrEntity)
    {
        if (CurrEntity->PhysicalBaseAddress == PhysicalBaseAddress)
        {
            HookedEntry = CurrEntity;
            return TRUE;
        }
    }

    HookedEntry = NULL;
    return FALSE;
}

static UINT64
EptHookCalcBreakpointOffset(_In_ PVOID TargetAddress,
                            _In_ EPT_HOOKED_PAGE_DETAIL * HookedEntry)
{
    UINT64 TargetAddressInFakePageContent;
    UINT64 PageOffset;

    TargetAddressInFakePageContent = &HookedEntry->FakePageContents;
    TargetAddressInFakePageContent = PAGE_ALIGN(TargetAddressInFakePageContent);
    PageOffset                     = PAGE_OFFSET(TargetAddress);
    TargetAddressInFakePageContent = TargetAddressInFakePageContent + PageOffset;

    return TargetAddressInFakePageContent;
}

static BOOLEAN
EptHookCreateHookPage(_In_ PVOID    TargetAddress,
                      _In_ CR3_TYPE ProcessCr3)
{
    EPT_PML1_ENTRY          ChangedEntry;
    INVEPT_DESCRIPTOR       Descriptor;
    SIZE_T                  PhysicalBaseAddress;
    PVOID                   VirtualTarget;
    PVOID                   TargetBuffer;
    UINT64                  TargetAddressInFakePageContent;
    UINT64                  PageOffset;
    PEPT_PML1_ENTRY         TargetPage;
    PEPT_HOOKED_PAGE_DETAIL HookedPage;

    CR3_TYPE Cr3OfCurrentProcess;
    BYTE     OriginalByte;
    BOOLEAN  HookedEntryFound = FALSE;

    //
    // Check whether we are in VMX Root Mode or Not
    //
    ULONG                   CurrentCore    = KeGetCurrentProcessorIndex();
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CurrentCore];

    if (CurrentVmState->IsOnVmxRootMode && !CurrentVmState->HasLaunched)
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
    // Find cr3 of target core
    //
    PhysicalBaseAddress = (SIZE_T)VirtualAddressToPhysicalAddressByProcessCr3(VirtualTarget, ProcessCr3);

    if (!PhysicalBaseAddress)
    {
        DebuggerSetLastError(DEBUGGER_ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    //
    // Set target buffer, request buffer from pool manager,
    // we also need to allocate new page to replace the current page ASAP
    //
    TargetBuffer = PoolManagerRequestPool(SPLIT_2MB_PAGING_TO_4KB_PAGE, TRUE, sizeof(VMM_EPT_DYNAMIC_SPLIT));

    if (!TargetBuffer)
    {
        DebuggerSetLastError(DEBUGGER_ERROR_PRE_ALLOCATED_BUFFER_IS_EMPTY);
        return FALSE;
    }

    if (!EptSplitLargePage(g_EptState->EptPageTable, TargetBuffer, PhysicalBaseAddress, CurrentCore))
    {
        PoolManagerFreePool(TargetBuffer);

        LogDebugInfo("Err, could not split page for the address : 0x%llx", PhysicalBaseAddress);
        DebuggerSetLastError(DEBUGGER_ERROR_EPT_COULD_NOT_SPLIT_THE_LARGE_PAGE_TO_4KB_PAGES);
        return FALSE;
    }

    //
    // Pointer to the page entry in the page table
    //
    TargetPage = EptGetPml1Entry(g_EptState->EptPageTable, PhysicalBaseAddress);

    //
    // Ensure the target is valid
    //
    if (!TargetPage)
    {
        PoolManagerFreePool(TargetBuffer);

        DebuggerSetLastError(DEBUGGER_ERROR_EPT_FAILED_TO_GET_PML1_ENTRY_OF_TARGET_ADDRESS);
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
        PoolManagerFreePool(TargetBuffer);

        DebuggerSetLastError(DEBUGGER_ERROR_PRE_ALLOCATED_BUFFER_IS_EMPTY);
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
    HookedPage->PhysicalBaseAddress = PhysicalBaseAddress;

    //
    // Fake page content physical address
    //
    HookedPage->PhysicalBaseAddressOfFakePageContents = (SIZE_T)VirtualAddressToPhysicalAddress(&HookedPage->FakePageContents[0]) / PAGE_SIZE;

    //
    // Save the entry address
    //
    HookedPage->EntryAddress = TargetPage;

    //
    // Save the original entry
    //
    HookedPage->OriginalEntry = *TargetPage;

    //
    // Show that entry has hidden hooks for execution
    //
    HookedPage->IsExecutionHook = TRUE;

    //
    // Save the address of (first) new breakpoint
    //
    HookedPage->BreakpointAddresses[0] = TargetAddress;

    //
    // Change the counter (set it to 1)
    //
    HookedPage->CountOfBreakpoints = 1;

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
    // Compute new offset of target offset into a safe buffer
    // It will be used to compute the length of the detours
    // address because we might have a user mode code
    //
    TargetAddressInFakePageContent = &HookedPage->FakePageContents;
    TargetAddressInFakePageContent = PAGE_ALIGN(TargetAddressInFakePageContent);
    PageOffset                     = PAGE_OFFSET(TargetAddress);
    TargetAddressInFakePageContent = TargetAddressInFakePageContent + PageOffset;

    //
    // Switch to target process
    //
    Cr3OfCurrentProcess = SwitchOnAnotherProcessMemoryLayoutByCr3(ProcessCr3);

    //
    // Copy the content to the fake page
    // The following line can't be used in user mode addresses
    // RtlCopyBytes(&HookedPage->FakePageContents, VirtualTarget, PAGE_SIZE);
    //
    MemoryMapperReadMemorySafe(VirtualTarget, &HookedPage->FakePageContents, PAGE_SIZE);

    //
    // Set the breakpoint on the fake page
    //
    *(BYTE *)TargetAddressInFakePageContent = 0xcc;

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
    if (!CurrentVmState->HasLaunched)
    {
        //
        // Apply the hook to EPT
        //
        TargetPage->AsUInt = ChangedEntry.AsUInt;
    }
    else
    {
        //
        // Apply the hook to EPT
        //
        EptSetPML1AndInvalidateTLB(TargetPage, ChangedEntry, InveptSingleContext);
    }

    return TRUE;
}

static BOOLEAN
EptHookUpdateHookPage(_In_ PVOID TargetAddress,
                      _Inout_ EPT_HOOKED_PAGE_DETAIL * HookedEntry)
{
    UINT64 TargetAddressInFakePageContent;
    UINT64 PageOffset;
    BYTE   OriginalByte;

    if (HookedEntry == NULL)
        return FALSE;

    //
    // Here we should add the breakpoint to previous breakpoint
    //
    if (HookedEntry->CountOfBreakpoints >= MaximumHiddenBreakpointsOnPage)
    {
        //
        // Means that breakpoint is full and we can't apply this breakpoint
        //
        DebuggerSetLastError(DEBUGGER_ERROR_MAXIMUM_BREAKPOINT_FOR_A_SINGLE_PAGE_IS_HIT);
        return FALSE;
    }

    //
    // Apply the hook 0xcc
    //

    //
    // Compute new offset of target offset into a safe buffer
    // It will be used to compute the length of the detours
    // address because we might have a user mode code
    //
    TargetAddressInFakePageContent = &HookedEntry->FakePageContents;
    TargetAddressInFakePageContent = PAGE_ALIGN(TargetAddressInFakePageContent);
    PageOffset                     = PAGE_OFFSET(TargetAddress);
    TargetAddressInFakePageContent = TargetAddressInFakePageContent + PageOffset;

    //
    // Read the original byte
    //
    OriginalByte = *(BYTE *)TargetAddressInFakePageContent;

    //
    // Set the breakpoint on the fake page
    //
    *(BYTE *)TargetAddressInFakePageContent = 0xcc;

    //
    // Add target address to the list of breakpoints
    //
    HookedEntry->BreakpointAddresses[HookedEntry->CountOfBreakpoints] = TargetAddress;

    //
    // Save the original byte
    //
    HookedEntry->PreviousBytesOnBreakpointAddresses[HookedEntry->CountOfBreakpoints] = OriginalByte;

    //
    // Add to the breakpoint counts
    //
    HookedEntry->CountOfBreakpoints = HookedEntry->CountOfBreakpoints + 1;

    return TRUE;
}

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
    LogInfo("ExAllocatePoolWithTag Called with : Tag = 0x%x , Number Of Bytes = 0x%x , Pool Type = 0x%x ", Tag, NumberOfBytes, PoolType);
    return ExAllocatePoolWithTagOrig(PoolType, NumberOfBytes, Tag);
}

/**
 * @brief The main function that performs EPT page hook with hidden breakpoint
 * @details This function returns false in VMX Non-Root Mode if the VM is already initialized
 * This function have to be called through a VMCALL in VMX Root Mode
 * 
 * @param TargetAddress The address of function or memory address to be hooked
 * @param ProcessCr3 The process cr3 to translate based on that process's cr3
 * @return BOOLEAN Returns true if the hook was successfull or false if there was an error
 */
BOOLEAN
EptHookPerformPageHook(PVOID TargetAddress, CR3_TYPE ProcessCr3)
{
    EPT_PML1_ENTRY          ChangedEntry;
    INVEPT_DESCRIPTOR       Descriptor;
    SIZE_T                  PhysicalBaseAddress;
    PVOID                   VirtualTarget;
    PVOID                   TargetBuffer;
    UINT64                  TargetAddressInFakePageContent;
    UINT64                  PageOffset;
    PEPT_PML1_ENTRY         TargetPage;
    PEPT_HOOKED_PAGE_DETAIL HookedPage;

    ULONG    CurrentCore;
    CR3_TYPE Cr3OfCurrentProcess;
    BYTE     OriginalByte;
    BOOLEAN  HookedEntryFound = FALSE;

    //
    // Check whether we are in VMX Root Mode or Not
    //
    CurrentCore                            = KeGetCurrentProcessorIndex();
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CurrentCore];

    if (CurrentVmState->IsOnVmxRootMode && !CurrentVmState->HasLaunched)
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
    // Find cr3 of target core
    //
    PhysicalBaseAddress = (SIZE_T)VirtualAddressToPhysicalAddressByProcessCr3(VirtualTarget, ProcessCr3);

    if (!PhysicalBaseAddress)
    {
        DebuggerSetLastError(DEBUGGER_ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    //
    // try to see if we can find the address
    //

    EPT_HOOKED_PAGE_DETAIL * HookedEntry = {0};

    if (EptHookFindByPhysAddress(PhysicalBaseAddress, HookedEntry) == TRUE && HookedEntry != NULL)
    {
        return EptHookUpdateHookPage(TargetAddress, HookedEntry);
    }
    else
    {
        return EptHookCreateHookPage(TargetAddress, ProcessCr3);
    }
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
 * @param ProcessId The process id to translate based on that process's cr3
 * @return BOOLEAN Returns true if the hook was successfull or false if there was an error
 */
BOOLEAN
EptHook(PVOID TargetAddress, UINT32 ProcessId)
{
    //
    // Check whether we are in VMX Root Mode or Not
    //
    ULONG                   CurrentCore    = KeGetCurrentProcessorIndex();
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CurrentCore];

    //
    // We won't support this type of breakpoint when VMLAUNCH is not executed
    // take a look at "details" about the function to see why we decide to not
    // support this feature.
    //
    if (CurrentVmState->HasLaunched == FALSE)
        return FALSE;

    //
    // Broadcast to all cores to enable vm-exit for breakpoints (exception bitmaps)
    //
    BroadcastEnableBreakpointExitingOnExceptionBitmapAllCores();

    if (AsmVmxVmcall(VMCALL_SET_HIDDEN_CC_BREAKPOINT, TargetAddress, GetCr3FromProcessId(ProcessId).Flags, NULL) == STATUS_SUCCESS)
    {
        LogDebugInfo("Hidden breakpoint hook applied from VMX Root Mode");

        if (!CurrentVmState->IsOnVmxRootMode)
        {
            //
            // Now we have to notify all the core to invalidate their EPT
            //
            BroadcastNotifyAllToInvalidateEptAllCores();
        }
        else
        {
            LogError("Err, unable to notify all cores to invalidate their TLB caches as you called hook on vmx-root mode");
        }

        return TRUE;
    }

    //
    // sth went wrong as we're here
    //
    return FALSE;
}

/**
 * @brief Remove and Invalidate Hook in TLB (Hidden Detours and if counter of hidden breakpoint is zero)
 * @warning This function won't remove entries from LIST_ENTRY,
 *  just invalidate the paging, use EptHookUnHookSingleAddress instead
 * 
 * 
 * @param PhysicalAddress 
 * @return BOOLEAN Return false if there was an error or returns true if it was successfull
 */
BOOLEAN
EptHookRestoreSingleHookToOrginalEntry(SIZE_T PhysicalAddress)
{
    //
    // Should be called from vmx-root, for calling from vmx non-root use the corresponding VMCALL
    //
    if (!g_GuestState[KeGetCurrentProcessorNumber()].IsOnVmxRootMode)
    {
        return FALSE;
    }

    EPT_HOOKED_PAGE_DETAIL * HookedEntry = {0};

    if (EptHookFindByPhysAddress(PAGE_ALIGN(PhysicalAddress), HookedEntry) == TRUE && HookedEntry != NULL)
    {
        //
        // Undo the hook on the EPT table
        //
        EptSetPML1AndInvalidateTLB(HookedEntry->EntryAddress, HookedEntry->OriginalEntry, InveptSingleContext);

        return TRUE;
    }

    //
    // Nothing found, probably the list is not found
    //
    return FALSE;
}

/**
 * @brief Remove and Invalidate Hook in TLB
 * @warning This function won't remove entries from LIST_ENTRY, just invalidate the paging, use EptHookUnHookAll instead
 * 
 * @return VOID 
 */
VOID
EptHookRestoreAllHooksToOrginalEntry()
{
    //
    // Should be called from vmx-root, for calling from vmx non-root use the corresponding VMCALL
    //
    if (GetCurrentVmxExecutionMode() == VmxExecutionModeNonRoot)
        return;

    LIST_FOR_EACH_LINK(g_EptState->HookedPagesList, EPT_HOOKED_PAGE_DETAIL, PageHookList, CurrEntity)
    {
        //
        // Undo the hook on the EPT table
        //
        EptSetPML1AndInvalidateTLB(CurrEntity->EntryAddress, CurrEntity->OriginalEntry, InveptSingleContext);
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
    // push Lower 4-byte TargetAddress
    //
    TargetBuffer[5] = 0x68;

    //
    // Lower 4-byte TargetAddress
    //
    *((PUINT32)&TargetBuffer[6]) = (UINT32)TargetAddress;

    //
    // mov [rsp+4],High 4-byte TargetAddress
    //
    TargetBuffer[10] = 0xC7;
    TargetBuffer[11] = 0x44;
    TargetBuffer[12] = 0x24;
    TargetBuffer[13] = 0x04;

    //
    // High 4-byte TargetAddress
    //
    *((PUINT32)&TargetBuffer[14]) = (UINT32)(TargetAddress >> 32);

    //
    // ret
    //
    TargetBuffer[18] = 0xC3;
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
    // push Lower 4-byte TargetAddress
    //
    TargetBuffer[0] = 0x68;

    //
    // Lower 4-byte TargetAddress
    //
    *((PUINT32)&TargetBuffer[1]) = (UINT32)TargetAddress;

    //
    // mov [rsp+4],High 4-byte TargetAddress
    //
    TargetBuffer[5] = 0xC7;
    TargetBuffer[6] = 0x44;
    TargetBuffer[7] = 0x24;
    TargetBuffer[8] = 0x04;

    //
    // High 4-byte TargetAddress
    //
    *((PUINT32)&TargetBuffer[9]) = (UINT32)(TargetAddress >> 32);

    //
    // ret
    //
    TargetBuffer[13] = 0xC3;
}

/**
 * @brief Hook ins
 * 
 * @param Hook The details of hooked pages
 * @param ProcessCr3 The target Process CR3
 * @param TargetFunction Target function that needs to be hooked
 * @param TargetFunctionInSafeMemory Target content in the safe memory (used in Length Disassembler Engine)
 * @param HookFunction The function that will be called when hook triggered
 * @return BOOLEAN Returns true if the hook was successful or returns false if it was not successful
 */
BOOLEAN
EptHookInstructionMemory(PEPT_HOOKED_PAGE_DETAIL Hook,
                         CR3_TYPE                ProcessCr3,
                         PVOID                   TargetFunction,
                         PVOID                   TargetFunctionInSafeMemory,
                         PVOID                   HookFunction)
{
    PHIDDEN_HOOKS_DETOUR_DETAILS DetourHookDetails;
    SIZE_T                       SizeOfHookedInstructions;
    SIZE_T                       OffsetIntoPage;
    CR3_TYPE                     Cr3OfCurrentProcess;

    OffsetIntoPage = ADDRMASK_EPT_PML1_OFFSET((SIZE_T)TargetFunction);

    //
    // Log offset
    //
    // LogInfo("OffsetIntoPage: 0x%llx", OffsetIntoPage);
    //

    if ((OffsetIntoPage + 19) > PAGE_SIZE - 1)
    {
        LogError("Err, function extends past a page boundary");
        return FALSE;
    }

    //
    // Determine the number of instructions necessary to overwrite using Length Disassembler Engine
    //
    for (SizeOfHookedInstructions = 0;
         SizeOfHookedInstructions < 19;
         SizeOfHookedInstructions += ldisasm(((UINT64)TargetFunctionInSafeMemory + SizeOfHookedInstructions), TRUE))
    {
        //
        // Get the full size of instructions necessary to copy
        //
    }

    //for (SizeOfHookedInstructions = 0;
    //     SizeOfHookedInstructions < 19;
    //     SizeOfHookedInstructions += ZydisLde(((UINT64)TargetFunctionInSafeMemory + SizeOfHookedInstructions), TRUE))
    //{
    //    //
    //    // Get the full size of instructions necessary to copy
    //    //
    //}

    //
    // For logging purpose
    //
    // LogInfo("Number of bytes of instruction mem: %x", SizeOfHookedInstructions);

    //
    // Build a trampoline
    //

    //
    // Allocate some executable memory for the trampoline
    //
    Hook->Trampoline = PoolManagerRequestPool(EXEC_TRAMPOLINE, TRUE, MAX_EXEC_TRAMPOLINE_SIZE);

    if (!Hook->Trampoline)
    {
        LogError("Err, could not allocate trampoline function buffer");
        return FALSE;
    }

    //
    // Copy the trampoline instructions in
    //

    // Switch to target process
    //
    Cr3OfCurrentProcess = SwitchOnAnotherProcessMemoryLayoutByCr3(ProcessCr3);

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

    //
    //
    //
    // LogInfo("Trampoline: 0x%llx", Hook->Trampoline);
    // LogInfo("HookFunction: 0x%llx", HookFunction);

    //
    // Let the hook function call the original function
    //
    // *OrigFunction = Hook->Trampoline;
    //

    //
    // Create the structure to return for the debugger, we do it here because it's the first
    // function that changes the original function and if our structure is no ready after this
    // function then we probably see BSOD on other cores
    //
    DetourHookDetails                        = PoolManagerRequestPool(DETOUR_HOOK_DETAILS, TRUE, sizeof(HIDDEN_HOOKS_DETOUR_DETAILS));
    DetourHookDetails->HookedFunctionAddress = TargetFunction;
    DetourHookDetails->ReturnAddress         = Hook->Trampoline;

    //
    // Save the address of DetourHookDetails because we want to
    // deallocate it when the hook is finished
    //
    Hook->AddressOfEptHook2sDetourListEntry = DetourHookDetails;

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
 * @param ProcessCr3 The process cr3 to translate based on that process's cr3
 * @param UnsetRead Hook READ Access
 * @param UnsetWrite Hook WRITE Access
 * @param UnsetExecute Hook EXECUTE Access
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
EptHookPerformPageHook2(PVOID    TargetAddress,
                        PVOID    HookFunction,
                        CR3_TYPE ProcessCr3,
                        BOOLEAN  UnsetRead,
                        BOOLEAN  UnsetWrite,
                        BOOLEAN  UnsetExecute)
{
    EPT_PML1_ENTRY          ChangedEntry;
    INVEPT_DESCRIPTOR       Descriptor;
    SIZE_T                  PhysicalBaseAddress;
    PVOID                   VirtualTarget;
    PVOID                   TargetBuffer;
    UINT64                  TargetAddressInSafeMemory;
    UINT64                  PageOffset;
    PEPT_PML1_ENTRY         TargetPage;
    PEPT_HOOKED_PAGE_DETAIL HookedPage;
    ULONG                   LogicalCoreIndex;
    CR3_TYPE                Cr3OfCurrentProcess;
    PLIST_ENTRY             TempList    = 0;
    PEPT_HOOKED_PAGE_DETAIL HookedEntry = NULL;

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
    // Find cr3 of target core
    //
    PhysicalBaseAddress = (SIZE_T)VirtualAddressToPhysicalAddressByProcessCr3(VirtualTarget, ProcessCr3);

    if (!PhysicalBaseAddress)
    {
        DebuggerSetLastError(DEBUGGER_ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    //
    // try to see if we can find the address
    //
    TempList = &g_EptState->HookedPagesList;

    while (&g_EptState->HookedPagesList != TempList->Flink)
    {
        TempList    = TempList->Flink;
        HookedEntry = CONTAINING_RECORD(TempList, EPT_HOOKED_PAGE_DETAIL, PageHookList);

        if (HookedEntry->PhysicalBaseAddress == PhysicalBaseAddress)
        {
            //
            // Means that we find the address and !epthook2 doesn't support
            // multiple breakpoints in on page
            //
            DebuggerSetLastError(DEBUGGER_ERROR_EPT_MULTIPLE_HOOKS_IN_A_SINGLE_PAGE);
            return FALSE;
        }
    }

    //
    // Set target buffer, request buffer from pool manager,
    // we also need to allocate new page to replace the current page ASAP
    //
    TargetBuffer = PoolManagerRequestPool(SPLIT_2MB_PAGING_TO_4KB_PAGE, TRUE, sizeof(VMM_EPT_DYNAMIC_SPLIT));

    if (!TargetBuffer)
    {
        DebuggerSetLastError(DEBUGGER_ERROR_PRE_ALLOCATED_BUFFER_IS_EMPTY);
        return FALSE;
    }

    if (!EptSplitLargePage(g_EptState->EptPageTable, TargetBuffer, PhysicalBaseAddress, LogicalCoreIndex))
    {
        PoolManagerFreePool(TargetBuffer);

        LogDebugInfo("Err, could not split page for the address : 0x%llx", PhysicalBaseAddress);
        DebuggerSetLastError(DEBUGGER_ERROR_EPT_COULD_NOT_SPLIT_THE_LARGE_PAGE_TO_4KB_PAGES);
        return FALSE;
    }

    //
    // Pointer to the page entry in the page table
    //
    TargetPage = EptGetPml1Entry(g_EptState->EptPageTable, PhysicalBaseAddress);

    //
    // Ensure the target is valid
    //
    if (!TargetPage)
    {
        PoolManagerFreePool(TargetBuffer);

        DebuggerSetLastError(DEBUGGER_ERROR_EPT_FAILED_TO_GET_PML1_ENTRY_OF_TARGET_ADDRESS);
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
        PoolManagerFreePool(TargetBuffer);

        DebuggerSetLastError(DEBUGGER_ERROR_PRE_ALLOCATED_BUFFER_IS_EMPTY);
        return FALSE;
    }

    //
    // Save the virtual address
    //
    HookedPage->VirtualAddress = TargetAddress;

    //
    // Save the physical address
    //
    HookedPage->PhysicalBaseAddress = PhysicalBaseAddress;

    //
    // Fake page content physical address
    //
    HookedPage->PhysicalBaseAddressOfFakePageContents = (SIZE_T)VirtualAddressToPhysicalAddress(&HookedPage->FakePageContents[0]) / PAGE_SIZE;

    //
    // Save the entry address
    //
    HookedPage->EntryAddress = TargetPage;

    //
    // Save the original entry
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
        Cr3OfCurrentProcess = SwitchOnAnotherProcessMemoryLayoutByCr3(ProcessCr3);

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
        if (!EptHookInstructionMemory(HookedPage, ProcessCr3, TargetAddress, TargetAddressInSafeMemory, HookFunction))
        {
            PoolManagerFreePool(TargetBuffer);
            PoolManagerFreePool(HookedPage);

            DebuggerSetLastError(DEBUGGER_ERROR_COULD_NOT_BUILD_THE_EPT_HOOK);
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
        TargetPage->AsUInt = ChangedEntry.AsUInt;
    }
    else
    {
        //
        // Apply the hook to EPT
        //
        EptSetPML1AndInvalidateTLB(TargetPage, ChangedEntry, InveptSingleContext);
    }

    return TRUE;
}

/**
 * @brief This function allocates a buffer in VMX Non Root Mode and then invokes a VMCALL to set the hook
 * @details this command uses hidden detours, this NOT be called from vmx-root mode
 *
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
        // In the current design of hyperdbg we use execute-only pages
        // to implement hidden hooks for exec page, so your processor doesn't
        // have this feature and you have to implment it in other ways :(
        //
        return FALSE;
    }

    if (!SetHookForWrite && SetHookForRead)
    {
        //
        // The hidden hook with Write Enable and Read Disabled will cause EPT violation!
        // fixed
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

        if (AsmVmxVmcall(VmcallNumber, TargetAddress, HookFunction, GetCr3FromProcessId(ProcessId).Flags) == STATUS_SUCCESS)
        {
            //
            // Test log
            //
            // LogInfo("Hook applied from VMX Root Mode");
            //

            if (!g_GuestState[LogicalCoreIndex].IsOnVmxRootMode)
            {
                //
                // Now we have to notify all the core to invalidate their EPT
                //
                BroadcastNotifyAllToInvalidateEptAllCores();
            }
            else
            {
                LogInfo("Err, unable to notify all cores to invalidate their TLB "
                        "caches as you called hook on vmx-root mode, however, the "
                        "hook is still works");
            }

            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        if (EptHookPerformPageHook2(TargetAddress,
                                    HookFunction,
                                    GetCr3FromProcessId(ProcessId),
                                    SetHookForRead,
                                    SetHookForWrite,
                                    SetHookForExec) == TRUE)
        {
            LogInfo("Hook applied (VM has not launched)");
            return TRUE;
        }
    }

    //
    // There was a error, we shouldn't reach here
    //
    LogWarning("Err, hook was not applied");

    return FALSE;
}

/**
 * @brief Handles page hooks
 * 
 * @param Regs Guest registers
 * @param HookedEntryDetails The entry that describes the hooked page
 * @param ViolationQualification The exit qualification of vm-exit
 * @param PhysicalAddress The physical address that cause this vm-exit
 * @return BOOLEAN Returns TRUE if the function was hook was handled or returns false 
 * if there was an unexpected ept violation
 */
BOOLEAN
EptHookHandleHookedPage(PGUEST_REGS                          Regs,
                        EPT_HOOKED_PAGE_DETAIL *             HookedEntryDetails,
                        VMX_EXIT_QUALIFICATION_EPT_VIOLATION ViolationQualification,
                        SIZE_T                               PhysicalAddress)
{
    UINT64                      GuestRip;
    UINT64                      ExactAccessedVirtualAddress;
    UINT64                      AlignedVirtualAddress;
    UINT64                      AlignedPhysicalAddress;
    EPT_HOOKS_TEMPORARY_CONTEXT TemporaryContext = {0};

    //
    // Get alignment
    //
    AlignedVirtualAddress  = PAGE_ALIGN(HookedEntryDetails->VirtualAddress);
    AlignedPhysicalAddress = PAGE_ALIGN(PhysicalAddress);

    //
    // Let's read the exact address that was accessed
    //
    ExactAccessedVirtualAddress = AlignedVirtualAddress + PhysicalAddress - AlignedPhysicalAddress;

    //
    // Create the temporary context
    //
    TemporaryContext.PhysicalAddress = PhysicalAddress;
    TemporaryContext.VirtualAddress  = ExactAccessedVirtualAddress;

    if (!ViolationQualification.EptExecutable && ViolationQualification.ExecuteAccess)
    {
        //
        // Reading guest's RIP
        //
        __vmx_vmread(VMCS_GUEST_RIP, &GuestRip);

        //
        // Generally, we should never reach here, we didn't implement HyperDbg like this ;)
        //
        LogError("Err, Guest RIP : 0x%llx tries to execute the page at : 0x%llx", GuestRip, ExactAccessedVirtualAddress);
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
        // Check whether the user-mode module needs to be investigated or not
        //

        //
        // Trigger the event related to Monitor Write
        //
        DebuggerTriggerEvents(HIDDEN_HOOK_WRITE, Regs, &TemporaryContext);

        //
        // And also search the read/write event
        //
        DebuggerTriggerEvents(HIDDEN_HOOK_READ_AND_WRITE, Regs, &TemporaryContext);
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
        DebuggerTriggerEvents(HIDDEN_HOOK_READ, Regs, &TemporaryContext);

        //
        // And also search the read/write event
        //
        DebuggerTriggerEvents(HIDDEN_HOOK_READ_AND_WRITE, Regs, &TemporaryContext);
    }
    else
    {
        //
        // there was an unexpected ept violation
        //
        return FALSE;
    }

    //
    // Means that restore the Entry to the previous state after current instruction executed in the guest
    //
    return TRUE;
}

/**
 * @brief Remove the enrty from g_EptHook2sDetourListHead in the case
 * of !epthook2 details
 * @param Address Address to remove
 * @return BOOLEAN TRUE if successfully removed and false if not found 
 */
BOOLEAN
EptHookRemoveEntryAndFreePoolFromEptHook2sDetourList(UINT64 Address)
{
    //
    // Iterate through the list of hooked pages details to find
    // the entry in the list
    //
    LIST_FOR_EACH_LINK(g_EptHook2sDetourListHead, HIDDEN_HOOKS_DETOUR_DETAILS, OtherHooksList, CurrentHookedDetails)
    {
        if (CurrentHookedDetails->HookedFunctionAddress == Address)
        {
            //
            // We found the address, we should remove it and add it for
            // future deallocation
            //
            RemoveEntryList(&CurrentHookedDetails->OtherHooksList);

            //
            // Free the pool in next ioctl
            //
            if (!PoolManagerFreePool(CurrentHookedDetails))
            {
                LogError("Err, something goes wrong, the pool not found in the list of previously allocated pools by pool manager");
            }
            return TRUE;
        }
    }
    //
    // No entry found !
    //
    return FALSE;
}

/**
 * @brief get the length of active EPT hooks (!epthook and !epthook2)
 * @param IsEptHook2 Whether the length should be for !epthook or !epthook2
 * 
 * @return UINT32 Count of remained breakpoints 
 */
UINT32
EptHookGetCountOfEpthooks(BOOLEAN IsEptHook2)
{
    UINT32 Count = 0;

    LIST_FOR_EACH_LINK(g_EptState->HookedPagesList, EPT_HOOKED_PAGE_DETAIL, PageHookList, HookedEntry)
    {
        if (IsEptHook2)
        {
            if (HookedEntry->IsHiddenBreakpoint == FALSE)
            {
                Count++;
            }
        }
        else
        {
            if (HookedEntry->IsHiddenBreakpoint == TRUE)
            {
                Count++;
            }
        }
    }

    return Count;
}

/**
 * @brief Remove single hook of detours type
 * @details Should be called from vmx non-root
 * 
 * @param HookedEntry entry detail of hooked address
 * @return BOOLEAN If unhook was successful it returns true or if it
 * was not successful returns false
 */
BOOLEAN
EptHookUnHookSingleAddressDetours(PEPT_HOOKED_PAGE_DETAIL HookedEntry)
{
    //
    // Remove it in all the cores
    //
    KeGenericCallDpc(DpcRoutineRemoveHookAndInvalidateSingleEntryOnAllCores, HookedEntry->PhysicalBaseAddress);

    //
    // Now that we removed this hidden detours hook, it is
    // time to remove it from g_EptHook2sDetourListHead
    //
    EptHookRemoveEntryAndFreePoolFromEptHook2sDetourList(HookedEntry->VirtualAddress);

    //
    // remove the entry from the list
    //
    RemoveEntryList(&HookedEntry->PageHookList);

    //
    // we add the hooked entry to the list
    // of pools that will be deallocated on next IOCTL
    //
    if (!PoolManagerFreePool(HookedEntry))
    {
        LogError("Err, something goes wrong, the pool not found in the list of previously allocated pools by pool manager");
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Remove single hook of hidden breakpoint type
 * @details Should be called from vmx non-root
 * 
 * @param HookedEntry entry detail of hooked address
 * @param VirtualAddress virtual address to unhook
 * @return BOOLEAN If unhook was successful it returns true or if it
 * was not successful returns false
 */
BOOLEAN
EptHookUnHookSingleAddressHiddenBreakpoint(PEPT_HOOKED_PAGE_DETAIL HookedEntry, UINT64 VirtualAddress)
{
    UINT64 TargetAddressInFakePageContent;
    UINT64 PageOffset;
    UINT32 CountOfEntriesWithSameAddr = 0;

    //
    // It's a hidden breakpoint (we have to search through an array of addresses)
    // We count it from top to down because if there are two ept hooks at the
    // same address, then the last one has an invalid PreviousByte and this
    // is the HookedEntry that should be remove (not the first one as it has the
    // correct PreviousByte)
    //
    for (size_t i = HookedEntry->CountOfBreakpoints; i-- > 0;)
    {
        if (HookedEntry->BreakpointAddresses[i] == VirtualAddress)
        {
            //
            // Check if it's a single breakpoint
            //
            if (HookedEntry->CountOfBreakpoints == 1)
            {
                //
                // Remove the hook entirely on all cores
                //
                KeGenericCallDpc(DpcRoutineRemoveHookAndInvalidateSingleEntryOnAllCores, HookedEntry->PhysicalBaseAddress);

                //
                // remove the entry from the list
                //
                RemoveEntryList(&HookedEntry->PageHookList);

                //
                // we add the hooked entry to the list
                // of pools that will be deallocated on next IOCTL
                //
                if (!PoolManagerFreePool(HookedEntry))
                {
                    LogError("Err, something goes wrong, the pool not found in the list of previously allocated pools by pool manager");
                }

                //
                // Check if there is any other breakpoints, if no then we have to disalbe
                // exception bitmaps on vm-exits for breakpoint, for this purpose, we have
                // to visit all the entries to see if there is any entries
                //
                if (EptHookGetCountOfEpthooks(FALSE) == 0)
                {
                    //
                    // Did not find any entry, let's disable the breakpoints vm-exits
                    // on exception bitmaps
                    //
                    BroadcastDisableBreakpointExitingOnExceptionBitmapAllCores();
                }

                return TRUE;
            }
            else
            {
                //
                // Set 0xcc to its previous value
                //
                TargetAddressInFakePageContent = &HookedEntry->FakePageContents;
                TargetAddressInFakePageContent = PAGE_ALIGN(TargetAddressInFakePageContent);
                PageOffset                     = PAGE_OFFSET(VirtualAddress);
                TargetAddressInFakePageContent = TargetAddressInFakePageContent + PageOffset;

                //
                // We'll check if there is another hooked address with the same virtual address
                // in the array, then we'll ignore setting the previous bit as previous bit might
                // be modified for the previous command
                //
                for (size_t j = 0; j < HookedEntry->CountOfBreakpoints; j++)
                {
                    if (HookedEntry->BreakpointAddresses[i] == VirtualAddress)
                    {
                        CountOfEntriesWithSameAddr++;
                    }
                }

                if (CountOfEntriesWithSameAddr == 1)
                {
                    //
                    // Set the previous value
                    //
                    *(BYTE *)TargetAddressInFakePageContent = HookedEntry->PreviousBytesOnBreakpointAddresses[i];
                }

                //
                // Remove just that special entry
                // Btw, No need to remove it, it will be replaced automatically
                //
                HookedEntry->BreakpointAddresses[i]                = NULL;
                HookedEntry->PreviousBytesOnBreakpointAddresses[i] = 0x0;

                //
                // all addresses to a lower array index (because one entry is
                // missing and might) be in the middle of the array
                //
                for (size_t j = i; j < MaximumHiddenBreakpointsOnPage - 1; j++)
                {
                    HookedEntry->BreakpointAddresses[j]                = HookedEntry->BreakpointAddresses[j + 1];
                    HookedEntry->PreviousBytesOnBreakpointAddresses[j] = HookedEntry->PreviousBytesOnBreakpointAddresses[j + 1];
                }

                //
                // Decrease the count of breakpoints
                //
                HookedEntry->CountOfBreakpoints = HookedEntry->CountOfBreakpoints - 1;

                return TRUE;
            }
        }
    }

    //
    // If we reach here, sth went
    //
    return FALSE;
}

/**
 * @brief Remove single hook from the hooked pages list and invalidate TLB
 * @details Should be called from vmx non-root
 * 
 * @param VirtualAddress Virtual address to unhook
 * @param PhysAddress Physical address to unhook (optional)
 * @param ProcessId The process id of target process
 * @details in unhooking for some hooks only physical address is availables
 * 
 * @return BOOLEAN If unhook was successful it returns true or if it was not successful returns false
 */
BOOLEAN
EptHookUnHookSingleAddress(UINT64 VirtualAddress, UINT64 PhysAddress, UINT32 ProcessId)
{
    SIZE_T PhysicalAddress;

    //
    // Should be called from vmx non-root
    //
    if (GetCurrentVmxExecutionMode() == VmxExecutionModeRoot)
    {
        return FALSE;
    }

    if (ProcessId == DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES || ProcessId == 0)
    {
        ProcessId = PsGetCurrentProcessId();
    }

    //
    // Check if the physical address is available or not
    //
    if (PhysAddress == NULL)
    {
        PhysicalAddress = PAGE_ALIGN(VirtualAddressToPhysicalAddressByProcessId(VirtualAddress, ProcessId));
    }
    else
    {
        PhysicalAddress = PAGE_ALIGN(PhysAddress);
    }

    LIST_FOR_EACH_LINK(g_EptState->HookedPagesList, EPT_HOOKED_PAGE_DETAIL, PageHookList, CurrEntity)
    {
        //
        // Check if it's a hidden breakpoint or hidden detours
        //
        if (CurrEntity->IsHiddenBreakpoint)
        {
            //
            // It's a hidden breakpoint
            //
            return EptHookUnHookSingleAddressHiddenBreakpoint(CurrEntity, VirtualAddress);
        }
        else
        {
            //
            // It's either a hidden detours or a monitor (read/write) entry
            //
            if (CurrEntity->PhysicalBaseAddress == PhysicalAddress)
            {
                return EptHookUnHookSingleAddressDetours(CurrEntity);
            }
        }
    }

    //
    // Nothing found , probably the list is not found
    //
    return FALSE;
}

/**
 * @brief Remove all hooks from the hooked pages list and invalidate TLB
 * @detailsShould be called from Vmx Non-root
 * 
 * @return VOID 
 */
VOID
EptHookUnHookAll()
{
    //
    // Should be called from vmx non-root
    //
    if (GetCurrentVmxExecutionMode() != VmxExecutionModeNonRoot)
    {
        return;
    }

    //
    // Remove it in all the cores
    //
    KeGenericCallDpc(DpcRoutineRemoveHookAndInvalidateAllEntriesOnAllCores, 0x0);

    //
    // In the case of unhooking all pages, we remove the hooked
    // from EPT table in vmx-root and at last, we need to deallocate
    // it from the buffers
    //

    LIST_FOR_EACH_LINK(g_EptState->HookedPagesList, EPT_HOOKED_PAGE_DETAIL, PageHookList, CurrEntity)
    {
        //
        // Now that we removed this hidden detours hook, it is
        // time to remove it from g_EptHook2sDetourListHead
        // if the hook is detours
        //
        if (!CurrEntity->IsHiddenBreakpoint)
        {
            EptHookRemoveEntryAndFreePoolFromEptHook2sDetourList(CurrEntity->VirtualAddress);
        }

        //
        // As we are in vmx-root here, we add the hooked entry to the list
        // of pools that will be deallocated on next IOCTL
        //
        if (!PoolManagerFreePool(CurrEntity))
        {
            LogError("Err, something goes wrong, the pool not found in the list of previously allocated pools by pool manager");
        }
    }
}
