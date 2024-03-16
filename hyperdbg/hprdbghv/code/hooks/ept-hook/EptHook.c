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

/**
 * @brief Check whether the desired PhysicalAddress is already in the g_EptState->HookedPagesList hooks or not
 *
 * @param PhysicalBaseAddress
 *
 * @return PEPT_HOOKED_PAGE_DETAIL  if the address was already hooked, or FALSE
 */
_Must_inspect_result_
_Success_(return == TRUE)
static EPT_HOOKED_PAGE_DETAIL *
EptHookFindByPhysAddress(_In_ UINT64 PhysicalBaseAddress)
{
    LIST_FOR_EACH_LINK(g_EptState->HookedPagesList, EPT_HOOKED_PAGE_DETAIL, PageHookList, CurrEntity)
    {
        if (CurrEntity->PhysicalBaseAddress == PhysicalBaseAddress)
        {
            return CurrEntity;
        }
    }

    return NULL;
}

/**
 * @brief Calculate the breakpoint offset
 *
 * @param TargetAddress
 * @param HookedEntry
 *
 * @return UINT64
 */
static UINT64
EptHookCalcBreakpointOffset(_In_ PVOID                    TargetAddress,
                            _In_ EPT_HOOKED_PAGE_DETAIL * HookedEntry)
{
    UINT64 TargetAddressInFakePageContent;
    UINT64 PageOffset;

    TargetAddressInFakePageContent = (UINT64)&HookedEntry->FakePageContents;
    TargetAddressInFakePageContent = (UINT64)PAGE_ALIGN(TargetAddressInFakePageContent);
    PageOffset                     = (UINT64)PAGE_OFFSET(TargetAddress);
    TargetAddressInFakePageContent = TargetAddressInFakePageContent + PageOffset;

    return TargetAddressInFakePageContent;
}

/**
 * @brief Reserve pre-allocated pools for EPT hooks
 *
 * @param Count number of hooks
 *
 * @return VOID
 */
VOID
EptHookReservePreallocatedPoolsForEptHooks(UINT32 Count)
{
    ULONG ProcessorsCount;

    //
    // Get number of processors
    //
    ProcessorsCount = KeQueryActiveProcessorCount(0);

    //
    // Request pages to be allocated for converting 2MB to 4KB pages
    // Each core needs its own splitting page-tables
    //
    PoolManagerRequestAllocation(sizeof(VMM_EPT_DYNAMIC_SPLIT), Count * ProcessorsCount, SPLIT_2MB_PAGING_TO_4KB_PAGE);

    //
    // Request pages to be allocated for paged hook details
    //
    PoolManagerRequestAllocation(sizeof(EPT_HOOKED_PAGE_DETAIL), Count, TRACKING_HOOKED_PAGES);

    //
    // Request pages to be allocated for Trampoline of Executable hooked pages
    //
    PoolManagerRequestAllocation(MAX_EXEC_TRAMPOLINE_SIZE, Count, EXEC_TRAMPOLINE);

    //
    // Request pages to be allocated for detour hooked pages details
    //
    PoolManagerRequestAllocation(sizeof(HIDDEN_HOOKS_DETOUR_DETAILS), Count, DETOUR_HOOK_DETAILS);
}

/**
 * @brief Allocate (reserve) extra pages for storing details of page hooks
 * for memory monitor and regular hidden breakpoit exec EPT hooks
 *
 * @param Count
 *
 * @return VOID
 */
VOID
EptHookAllocateExtraHookingPagesForMemoryMonitorsAndExecEptHooks(UINT32 Count)
{
    ULONG ProcessorsCount;

    //
    // Get number of processors
    //
    ProcessorsCount = KeQueryActiveProcessorCount(0);

    //
    // Request pages to be allocated for converting 2MB to 4KB pages
    // Each core needs its own splitting page-tables
    //
    PoolManagerRequestAllocation(sizeof(VMM_EPT_DYNAMIC_SPLIT),
                                 Count * ProcessorsCount,
                                 SPLIT_2MB_PAGING_TO_4KB_PAGE);

    //
    // Request pages to be allocated for paged hook details
    //
    PoolManagerRequestAllocation(sizeof(EPT_HOOKED_PAGE_DETAIL),
                                 Count,
                                 TRACKING_HOOKED_PAGES);
}

/**
 * @brief Create EPT hook for the target page
 *
 * @param VCpu The virtual processor's state
 * @param TargetAddress
 * @param ProcessCr3
 *
 * @return BOOLEAN
 */
static BOOLEAN
EptHookCreateHookPage(_Inout_ VIRTUAL_MACHINE_STATE * VCpu,
                      _In_ PVOID                      TargetAddress,
                      _In_ CR3_TYPE                   ProcessCr3)
{
    ULONG                   ProcessorsCount;
    EPT_PML1_ENTRY          ChangedEntry;
    SIZE_T                  PhysicalBaseAddress;
    PVOID                   VirtualTarget;
    PVOID                   TargetBuffer;
    UINT64                  TargetAddressInFakePageContent;
    PEPT_PML1_ENTRY         TargetPage;
    PEPT_HOOKED_PAGE_DETAIL HookedPage;
    CR3_TYPE                Cr3OfCurrentProcess;

    //
    // Get number of processors
    //
    ProcessorsCount = KeQueryActiveProcessorCount(0);

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
        VmmCallbackSetLastError(DEBUGGER_ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    //
    // Save the detail of hooked page to keep track of it
    //
    HookedPage = (EPT_HOOKED_PAGE_DETAIL *)PoolManagerRequestPool(TRACKING_HOOKED_PAGES, TRUE, sizeof(EPT_HOOKED_PAGE_DETAIL));

    if (!HookedPage)
    {
        VmmCallbackSetLastError(DEBUGGER_ERROR_PRE_ALLOCATED_BUFFER_IS_EMPTY);
        return FALSE;
    }

    //
    // This is a hidden breakpoint
    //
    HookedPage->IsHiddenBreakpoint = TRUE;

    //
    // Save the virtual address
    //
    HookedPage->VirtualAddress = (UINT64)TargetAddress;

    //
    // Save the physical address
    //
    HookedPage->PhysicalBaseAddress = PhysicalBaseAddress;

    //
    // Fake page content physical address
    //
    HookedPage->PhysicalBaseAddressOfFakePageContents = (SIZE_T)VirtualAddressToPhysicalAddress(&HookedPage->FakePageContents[0]) / PAGE_SIZE;

    //
    // Show that entry has hidden hooks for execution
    //
    HookedPage->IsExecutionHook = TRUE;

    //
    // Save the address of (first) new breakpoint
    //
    HookedPage->BreakpointAddresses[0] = (UINT64)TargetAddress;

    //
    // Change the counter (set it to 1)
    //
    HookedPage->CountOfBreakpoints = 1;

    //
    // Compute new offset of target offset into a safe buffer
    // It will be used to compute the length of the detours
    // address because we might have a user mode code
    //
    TargetAddressInFakePageContent = EptHookCalcBreakpointOffset(TargetAddress, HookedPage);

    //
    // Switch to target process
    //
    Cr3OfCurrentProcess = SwitchToProcessMemoryLayoutByCr3(ProcessCr3);

    //
    // Copy the content to the fake page
    // The following line can't be used in user mode addresses
    // RtlCopyBytes(&HookedPage->FakePageContents, VirtualTarget, PAGE_SIZE);
    //
    MemoryMapperReadMemorySafe((UINT64)VirtualTarget, &HookedPage->FakePageContents, PAGE_SIZE);

    //
    // we set the breakpoint on the fake page
    //
    *(BYTE *)TargetAddressInFakePageContent = 0xcc;

    //
    // Restore to original process
    //
    SwitchToPreviousProcess(Cr3OfCurrentProcess);

    //
    // Split the 2MB page-table of each core to 4KB page-table
    //
    for (size_t i = 0; i < ProcessorsCount; i++)
    {
        //
        // Set target buffer, request buffer from pool manager,
        // we also need to allocate new page to replace the current page
        //
        TargetBuffer = (PVOID)PoolManagerRequestPool(SPLIT_2MB_PAGING_TO_4KB_PAGE, TRUE, sizeof(VMM_EPT_DYNAMIC_SPLIT));

        if (!TargetBuffer)
        {
            PoolManagerFreePool((UINT64)HookedPage);

            VmmCallbackSetLastError(DEBUGGER_ERROR_PRE_ALLOCATED_BUFFER_IS_EMPTY);
            return FALSE;
        }

        if (!EptSplitLargePage(g_GuestState[i].EptPageTable, TargetBuffer, PhysicalBaseAddress))
        {
            PoolManagerFreePool((UINT64)HookedPage);
            PoolManagerFreePool((UINT64)TargetBuffer); // Here also other previous pools should be specified, but we forget it for now

            LogDebugInfo("Err, could not split page for the address : 0x%llx", PhysicalBaseAddress);
            VmmCallbackSetLastError(DEBUGGER_ERROR_EPT_COULD_NOT_SPLIT_THE_LARGE_PAGE_TO_4KB_PAGES);
            return FALSE;
        }

        //
        // Pointer to the page entry in the page table
        //
        TargetPage = EptGetPml1Entry(g_GuestState[i].EptPageTable, PhysicalBaseAddress);

        //
        // Ensure the target is valid
        //
        if (!TargetPage)
        {
            PoolManagerFreePool((UINT64)HookedPage);
            PoolManagerFreePool((UINT64)TargetBuffer); // Here also other previous pools should be specified, but we forget it for now

            VmmCallbackSetLastError(DEBUGGER_ERROR_EPT_FAILED_TO_GET_PML1_ENTRY_OF_TARGET_ADDRESS);
            return FALSE;
        }

        //
        // Save the original entry (only one of the page tables as the base original entry
        // is enough)
        //
        HookedPage->OriginalEntry = *TargetPage;

        //
        // Save the original permissions of the page
        //
        ChangedEntry = *TargetPage;

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
        // Only for the first time execution of the loop, we save these details,
        // it is because after this condition, the hook is applied and by applying
        // the hook, we have to make sure that the address is saved g_EptState->HookedPagesList
        // because the hook might be simultaneously triggered from other cores
        //
        if (i == 0)
        {
            //
            // Save the modified entry
            //
            HookedPage->ChangedEntry = ChangedEntry;

            //
            // Add it to the list
            //
            InsertHeadList(&g_EptState->HookedPagesList, &(HookedPage->PageHookList));
        }

        //
        // Apply the hook to EPT
        //
        TargetPage->AsUInt = ChangedEntry.AsUInt;

        //
        // If it's the current core then we invalidate the EPT
        //
        if (VCpu->CoreId == i && g_GuestState[i].HasLaunched)
        {
            EptInveptSingleContext(VCpu->EptPointer.AsUInt);
        }
    }

    return TRUE;
}

/**
 * @brief Update the list of an already hooked page
 *
 * @param TargetAddress
 * @param HookedEntry
 *
 * @return BOOLEAN
 */
static BOOLEAN
EptHookUpdateHookPage(_In_ PVOID                       TargetAddress,
                      _Inout_ EPT_HOOKED_PAGE_DETAIL * HookedEntry)
{
    UINT64 TargetAddressInFakePageContent;
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
        VmmCallbackSetLastError(DEBUGGER_ERROR_MAXIMUM_BREAKPOINT_FOR_A_SINGLE_PAGE_IS_HIT);
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
    TargetAddressInFakePageContent = EptHookCalcBreakpointOffset(TargetAddress, HookedEntry);

    //
    // Read the original byte
    //
    OriginalByte = *(BYTE *)TargetAddressInFakePageContent;

    //
    // Add target address to the list of breakpoints
    //
    HookedEntry->BreakpointAddresses[HookedEntry->CountOfBreakpoints] = (UINT64)TargetAddress;

    //
    // Save the original byte
    //
    HookedEntry->PreviousBytesOnBreakpointAddresses[HookedEntry->CountOfBreakpoints] = OriginalByte;

    //
    // Add to the breakpoint counts
    //
    HookedEntry->CountOfBreakpoints = HookedEntry->CountOfBreakpoints + 1;

    //
    // Once we set every details, now we can apply the breakpoint on the fake page
    // It should be after setting the above details because the 0xcc might be triggered
    // in other cores before we saved the details and it will cause errors as the above
    // details might not be available
    //
    *(BYTE *)TargetAddressInFakePageContent = 0xcc;

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
    LogInfo("ExAllocatePoolWithTag Called with : Tag = 0x%x, Number Of Bytes = 0x%x, Pool Type = 0x%x ",
            Tag,
            NumberOfBytes,
            PoolType);

    return ExAllocatePoolWithTagOrig(PoolType, NumberOfBytes, Tag);
}

/**
 * @brief The main function that performs EPT page hook with hidden breakpoint
 * @details This function returns false in VMX Non-Root Mode if the VM is already initialized
 * This function have to be called through a VMCALL in VMX Root Mode
 *
 * @param VCpu The virtual processor's state
 * @param TargetAddress The address of function or memory address to be hooked
 * @param ProcessCr3 The process cr3 to translate based on that process's cr3
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
EptHookPerformPageHook(VIRTUAL_MACHINE_STATE * VCpu,
                       PVOID                   TargetAddress,
                       CR3_TYPE                ProcessCr3)
{
    SIZE_T                   PhysicalBaseAddress;
    PVOID                    VirtualTarget;
    EPT_HOOKED_PAGE_DETAIL * HookedEntry = NULL;

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
        VmmCallbackSetLastError(DEBUGGER_ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    //
    // try to see if we can find the address
    //

    HookedEntry = EptHookFindByPhysAddress(PhysicalBaseAddress);

    if (HookedEntry != NULL)
    {
        return EptHookUpdateHookPage(TargetAddress, HookedEntry);
    }
    else
    {
        return EptHookCreateHookPage(VCpu, TargetAddress, ProcessCr3);
    }
}

/**
 * @brief This function invokes a VMCALL to set the hook and broadcast the exiting for
 * the breakpoints on exception bitmap
 *
 * @param TargetAddress The address of function or memory address to be hooked
 * @param ProcessId The process id to translate based on that process's cr3
 * @param ApplyDirectlyFromVmxRoot should it be directly applied from VMX-root mode or not
 *
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
EptHookPerformHook(PVOID   TargetAddress,
                   UINT32  ProcessId,
                   BOOLEAN ApplyDirectlyFromVmxRoot)
{
    if (ApplyDirectlyFromVmxRoot)
    {
        DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};

        //
        // Set VMCALL options
        //
        DirectVmcallOptions.OptionalParam1 = (UINT64)TargetAddress;
        DirectVmcallOptions.OptionalParam2 = LayoutGetCurrentProcessCr3().Flags;

        //
        // Perform the direct VMCALL
        //
        if (DirectVmcallSetHiddenBreakpointHook(KeGetCurrentProcessorNumberEx(NULL), &DirectVmcallOptions) == STATUS_SUCCESS)
        {
            LogDebugInfo("Hidden breakpoint hook applied from VMX Root Mode");

            return TRUE;
        }
    }
    else
    {
        //
        // Broadcast to all cores to enable vm-exit for breakpoints (exception bitmaps)
        //
        BroadcastEnableBreakpointExitingOnExceptionBitmapAllCores();

        if (AsmVmxVmcall(VMCALL_SET_HIDDEN_CC_BREAKPOINT,
                         (UINT64)TargetAddress,
                         LayoutGetCr3ByProcessId(ProcessId).Flags,
                         (UINT64)NULL64_ZERO) == STATUS_SUCCESS)
        {
            LogDebugInfo("Hidden breakpoint hook applied from VMX Root Mode");

            //
            // Now we have to notify all the core to invalidate their EPT
            //
            BroadcastNotifyAllToInvalidateEptAllCores();

            return TRUE;
        }
    }

    //
    // sth went wrong as we're here
    //
    return FALSE;
}

/**
 * @brief This function invokes a VMCALL to set the hook and broadcast the exiting for
 * the breakpoints on exception bitmap
 *
 * @details this command uses hidden breakpoints (0xcc) to hook, THIS FUNCTION SHOULD BE CALLED WHEN THE
 * VMLAUNCH ALREADY EXECUTED, it is because, broadcasting to enable exception bitmap for breakpoint is not
 * clear here, if we want to broadcast to enable exception bitmaps on all cores when vmlaunch is not executed
 * then that's ok but a user might call this function when we didn't configure the vmcs, it's a problem! we
 * can solve it by giving a hint to vmcs configure function to make it ok for future configuration but that
 * sounds stupid, I think it's better to not support this feature. Btw, debugger won't use this function in
 * the above mentioned method, so we won't have any problem with this
 * This function should be called from VMX non-root mode
 *
 * @param TargetAddress The address of function or memory address to be hooked
 * @param ProcessId The process id to translate based on that process's cr3
 *
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
EptHook(PVOID TargetAddress, UINT32 ProcessId)
{
    //
    // Should be called from vmx non-root
    //
    if (VmxGetCurrentExecutionMode() == TRUE)
    {
        return FALSE;
    }

    return EptHookPerformHook(TargetAddress, ProcessId, FALSE);
}

/**
 * @brief This function invokes a direct VMCALL to setup the hook
 *
 * @details the caller of this function should make sure to 1) broadcast to
 * all cores to intercept breakpoints (#BPs) and after calling this function
 * 2) the caller should broadcast to all cores to invalidate their EPTPs
 * This function should be called from VMX root-mode
 *
 * @param TargetAddress The address of function or memory address to be hooked
 *
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
EptHookFromVmxRoot(PVOID TargetAddress)
{
    //
    // Should be called from VMX root-mode
    //
    if (VmxGetCurrentExecutionMode() == FALSE)
    {
        return FALSE;
    }

    return EptHookPerformHook(TargetAddress, NULL_ZERO, TRUE);
}

/**
 * @brief Remove and Invalidate Hook in TLB (Hidden Detours and if counter of hidden breakpoint is zero)
 * @warning This function won't remove entries from LIST_ENTRY,
 *  just invalidate the paging, use EptHookUnHookSingleAddress instead
 *
 * @param VCpu The virtual processor's state
 * @param PhysicalAddress
 * @param OriginalEntry
 *
 * @return BOOLEAN Return false if there was an error or returns true if it was successful
 */
BOOLEAN
EptHookRestoreSingleHookToOriginalEntry(VIRTUAL_MACHINE_STATE *     VCpu,
                                        SIZE_T                      PhysicalAddress,
                                        UINT64 /* EPT_PML1_ENTRY */ OriginalEntry)
{
    PEPT_PML1_ENTRY TargetPage;

    //
    // Should be called from vmx-root, for calling from vmx non-root use the corresponding VMCALL
    //
    if (VmxGetCurrentExecutionMode() == FALSE)
    {
        return FALSE;
    }
    //
    // Pointer to the page entry in the page table
    //
    TargetPage = EptGetPml1Entry(VCpu->EptPageTable, PhysicalAddress);

    if (TargetPage != NULL)
    {
        //
        // Apply the hook to EPT
        //
        TargetPage->AsUInt = OriginalEntry;

        //
        // Invalidate EPT Cache
        //
        EptInveptSingleContext(VCpu->EptPointer.AsUInt);

        return TRUE;
    }

    //
    // The PML1 entry not found
    //
    return FALSE;
}

/**
 * @brief Remove and Invalidate Hook in TLB
 * @warning This function won't remove entries from LIST_ENTRY, just invalidate the paging, use EptHookUnHookAll instead
 *
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
EptHookRestoreAllHooksToOriginalEntry(VIRTUAL_MACHINE_STATE * VCpu)
{
    PEPT_PML1_ENTRY TargetPage;

    //
    // Should be called from vmx-root, for calling from vmx non-root use the corresponding VMCALL
    //
    if (VmxGetCurrentExecutionMode() == FALSE)
    {
        return;
    }

    LIST_FOR_EACH_LINK(g_EptState->HookedPagesList, EPT_HOOKED_PAGE_DETAIL, PageHookList, HookedEntry)
    {
        //
        // Pointer to the page entry in the page table
        //
        TargetPage = EptGetPml1Entry(VCpu->EptPageTable, HookedEntry->PhysicalBaseAddress);

        //
        // Apply the hook to EPT
        //
        TargetPage->AsUInt = HookedEntry->OriginalEntry.AsUInt;
    }

    //
    // Invalidate EPT Cache
    //
    EptInveptSingleContext(VCpu->EptPointer.AsUInt);
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
 * @brief Hook instructions
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
    // EPTHOOK2 only supports 64-bit kernel (32-bit LDE is not supported)
    //
    for (SizeOfHookedInstructions = 0;
         SizeOfHookedInstructions < 19;
         SizeOfHookedInstructions += DisassemblerLengthDisassembleEngineInVmxRootOnTargetProcess(
             (PVOID)((UINT64)TargetFunctionInSafeMemory + SizeOfHookedInstructions),
             FALSE))
    {
        //
        // Get the full size of instructions necessary to copy
        //
    }

    // for (SizeOfHookedInstructions = 0;
    //      SizeOfHookedInstructions < 19;
    //      SizeOfHookedInstructions += ZydisLde(((UINT64)TargetFunctionInSafeMemory + SizeOfHookedInstructions), TRUE))
    //{
    //     //
    //     // Get the full size of instructions necessary to copy
    //     //
    // }

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
    Hook->Trampoline = (CHAR *)PoolManagerRequestPool(EXEC_TRAMPOLINE, TRUE, MAX_EXEC_TRAMPOLINE_SIZE);

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
    Cr3OfCurrentProcess = SwitchToProcessMemoryLayoutByCr3(ProcessCr3);

    //
    // The following line can't be used in user mode addresses
    // RtlCopyMemory(Hook->Trampoline, TargetFunction, SizeOfHookedInstructions);
    //
    MemoryMapperReadMemorySafe((UINT64)TargetFunction, Hook->Trampoline, SizeOfHookedInstructions);

    //
    // Restore to original process
    //
    SwitchToPreviousProcess(Cr3OfCurrentProcess);

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
    DetourHookDetails                        = (HIDDEN_HOOKS_DETOUR_DETAILS *)PoolManagerRequestPool(DETOUR_HOOK_DETAILS, TRUE, sizeof(HIDDEN_HOOKS_DETOUR_DETAILS));
    DetourHookDetails->HookedFunctionAddress = TargetFunction;
    DetourHookDetails->ReturnAddress         = Hook->Trampoline;

    //
    // Save the address of DetourHookDetails because we want to
    // deallocate it when the hook is finished
    //
    Hook->AddressOfEptHook2sDetourListEntry = (UINT64)DetourHookDetails;

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
 * @param VCpu The virtual processor's state
 * @param HookingDetails The address of function or memory address to be hooked
 * @param ProcessCr3 The process cr3 to translate based on that process's cr3
 * @param PageHookMask Mask hook of the page
 *
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
EptHookPerformPageHookMonitorAndInlineHook(VIRTUAL_MACHINE_STATE * VCpu,
                                           PVOID                   HookingDetails,
                                           CR3_TYPE                ProcessCr3,
                                           UINT32                  PageHookMask)
{
    ULONG                   ProcessorsCount;
    EPT_PML1_ENTRY          ChangedEntry;
    SIZE_T                  PhysicalBaseAddress;
    PVOID                   AlignedTargetVa;
    PVOID                   TargetBuffer;
    PVOID                   TargetAddress;
    PVOID                   HookFunction;
    UINT64                  TargetAddressInSafeMemory;
    PEPT_PML1_ENTRY         TargetPage;
    PEPT_HOOKED_PAGE_DETAIL HookedPage;
    CR3_TYPE                Cr3OfCurrentProcess;
    PLIST_ENTRY             TempList      = 0;
    PEPT_HOOKED_PAGE_DETAIL HookedEntry   = NULL;
    BOOLEAN                 UnsetExecute  = FALSE;
    BOOLEAN                 UnsetRead     = FALSE;
    BOOLEAN                 UnsetWrite    = FALSE;
    BOOLEAN                 EptHiddenHook = FALSE;

    UnsetRead     = (PageHookMask & PAGE_ATTRIB_READ) ? TRUE : FALSE;
    UnsetWrite    = (PageHookMask & PAGE_ATTRIB_WRITE) ? TRUE : FALSE;
    UnsetExecute  = (PageHookMask & PAGE_ATTRIB_EXEC) ? TRUE : FALSE;
    EptHiddenHook = (PageHookMask & PAGE_ATTRIB_EXEC_HIDDEN_HOOK) ? TRUE : FALSE;

    //
    // Get number of processors
    //
    ProcessorsCount = KeQueryActiveProcessorCount(0);

    //
    // Translate the page from a physical address to virtual so we can read its memory.
    // This function will return NULL if the physical address was not already mapped in
    // virtual memory.
    //
    if (EptHiddenHook)
    {
        TargetAddress = ((EPT_HOOKS_ADDRESS_DETAILS_FOR_EPTHOOK2 *)HookingDetails)->TargetAddress;
    }
    else
    {
        TargetAddress = (PVOID)((EPT_HOOKS_ADDRESS_DETAILS_FOR_MEMORY_MONITOR *)HookingDetails)->StartAddress;
    }

    AlignedTargetVa = PAGE_ALIGN(TargetAddress);

    //
    // Here we have to change the CR3, it is because we are in SYSTEM process
    // and if the target address is not mapped in SYSTEM address space (e.g
    // user mode address of another process) then the translation is invalid
    //

    //
    // based on the CR3 of target core
    //
    PhysicalBaseAddress = (SIZE_T)VirtualAddressToPhysicalAddressByProcessCr3(AlignedTargetVa, ProcessCr3);

    if (!PhysicalBaseAddress)
    {
        VmmCallbackSetLastError(DEBUGGER_ERROR_INVALID_ADDRESS);
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
            VmmCallbackSetLastError(DEBUGGER_ERROR_EPT_MULTIPLE_HOOKS_IN_A_SINGLE_PAGE);
            return FALSE;
        }
    }

    //
    // Save the detail of hooked page to keep track of it
    //
    HookedPage = (EPT_HOOKED_PAGE_DETAIL *)PoolManagerRequestPool(TRACKING_HOOKED_PAGES, TRUE, sizeof(EPT_HOOKED_PAGE_DETAIL));

    if (!HookedPage)
    {
        VmmCallbackSetLastError(DEBUGGER_ERROR_PRE_ALLOCATED_BUFFER_IS_EMPTY);
        return FALSE;
    }

    //
    // Save the virtual address
    //
    HookedPage->VirtualAddress = (UINT64)TargetAddress;

    //
    // Save the physical address
    //
    HookedPage->PhysicalBaseAddress = PhysicalBaseAddress;

    //
    // If it's a monitor hook, then we need to hold the address of the start
    // physical address as well as the end physical address, plus tagging information
    //
    if (!EptHiddenHook)
    {
        //
        // Save the target tag
        //
        HookedPage->HookingTag = ((EPT_HOOKS_ADDRESS_DETAILS_FOR_MEMORY_MONITOR *)HookingDetails)->Tag;

        //
        // Save the start of the target physical address
        //
        HookedPage->StartOfTargetPhysicalAddress = (SIZE_T)VirtualAddressToPhysicalAddressByProcessCr3(
            (PVOID)(((EPT_HOOKS_ADDRESS_DETAILS_FOR_MEMORY_MONITOR *)HookingDetails)->StartAddress),
            ProcessCr3);

        if (!HookedPage->StartOfTargetPhysicalAddress)
        {
            PoolManagerFreePool((UINT64)HookedPage);

            VmmCallbackSetLastError(DEBUGGER_ERROR_INVALID_ADDRESS);
            return FALSE;
        }

        //
        // Save the end of the target physical address
        //
        HookedPage->EndOfTargetPhysicalAddress = (SIZE_T)VirtualAddressToPhysicalAddressByProcessCr3(
            (PVOID)(((EPT_HOOKS_ADDRESS_DETAILS_FOR_MEMORY_MONITOR *)HookingDetails)->EndAddress),
            ProcessCr3);

        if (!HookedPage->EndOfTargetPhysicalAddress)
        {
            PoolManagerFreePool((UINT64)HookedPage);

            VmmCallbackSetLastError(DEBUGGER_ERROR_INVALID_ADDRESS);
            return FALSE;
        }
    }

    //
    // Fake page content physical address
    //
    HookedPage->PhysicalBaseAddressOfFakePageContents = (SIZE_T)VirtualAddressToPhysicalAddress(&HookedPage->FakePageContents[0]) / PAGE_SIZE;

    if (EptHiddenHook)
    {
        //
        // Show that entry has hidden hooks for execution
        //
        HookedPage->IsExecutionHook = TRUE;

        //
        // Switch to target process
        //
        Cr3OfCurrentProcess = SwitchToProcessMemoryLayoutByCr3(ProcessCr3);

        //
        // Copy the content to the fake page
        // The following line can't be used in user mode addresses
        // RtlCopyBytes(&HookedPage->FakePageContents, VirtualTarget, PAGE_SIZE);
        //
        MemoryMapperReadMemorySafe((UINT64)AlignedTargetVa, &HookedPage->FakePageContents, PAGE_SIZE);

        //
        // Restore to original process
        //
        SwitchToPreviousProcess(Cr3OfCurrentProcess);

        //
        // Compute new offset of target offset into a safe bufferr
        // It will be used to compute the length of the detours
        // address because we might have a user mode code
        //
        TargetAddressInSafeMemory = EptHookCalcBreakpointOffset(TargetAddress, HookedPage);

        //
        // Make sure if handler function is valid or if it's default
        // then we set it to the default handler
        //
        if (((EPT_HOOKS_ADDRESS_DETAILS_FOR_EPTHOOK2 *)HookingDetails)->HookFunction == NULL)
        {
            HookFunction = (PVOID)AsmGeneralDetourHook;
        }
        else
        {
            HookFunction = ((EPT_HOOKS_ADDRESS_DETAILS_FOR_EPTHOOK2 *)HookingDetails)->HookFunction;
        }

        //
        // Create Hook
        //
        if (!EptHookInstructionMemory(HookedPage, ProcessCr3, TargetAddress, (PVOID)TargetAddressInSafeMemory, HookFunction))
        {
            PoolManagerFreePool((UINT64)HookedPage);

            VmmCallbackSetLastError(DEBUGGER_ERROR_COULD_NOT_BUILD_THE_EPT_HOOK);
            return FALSE;
        }
    }

    for (size_t i = 0; i < ProcessorsCount; i++)
    {
        //
        // Set target buffer, request buffer from pool manager,
        // we also need to allocate new page to replace the current page
        //
        TargetBuffer = (PVOID)PoolManagerRequestPool(SPLIT_2MB_PAGING_TO_4KB_PAGE, TRUE, sizeof(VMM_EPT_DYNAMIC_SPLIT));

        if (!TargetBuffer)
        {
            PoolManagerFreePool((UINT64)HookedPage);

            VmmCallbackSetLastError(DEBUGGER_ERROR_PRE_ALLOCATED_BUFFER_IS_EMPTY);
            return FALSE;
        }

        if (!EptSplitLargePage(g_GuestState[i].EptPageTable, TargetBuffer, PhysicalBaseAddress))
        {
            PoolManagerFreePool((UINT64)HookedPage);
            PoolManagerFreePool((UINT64)TargetBuffer); // Here also other previous pools should be specified, but we forget it for now

            LogDebugInfo("Err, could not split page for the address : 0x%llx", PhysicalBaseAddress);
            VmmCallbackSetLastError(DEBUGGER_ERROR_EPT_COULD_NOT_SPLIT_THE_LARGE_PAGE_TO_4KB_PAGES);
            return FALSE;
        }

        //
        // Pointer to the page entry in the page table
        //
        TargetPage = EptGetPml1Entry(g_GuestState[i].EptPageTable, PhysicalBaseAddress);

        //
        // Ensure the target is valid
        //
        if (!TargetPage)
        {
            PoolManagerFreePool((UINT64)HookedPage);
            PoolManagerFreePool((UINT64)TargetBuffer); // Here also other previous pools should be specified, but we forget it for now

            VmmCallbackSetLastError(DEBUGGER_ERROR_EPT_FAILED_TO_GET_PML1_ENTRY_OF_TARGET_ADDRESS);
            return FALSE;
        }

        //
        // Save the original entry (only one of the page tables as the base original entry
        // is enough)
        //
        HookedPage->OriginalEntry = *TargetPage;

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

        if (UnsetExecute)
            ChangedEntry.ExecuteAccess = 0;
        else
            ChangedEntry.ExecuteAccess = 1;

        //
        // If it's Execution hook then we have to set extra fields
        //
        if (EptHiddenHook)
        {
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
        }

        //
        // Only for the first time execution of the loop, we save these details,
        // it is because after this condition, the hook is applied and by applying
        // the hook, we have to make sure that the address is saved g_EptState->HookedPagesList
        // because the hook might be simultaneously triggered from other cores
        //
        if (i == 0)
        {
            //
            // Save the modified entry
            //
            HookedPage->ChangedEntry = ChangedEntry;

            //
            // Add it to the list
            //
            InsertHeadList(&g_EptState->HookedPagesList, &(HookedPage->PageHookList));
        }

        //
        // Apply the hook to EPT
        //
        TargetPage->AsUInt = ChangedEntry.AsUInt;

        //
        // If it's the current core then we invalidate the EPT
        //
        if (VCpu->CoreId == i && g_GuestState[i].HasLaunched)
        {
            EptInveptSingleContext(VCpu->EptPointer.AsUInt);
        }
    }

    return TRUE;
}

/**
 * @brief This function allocates a buffer in VMX Non Root Mode and then invokes a VMCALL to set the hook
 * @details this command uses hidden detours, if it calls from
 * VMX root-mode directly, it should also invalidate EPT caches (by the caller)
 *
 * @param VCpu The virtual processor's state
 * @param EptHook2AddressDetails The address details for inline EPT hooks
 * @param MemoryAddressDetails The address details for monitor EPT hooks
 * @param ProcessId The process id to translate based on that process's cr3
 * @param EptHiddenHook2 epthook2 style hook
 * @param ApplyDirectlyFromVmxRoot should it be directly applied from VMX-root mode or not
 *
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
EptHookPerformMemoryOrInlineHook(VIRTUAL_MACHINE_STATE *                        VCpu,
                                 EPT_HOOKS_ADDRESS_DETAILS_FOR_EPTHOOK2 *       EptHook2AddressDetails,
                                 EPT_HOOKS_ADDRESS_DETAILS_FOR_MEMORY_MONITOR * MemoryAddressDetails,
                                 UINT32                                         ProcessId,
                                 BOOLEAN                                        EptHiddenHook2,
                                 BOOLEAN                                        ApplyDirectlyFromVmxRoot)
{
    UINT32 PageHookMask        = 0;
    PVOID  HookDetailsToVmcall = NULL;

    //
    // Check for the features to avoid EPT Violation problems
    //
    if (MemoryAddressDetails != NULL)
    {
        HookDetailsToVmcall = MemoryAddressDetails;

        if (MemoryAddressDetails->SetHookForExec &&
            !g_CompatibilityCheck.ExecuteOnlySupport)
        {
            //
            // In the current design of hyperdbg we use execute-only pages
            // to implement hidden hooks for exec page, so your processor doesn't
            // have this feature and you have to implement it in other ways :(
            //
            return FALSE;
        }

        if (!MemoryAddressDetails->SetHookForWrite && MemoryAddressDetails->SetHookForRead)
        {
            //
            // The hidden hook with Write Enable and Read Disabled will cause EPT violation!
            // fixed
            return FALSE;
        }

        if (MemoryAddressDetails->SetHookForRead)
        {
            PageHookMask |= PAGE_ATTRIB_READ;
        }
        if (MemoryAddressDetails->SetHookForWrite)
        {
            PageHookMask |= PAGE_ATTRIB_WRITE;
        }
        if (MemoryAddressDetails->SetHookForExec)
        {
            PageHookMask |= PAGE_ATTRIB_EXEC;
        }
    }
    else if (EptHook2AddressDetails != NULL)
    {
        HookDetailsToVmcall = EptHook2AddressDetails;

        //
        // Initialize the list of ept hook detours if it's not already initialized
        //
        if (!g_IsEptHook2sDetourListInitialized)
        {
            g_IsEptHook2sDetourListInitialized = TRUE;

            InitializeListHead(&g_EptHook2sDetourListHead);
        }

        if (EptHiddenHook2)
        {
            PageHookMask |= PAGE_ATTRIB_EXEC_HIDDEN_HOOK;
        }
    }
    else
    {
        //
        // No details provided
        //
        return FALSE;
    }

    //
    // Check if mask is valid or not
    //
    if (PageHookMask == 0)
    {
        //
        // nothing to hook
        //
        return FALSE;
    }

    if (ApplyDirectlyFromVmxRoot)
    {
        DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
        DirectVmcallOptions.OptionalParam1           = (UINT64)HookDetailsToVmcall;
        DirectVmcallOptions.OptionalParam2           = PageHookMask;
        DirectVmcallOptions.OptionalParam3           = LayoutGetCurrentProcessCr3().Flags; // Process id is ignored while applied directly

        if (DirectVmcallPerformVmcall(VCpu->CoreId, VMCALL_CHANGE_PAGE_ATTRIB, &DirectVmcallOptions) == STATUS_SUCCESS)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        if (VmxGetCurrentLaunchState())
        {
            if (AsmVmxVmcall(VMCALL_CHANGE_PAGE_ATTRIB,
                             (UINT64)HookDetailsToVmcall,
                             PageHookMask,
                             LayoutGetCr3ByProcessId(ProcessId).Flags) == STATUS_SUCCESS)
            {
                //
                // Test log
                //
                // LogInfo("Hook applied from VMX Root Mode");
                //

                if (VmxGetCurrentExecutionMode() == FALSE)
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
            if (EptHookPerformPageHookMonitorAndInlineHook(VCpu,
                                                           HookDetailsToVmcall,
                                                           LayoutGetCr3ByProcessId(ProcessId),
                                                           PageHookMask) == TRUE)
            {
                LogWarning("Hook applied (VM has not launched)");
                return TRUE;
            }
        }
    }

    //
    // There was a error, we shouldn't reach here
    //
    LogWarning("Err, hook was not applied");

    return FALSE;
}

/**
 * @brief This function applies EPT hook 2 (inline) to the target EPT table
 * @details this function should be called from VMX non-root mode
 *
 * @param VCpu The virtual processor's state
 * @param TargetAddress The address of function or memory address to be hooked
 * @param HookFunction The function that will be called when hook triggered
 * @param ProcessId The process id to translate based on that process's cr3
 *
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
EptHookInlineHook(VIRTUAL_MACHINE_STATE * VCpu,
                  PVOID                   TargetAddress,
                  PVOID                   HookFunction,
                  UINT32                  ProcessId)
{
    EPT_HOOKS_ADDRESS_DETAILS_FOR_EPTHOOK2 HookingDetail = {0};

    //
    // Should be called from vmx non-root
    //
    if (VmxGetCurrentExecutionMode() == TRUE)
    {
        return FALSE;
    }

    //
    // Set the hooking details
    //
    HookingDetail.TargetAddress = TargetAddress;
    HookingDetail.HookFunction  = HookFunction;

    return EptHookPerformMemoryOrInlineHook(VCpu,
                                            &HookingDetail,
                                            NULL,
                                            ProcessId,
                                            TRUE,
                                            FALSE);
}

/**
 * @brief This function applies monitor hooks to the target EPT table
 * @details this function should be called from VMX non-root mode
 *
 * @param VCpu The virtual processor's state
 * @param HookingDetails Monitor hooking details
 * @param ProcessId The process id to translate based on that process's cr3
 *
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
EptHookMonitorHook(VIRTUAL_MACHINE_STATE *                        VCpu,
                   EPT_HOOKS_ADDRESS_DETAILS_FOR_MEMORY_MONITOR * HookingDetails,
                   UINT32                                         ProcessId)
{
    //
    // Should be called from vmx non-root
    //
    if (VmxGetCurrentExecutionMode() == TRUE)
    {
        return FALSE;
    }

    return EptHookPerformMemoryOrInlineHook(VCpu,
                                            NULL,
                                            HookingDetails,
                                            ProcessId,
                                            FALSE,
                                            FALSE);
}

/**
 * @brief This function applies EPT inline to the target EPT table
 * @details this function should be called from VMX root-mode
 *
 * @param VCpu The virtual processor's state
 * @param TargetAddress The address of function or memory address to be hooked
 * @param HookFunction The function that will be called when hook triggered
 *
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
EptHookInlineHookFromVmxRoot(VIRTUAL_MACHINE_STATE * VCpu,
                             PVOID                   TargetAddress,
                             PVOID                   HookFunction)
{
    EPT_HOOKS_ADDRESS_DETAILS_FOR_EPTHOOK2 HookingDetail = {0};

    //
    // Should be called from vmx root-mode
    //
    if (VmxGetCurrentExecutionMode() == FALSE)
    {
        return FALSE;
    }

    //
    // Configure the details
    //
    HookingDetail.TargetAddress = TargetAddress;
    HookingDetail.HookFunction  = HookFunction;

    return EptHookPerformMemoryOrInlineHook(VCpu,
                                            &HookingDetail,
                                            NULL,
                                            NULL_ZERO,
                                            TRUE,
                                            TRUE);
}

/**
 * @brief This function applies EPT monitor hooks to the target EPT table
 * @details this function should be called from VMX root-mode
 *
 * @param VCpu The virtual processor's state
 * @param MemoryAddressDetails details of the target memory
 *
 * @return BOOLEAN Returns true if the hook was successful or false if there was an error
 */
BOOLEAN
EptHookMonitorFromVmxRoot(VIRTUAL_MACHINE_STATE *                        VCpu,
                          EPT_HOOKS_ADDRESS_DETAILS_FOR_MEMORY_MONITOR * MemoryAddressDetails)
{
    //
    // Should be called from vmx root-mode
    //
    if (VmxGetCurrentExecutionMode() == FALSE)
    {
        return FALSE;
    }

    return EptHookPerformMemoryOrInlineHook(VCpu,
                                            NULL,
                                            MemoryAddressDetails,
                                            NULL_ZERO,
                                            FALSE,
                                            TRUE);
}

/**
 * @brief Handles page hooks (trigger events)
 *
 * @param VCpu The virtual processor's state
 * @param HookedEntryDetails The entry that describes the hooked page
 * @param ViolationQualification The exit qualification of vm-exit
 * @param PhysicalAddress The physical address that cause this vm-exit
 * @param LastContext The last (current) context of the execution
 * @param IgnoreReadOrWriteOrExec Whether to ignore the event effects or not
 * @param IsExecViolation Whether it's execution violation or not
 * executing the post triggering of the event or not
 *
 * @return BOOLEAN Returns TRUE if the function was hook was handled or returns false
 * if there was an unexpected ept violation
 */
BOOLEAN
EptHookHandleHookedPage(VIRTUAL_MACHINE_STATE *              VCpu,
                        EPT_HOOKED_PAGE_DETAIL *             HookedEntryDetails,
                        VMX_EXIT_QUALIFICATION_EPT_VIOLATION ViolationQualification,
                        SIZE_T                               PhysicalAddress,
                        EPT_HOOKS_CONTEXT *                  LastContext,
                        BOOLEAN *                            IgnoreReadOrWriteOrExec,
                        BOOLEAN *                            IsExecViolation)
{
    UINT64  ExactAccessedVirtualAddress;
    UINT64  AlignedVirtualAddress;
    UINT64  AlignedPhysicalAddress;
    BOOLEAN IsTriggeringPostEventAllowed = FALSE;

    //
    // Get alignment
    //
    AlignedVirtualAddress  = (UINT64)PAGE_ALIGN(HookedEntryDetails->VirtualAddress);
    AlignedPhysicalAddress = (UINT64)PAGE_ALIGN(PhysicalAddress);

    //
    // Let's read the exact address that was accessed
    //
    ExactAccessedVirtualAddress = AlignedVirtualAddress + PhysicalAddress - AlignedPhysicalAddress;

    //
    // Set the last context
    //
    LastContext->HookingTag      = HookedEntryDetails->HookingTag;
    LastContext->PhysicalAddress = PhysicalAddress;
    LastContext->VirtualAddress  = ExactAccessedVirtualAddress;

    if (!ViolationQualification.EptReadable && ViolationQualification.ReadAccess)
    {
        //
        // LogInfo("Guest RIP : 0x%llx tries to read the page at : 0x%llx", GuestRip, ExactAccessedAddress);
        //

        //
        // Set last violation
        //
        HookedEntryDetails->LastViolation = EPT_HOOKED_LAST_VIOLATION_READ;

        //
        // Trigger the event related to Monitor Read and Monitor Read & Write and
        // Monitor Read & Execute and Monitor Read & Write & Execute
        //
        *IgnoreReadOrWriteOrExec = DispatchEventHiddenHookPageReadWriteExecuteReadPreEvent(VCpu, LastContext, &IsTriggeringPostEventAllowed);

        //
        // It's not an execution violation
        //
        *IsExecViolation = FALSE;
    }
    else if (!ViolationQualification.EptWriteable && ViolationQualification.WriteAccess)
    {
        //
        // LogInfo("Guest RIP : 0x%llx tries to write on the page at : 0x%llx", GuestRip, ExactAccessedAddress);
        //

        //
        // Set last violation
        //
        HookedEntryDetails->LastViolation = EPT_HOOKED_LAST_VIOLATION_WRITE;

        //
        // Trigger the event related to Monitor Write and Monitor Read & Write and
        // Monitor Write & Execute and Monitor Read & Write & Execute
        //
        *IgnoreReadOrWriteOrExec = DispatchEventHiddenHookPageReadWriteExecuteWritePreEvent(VCpu, LastContext, &IsTriggeringPostEventAllowed);

        //
        // It's not an execution violation
        //
        *IsExecViolation = FALSE;
    }
    else if (!ViolationQualification.EptExecutable && ViolationQualification.ExecuteAccess)
    {
        //
        // LogInfo("Guest RIP : 0x%llx tries to execute the page at : 0x%llx", GuestRip, ExactAccessedAddress);
        //

        //
        // Set last violation
        //
        HookedEntryDetails->LastViolation = EPT_HOOKED_LAST_VIOLATION_EXEC;

        //
        // Trigger the event related to Monitor Execute and Monitor Read & Execute and
        // Monitor Write & Execute and Monitor Read & Write & Execute
        //
        *IgnoreReadOrWriteOrExec = DispatchEventHiddenHookPageReadWriteExecuteExecutePreEvent(VCpu, LastContext, &IsTriggeringPostEventAllowed);

        //
        // It's an execution violation
        //
        *IsExecViolation = TRUE;
    }
    else
    {
        //
        // triggering post event is not allowed as it's not valid
        //
        HookedEntryDetails->IsPostEventTriggerAllowed = FALSE;

        //
        // there was an unexpected ept violation
        //
        return FALSE;
    }

    //
    // Set whether the post event trigger is allowed or not
    // If only one event short-circuit a special EPT hook the 'post' mode will be ignored for all of the same events
    //
    if (*IgnoreReadOrWriteOrExec == FALSE)
    {
        HookedEntryDetails->IsPostEventTriggerAllowed = IsTriggeringPostEventAllowed;
    }
    else
    {
        //
        // Ignoring read/write/exec will remove the 'post' event
        //
        HookedEntryDetails->IsPostEventTriggerAllowed = FALSE;
    }

    //
    // Means that restore the Entry to the previous state after current
    // instruction executed in the guest
    //
    return TRUE;
}

/**
 * @brief Remove the entry from g_EptHook2sDetourListHead in the case
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
        if (CurrentHookedDetails->HookedFunctionAddress == (PVOID)Address)
        {
            //
            // We found the address, we should remove it and add it for
            // future deallocation
            //
            RemoveEntryList(&CurrentHookedDetails->OtherHooksList);

            //
            // Free the pool in next ioctl
            //
            if (!PoolManagerFreePool((UINT64)CurrentHookedDetails))
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
 *
 * @param HookedEntry entry detail of hooked address
 * @param ApplyDirectlyFromVmxRoot should it be directly applied from VMX-root mode or not
 * @param TargetUnhookingDetails Target data for the caller to restore EPT entry and
 * invalidate EPT caches. Only when applied in VMX-root mode directly
 *
 * @return BOOLEAN If unhook was successful it returns true or if it
 * was not successful returns false
 */
BOOLEAN
EptHookUnHookSingleAddressDetoursAndMonitor(PEPT_HOOKED_PAGE_DETAIL             HookedEntry,
                                            BOOLEAN                             ApplyDirectlyFromVmxRoot,
                                            EPT_SINGLE_HOOK_UNHOOKING_DETAILS * TargetUnhookingDetails)
{
    //
    // Set the unhooking details
    //
    TargetUnhookingDetails->PhysicalAddress = HookedEntry->PhysicalBaseAddress;
    TargetUnhookingDetails->OriginalEntry   = HookedEntry->OriginalEntry.AsUInt;

    //
    // If applied directly from VMX-root mode, it's the responsibility of the
    // caller to remove the hook and invalidate EPT caches for the target physical address
    //
    if (ApplyDirectlyFromVmxRoot)
    {
        TargetUnhookingDetails->CallerNeedsToRestoreEntryAndInvalidateEpt = TRUE;
    }
    else
    {
        //
        // Remove it in all the cores
        //
        TargetUnhookingDetails->CallerNeedsToRestoreEntryAndInvalidateEpt = FALSE;
        KeGenericCallDpc(DpcRoutineRemoveHookAndInvalidateSingleEntryOnAllCores, TargetUnhookingDetails);
    }

    //
    // Now that we removed this hidden detours hook, it is
    // time to remove it from g_EptHook2sDetourListHead
    //
    if (HookedEntry->IsExecutionHook)
    {
        EptHookRemoveEntryAndFreePoolFromEptHook2sDetourList(HookedEntry->VirtualAddress);
    }

    //
    // remove the entry from the list
    //
    RemoveEntryList(&HookedEntry->PageHookList);

    //
    // we add the hooked entry to the list
    // of pools that will be deallocated on next IOCTL
    //
    if (!PoolManagerFreePool((UINT64)HookedEntry))
    {
        LogError("Err, something goes wrong, the pool not found in the list of previously allocated pools by pool manager");
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Handle vm-exits for Monitor Trap Flag to restore previous state
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
EptHookHandleMonitorTrapFlag(VIRTUAL_MACHINE_STATE * VCpu)
{
    PVOID TargetPage;
    //
    // Pointer to the page entry in the page table
    //
    TargetPage = EptGetPml1Entry(VCpu->EptPageTable, VCpu->MtfEptHookRestorePoint->PhysicalBaseAddress);

    //
    // restore the hooked state
    //
    EptSetPML1AndInvalidateTLB(VCpu,
                               TargetPage,
                               VCpu->MtfEptHookRestorePoint->ChangedEntry,
                               InveptSingleContext);

    //
    // Check to trigger the post event (for events relating the !monitor command
    // and the emulation hardware debug registers)
    //
    if (VCpu->MtfEptHookRestorePoint->IsPostEventTriggerAllowed)
    {
        if (VCpu->MtfEptHookRestorePoint->LastViolation == EPT_HOOKED_LAST_VIOLATION_READ)
        {
            //
            // This is a "read" hook
            //
            DispatchEventHiddenHookPageReadWriteExecReadPostEvent(VCpu,
                                                                  &VCpu->MtfEptHookRestorePoint->LastContextState);
        }
        else if (VCpu->MtfEptHookRestorePoint->LastViolation == EPT_HOOKED_LAST_VIOLATION_WRITE)
        {
            //
            // This is a "write" hook
            //
            DispatchEventHiddenHookPageReadWriteExecWritePostEvent(VCpu,
                                                                   &VCpu->MtfEptHookRestorePoint->LastContextState);
        }
        else if (VCpu->MtfEptHookRestorePoint->LastViolation == EPT_HOOKED_LAST_VIOLATION_EXEC)
        {
            //
            // This is a "execute" hook
            //
            DispatchEventHiddenHookPageReadWriteExecExecutePostEvent(VCpu,
                                                                     &VCpu->MtfEptHookRestorePoint->LastContextState);
        }
    }

    //
    // Check for user-mode attaching mechanisms and callback
    // (we call it here, because this callback might change the EPTP entries and invalidate EPTP)
    //
    VmmCallbackRestoreEptState(VCpu->CoreId);
}

/**
 * @brief Remove single hook of hidden breakpoint type
 *
 * @param HookedEntry entry detail of hooked address
 * @param VirtualAddress virtual address to unhook
 * @param ApplyDirectlyFromVmxRoot should it be directly applied from VMX-root mode or not
 * @param TargetUnhookingDetails Target data for the caller to restore EPT entry and
 * invalidate EPT caches. Only when applied in VMX-root mode directly
 *
 * @return BOOLEAN If unhook was successful it returns true or if it
 * was not successful returns false
 */
BOOLEAN
EptHookUnHookSingleAddressHiddenBreakpoint(PEPT_HOOKED_PAGE_DETAIL             HookedEntry,
                                           UINT64                              VirtualAddress,
                                           BOOLEAN                             ApplyDirectlyFromVmxRoot,
                                           EPT_SINGLE_HOOK_UNHOOKING_DETAILS * TargetUnhookingDetails)
{
    UINT64 TargetAddressInFakePageContent;
    UINT32 CountOfEntriesWithSameAddr = 0;

    //
    // By default, the caller doesn't need to remove #BPs interceptions if directly
    // applied from VMX-root mode
    //
    TargetUnhookingDetails->RemoveBreakpointInterception = FALSE;

    //
    // It's a hidden breakpoint (we have to search through an array of addresses)
    // We count it from top to down because if there are two ept hooks at the
    // same address, then the last one has an invalid PreviousByte and this
    // is the HookedEntry that should be remove (not the first one as it has the
    // correct PreviousByte)
    //
    for (size_t i = 0; i < HookedEntry->CountOfBreakpoints; i++)
    {
        if (HookedEntry->BreakpointAddresses[i] == VirtualAddress)
        {
            //
            // Check if it's a single breakpoint
            //
            if (HookedEntry->CountOfBreakpoints == 1)
            {
                //
                // Set the unhooking details
                //
                TargetUnhookingDetails->PhysicalAddress = HookedEntry->PhysicalBaseAddress;
                TargetUnhookingDetails->OriginalEntry   = HookedEntry->OriginalEntry.AsUInt;

                //
                // If applied directly from VMX-root mode, it's the responsibility of the
                // caller to remove the hook and invalidate EPT caches for the target physical address
                //
                if (ApplyDirectlyFromVmxRoot)
                {
                    //
                    // The caller is responsible for restoring EPT entry and invalidate caches
                    //
                    TargetUnhookingDetails->CallerNeedsToRestoreEntryAndInvalidateEpt = TRUE;
                }
                else
                {
                    //
                    // Remove the hook entirely on all cores
                    //
                    TargetUnhookingDetails->CallerNeedsToRestoreEntryAndInvalidateEpt = FALSE;
                    KeGenericCallDpc(DpcRoutineRemoveHookAndInvalidateSingleEntryOnAllCores, TargetUnhookingDetails);
                }

                //
                // remove the entry from the list
                //
                RemoveEntryList(&HookedEntry->PageHookList);

                //
                // we add the hooked entry to the list
                // of pools that will be deallocated on next IOCTL
                //
                if (!PoolManagerFreePool((UINT64)HookedEntry))
                {
                    LogError("Err, something goes wrong, the pool not found in the list of previously allocated pools by pool manager");
                }

                //
                // Check if there is any other breakpoints, if no then we have to disable
                // exception bitmaps on vm-exits for breakpoint, for this purpose, we have
                // to visit all the entries to see if there is any entries
                //
                if (EptHookGetCountOfEpthooks(FALSE) == 0)
                {
                    //
                    // If applied directly from VMX-root mode, it's the responsibility of the
                    // caller to broadcast to disable breakpoint exceptions on all cores
                    //
                    if (ApplyDirectlyFromVmxRoot)
                    {
                        //
                        // Set whether it was the last hook (and the caller if applied from VMX-root needed
                        // to broadcast to disable #BPs interception on exception bitmaps or not)
                        //
                        TargetUnhookingDetails->RemoveBreakpointInterception = TRUE;
                    }
                    else
                    {
                        //
                        // Did not find any entry, let's disable the breakpoints vm-exits
                        // on exception bitmaps
                        //
                        TargetUnhookingDetails->RemoveBreakpointInterception = FALSE;
                        BroadcastDisableBreakpointExitingOnExceptionBitmapAllCores();
                    }
                }

                return TRUE;
            }
            else
            {
                //
                // Set 0xcc to its previous value
                //
                TargetAddressInFakePageContent = EptHookCalcBreakpointOffset((PVOID)VirtualAddress, HookedEntry);

                //
                // We'll check if there is another hooked address with the same virtual address
                // in the array, then we'll ignore setting the previous bit as previous bit might
                // be modified for the previous command
                //
                for (size_t j = 0; j < HookedEntry->CountOfBreakpoints; j++)
                {
                    if (HookedEntry->BreakpointAddresses[j] == VirtualAddress)
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
                // BTW, No need to remove it, it will be replaced automatically
                //
                HookedEntry->BreakpointAddresses[i]                = NULL64_ZERO;
                HookedEntry->PreviousBytesOnBreakpointAddresses[i] = 0x0;

                //
                // all addresses to a lower array index (because one entry is
                // missing and might) be in the middle of the array
                //
                for (size_t j = i /* IndexToRemove */; j < HookedEntry->CountOfBreakpoints - 1; j++)
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
    // If we reach here, sth went wrong
    //
    return FALSE;
}

/**
 * @brief Remove single hook from the hooked pages list and invalidate TLB
 *
 * @param VirtualAddress Virtual address to unhook
 * @param PhysAddress Physical address to unhook (optional)
 * @param ProcessId The process id of target process
 * (in unhooking for some hooks only physical address is availables)
 * @param ApplyDirectlyFromVmxRoot should it be directly applied from VMX-root mode or not
 * @param TargetUnhookingDetails Target data for the caller to restore EPT entry and
 * invalidate EPT caches. Only when applied in VMX-root mode directly
 *
 * @return BOOLEAN If unhook was successful it returns true or if it was not successful returns false
 */
BOOLEAN
EptHookPerformUnHookSingleAddress(UINT64                              VirtualAddress,
                                  UINT64                              PhysAddress,
                                  UINT32                              ProcessId,
                                  BOOLEAN                             ApplyDirectlyFromVmxRoot,
                                  EPT_SINGLE_HOOK_UNHOOKING_DETAILS * TargetUnhookingDetails)
{
    SIZE_T PhysicalAddress;

    //
    // Once applied directly from VMX-root mode, the process id should be the same process Id
    // on current process
    //
    if (ApplyDirectlyFromVmxRoot || ProcessId == DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES || ProcessId == 0)
    {
        ProcessId = HANDLE_TO_UINT32(PsGetCurrentProcessId());
    }

    //
    // Check if the physical address is available or not
    //
    if (PhysAddress == NULL64_ZERO)
    {
        if (ApplyDirectlyFromVmxRoot)
        {
            PhysicalAddress = (SIZE_T)(PAGE_ALIGN(VirtualAddressToPhysicalAddressOnTargetProcess((PVOID)VirtualAddress)));
        }
        else
        {
            PhysicalAddress = (SIZE_T)(PAGE_ALIGN(VirtualAddressToPhysicalAddressByProcessId((PVOID)VirtualAddress,
                                                                                             ProcessId)));
        }
    }
    else
    {
        PhysicalAddress = (SIZE_T)PAGE_ALIGN(PhysAddress);
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
            for (size_t i = 0; i < CurrEntity->CountOfBreakpoints; i++)
            {
                if (CurrEntity->BreakpointAddresses[i] == VirtualAddress)
                {
                    return EptHookUnHookSingleAddressHiddenBreakpoint(CurrEntity,
                                                                      VirtualAddress,
                                                                      ApplyDirectlyFromVmxRoot,
                                                                      TargetUnhookingDetails);
                }
            }
        }
        else
        {
            //
            // It's either a hidden detours or a monitor (read/write/execute) entry
            //
            if (CurrEntity->PhysicalBaseAddress == PhysicalAddress)
            {
                return EptHookUnHookSingleAddressDetoursAndMonitor(CurrEntity,
                                                                   ApplyDirectlyFromVmxRoot,
                                                                   TargetUnhookingDetails);
            }
        }
    }

    //
    // Nothing found, probably the list is not found
    //
    return FALSE;
}

/**
 * @brief Remove single hook from the hooked pages list and invalidate TLB
 * @details Should be called from VMX non-root
 *
 * @param VirtualAddress Virtual address to unhook
 * @param PhysAddress Physical address to unhook (optional)
 * @param ProcessId The process id of target process
 * @details in unhooking for some hooks only physical address is availables
 *
 * @return BOOLEAN If unhook was successful it returns true or if it was not successful returns false
 */
BOOLEAN
EptHookUnHookSingleAddress(UINT64 VirtualAddress,
                           UINT64 PhysAddress,
                           UINT32 ProcessId)
{
    EPT_SINGLE_HOOK_UNHOOKING_DETAILS TargetUnhookingDetails; // not used

    //
    // Should be called from VMX non-root
    //
    if (VmxGetCurrentExecutionMode() == TRUE)
    {
        return FALSE;
    }

    return EptHookPerformUnHookSingleAddress(VirtualAddress,
                                             PhysAddress,
                                             ProcessId,
                                             FALSE,
                                             &TargetUnhookingDetails);
}

/**
 * @brief Remove single hook from the hooked pages list and invalidate TLB
 * @details Should be called from VMX root-mode
 *
 * @param VirtualAddress Virtual address to unhook
 * @param PhysAddress Physical address to unhook (optional)
 * @param TargetUnhookingDetails Target data for the caller to restore EPT entry and
 * invalidate EPT caches. Only when applied in VMX-root mode directly
 *
 * @return BOOLEAN If unhook was successful it returns true or if it was not successful returns false
 */
BOOLEAN
EptHookUnHookSingleAddressFromVmxRoot(UINT64                              VirtualAddress,
                                      UINT64                              PhysAddress,
                                      EPT_SINGLE_HOOK_UNHOOKING_DETAILS * TargetUnhookingDetails)
{
    //
    // Should be called from VMX root-mode
    //
    if (VmxGetCurrentExecutionMode() == FALSE)
    {
        return FALSE;
    }

    return EptHookPerformUnHookSingleAddress(VirtualAddress,
                                             PhysAddress,
                                             NULL_ZERO,
                                             TRUE,
                                             TargetUnhookingDetails);
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
    if (VmxGetCurrentExecutionMode() == TRUE)
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
        if (!PoolManagerFreePool((UINT64)CurrEntity))
        {
            LogError("Err, something goes wrong, the pool not found in the list of previously allocated pools by pool manager");
        }
    }
}

/**
 * @brief routines to generally handle breakpoint hit for detour
 * @param Regs
 * @param CalledFrom
 *
 * @return PVOID
 */
PVOID
EptHook2GeneralDetourEventHandler(PGUEST_REGS Regs, PVOID CalledFrom)
{
    PLIST_ENTRY       TempList    = 0;
    EPT_HOOKS_CONTEXT TempContext = {0};

    //
    // The RSP register is the at the RCX and we just added (reverse by stack) to it's
    // values by the size of the GUEST_REGS
    //
    Regs->rsp = (UINT64)Regs - sizeof(GUEST_REGS);

    //
    // test
    //

    //
    // LogInfo("Hidden Hooked function Called with : rcx = 0x%llx , rdx = 0x%llx , r8 = 0x%llx ,  r9 = 0x%llx",
    //        Regs->rcx,
    //        Regs->rdx,
    //        Regs->r8,
    //        Regs->r9);
    //

    //
    // Create temporary context
    //
    TempContext.VirtualAddress  = (UINT64)CalledFrom;
    TempContext.PhysicalAddress = VirtualAddressToPhysicalAddress(CalledFrom);

    //
    // Create a temporary VCpu
    //
    VIRTUAL_MACHINE_STATE * VCpu = &g_GuestState[KeGetCurrentProcessorNumberEx(NULL)];

    //
    // Set the register for the temporary VCpu
    //
    VCpu->Regs = Regs;

    //
    // As the context to event trigger, we send the address of function
    // which is current hidden hook is triggered for it
    //
    DispatchEventHiddenHookExecDetours(VCpu, &TempContext);

    //
    // Iterate through the list of hooked pages details to find
    // and return where want to jump after this functions
    //
    TempList = &g_EptHook2sDetourListHead;

    while (&g_EptHook2sDetourListHead != TempList->Flink)
    {
        TempList                                          = TempList->Flink;
        PHIDDEN_HOOKS_DETOUR_DETAILS CurrentHookedDetails = CONTAINING_RECORD(TempList, HIDDEN_HOOKS_DETOUR_DETAILS, OtherHooksList);

        if (CurrentHookedDetails->HookedFunctionAddress == CalledFrom)
        {
            return CurrentHookedDetails->ReturnAddress;
        }
    }

    //
    // If we reach here, means that we didn't find the return address
    // that's an error, generally we can't do anything but as the user
    // might already cleaned the hook and the structures are removed
    // so we just return the original caller address and continue the
    // guest normally
    //
    return CalledFrom;
}

/**
 * @brief Change PML EPT state for execution (execute)
 * @detail should be called from VMX-root
 *
 * @param VCpu The virtual processor's state
 * @param PhysicalAddress Target physical address
 * @param IsUnset Is unsetting bit or setting bit
 *
 * @return BOOLEAN
 */
BOOLEAN
EptHookModifyInstructionFetchState(VIRTUAL_MACHINE_STATE * VCpu,
                                   PVOID                   PhysicalAddress,
                                   BOOLEAN                 IsUnset)
{
    PVOID   PmlEntry    = NULL;
    BOOLEAN IsLargePage = FALSE;

    PmlEntry = EptGetPml1OrPml2Entry(g_EptState->EptPageTable, (SIZE_T)PhysicalAddress, &IsLargePage);

    if (PmlEntry)
    {
        if (IsLargePage)
        {
            if (IsUnset)
            {
                ((PEPT_PML2_ENTRY)PmlEntry)->ExecuteAccess = FALSE;
            }
            else
            {
                ((PEPT_PML2_ENTRY)PmlEntry)->ExecuteAccess = TRUE;
            }
        }
        else
        {
            if (IsUnset)
            {
                ((PEPT_PML1_ENTRY)PmlEntry)->ExecuteAccess = FALSE;
            }
            else
            {
                ((PEPT_PML1_ENTRY)PmlEntry)->ExecuteAccess = TRUE;
            }
        }
    }
    else
    {
        return FALSE;
    }

    //
    // Invalidate the EPTP (single-context)
    //
    EptInveptSingleContext(VCpu->EptPointer.AsUInt);

    return TRUE;
}

/**
 * @brief Change PML EPT state for read
 * @detail should be called from VMX-root
 *
 * @param VCpu The virtual processor's state
 * @param PhysicalAddress Target physical address
 * @param IsUnset Is unsetting bit or setting bit
 *
 * @return BOOLEAN
 */
BOOLEAN
EptHookModifyPageReadState(VIRTUAL_MACHINE_STATE * VCpu,
                           PVOID                   PhysicalAddress,
                           BOOLEAN                 IsUnset)
{
    PVOID   PmlEntry    = NULL;
    BOOLEAN IsLargePage = FALSE;

    PmlEntry = EptGetPml1OrPml2Entry(g_EptState->EptPageTable, (SIZE_T)PhysicalAddress, &IsLargePage);

    if (PmlEntry)
    {
        if (IsLargePage)
        {
            if (IsUnset)
            {
                ((PEPT_PML2_ENTRY)PmlEntry)->ReadAccess = FALSE;
            }
            else
            {
                ((PEPT_PML2_ENTRY)PmlEntry)->ReadAccess = TRUE;
            }
        }
        else
        {
            if (IsUnset)
            {
                ((PEPT_PML1_ENTRY)PmlEntry)->ReadAccess = FALSE;
            }
            else
            {
                ((PEPT_PML1_ENTRY)PmlEntry)->ReadAccess = TRUE;
            }
        }
    }
    else
    {
        return FALSE;
    }

    //
    // Invalidate the EPTP (single-context)
    //
    EptInveptSingleContext(VCpu->EptPointer.AsUInt);

    return TRUE;
}

/**
 * @brief Change PML EPT state for write
 * @detail should be called from VMX-root
 *
 * @param VCpu The virtual processor's state
 * @param PhysicalAddress Target physical address
 * @param IsUnset Is unsetting bit or setting bit
 *
 * @return BOOLEAN
 */
BOOLEAN
EptHookModifyPageWriteState(VIRTUAL_MACHINE_STATE * VCpu,
                            PVOID                   PhysicalAddress,
                            BOOLEAN                 IsUnset)
{
    PVOID   PmlEntry    = NULL;
    BOOLEAN IsLargePage = FALSE;

    PmlEntry = EptGetPml1OrPml2Entry(g_EptState->EptPageTable, (SIZE_T)PhysicalAddress, &IsLargePage);

    if (PmlEntry)
    {
        if (IsLargePage)
        {
            if (IsUnset)
            {
                ((PEPT_PML2_ENTRY)PmlEntry)->WriteAccess = FALSE;
            }
            else
            {
                ((PEPT_PML2_ENTRY)PmlEntry)->WriteAccess = TRUE;
            }
        }
        else
        {
            if (IsUnset)
            {
                ((PEPT_PML1_ENTRY)PmlEntry)->WriteAccess = FALSE;
            }
            else
            {
                ((PEPT_PML1_ENTRY)PmlEntry)->WriteAccess = TRUE;
            }
        }
    }
    else
    {
        return FALSE;
    }

    //
    // Invalidate the EPTP (single-context)
    //
    EptInveptSingleContext(VCpu->EptPointer.AsUInt);

    return TRUE;
}
