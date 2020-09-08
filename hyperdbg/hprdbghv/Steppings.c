/**
 * @file Steppings.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Stepping (step in & step out) mechanisms
 * @details
 * @version 0.1
 * @date 2020-08-30
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

UINT32 ProcessId = 4488;
UINT32 ThreadId  = 7632;

BOOLEAN
SteppingsInitialize()
{
    //
    // Initialize the list that holds the debugging
    // state for each thread
    //
    InitializeListHead(&g_ThreadDebuggingStates);

    //
    // Enable debugger stepping mechanism
    //
    g_EnableDebuggerSteppings = TRUE;

    //
    // Check if we have a secondary EPT Identity Table or not
    //
    if (!g_EptState->SecondaryInitialized)
    {
        if (!EptInitializeSeconadaryEpt())
        {
            //
            // There was an error in making the secondary EPT identity map
            //
            return FALSE;
        }
    }

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
        // 9272894.o:     file format elf64-x86-64
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
    // Test on grabbing a thread-state
    //
    // SteppingsStartDebuggingThread(ProcessId, ThreadId);

    return TRUE;
}

VOID
SteppingsUninitialize()
{
    PLIST_ENTRY TempList;

    //
    // Disable debugger stepping mechanism
    //
    g_EnableDebuggerSteppings = FALSE;

    //
    // If the debugger used a secondary EPT Table then we
    // have to free it too
    //
    if (g_EptState->SecondaryInitialized)
    {
        //
        // Unset the initialized flag of secondary ept page table
        //
        g_EptState->SecondaryInitialized = FALSE;

        //
        // Null the EPTP of secondary page table
        //
        g_EptState->SecondaryEptPointer.Flags = NULL;

        //
        // Free the buffer
        //
        MmFreeContiguousMemory(g_EptState->SecondaryEptPageTable);
    }

    //
    // If the nop-sled is initialized then we have to de-allocate it
    //
    if (g_SteppingsNopSledState.IsNopSledInitialized)
    {
        //
        // Set it to a uninitialized state
        //
        g_SteppingsNopSledState.IsNopSledInitialized = FALSE;

        //
        // De-allocate the buffer
        //
        ExFreePoolWithTag(g_SteppingsNopSledState.NopSledVirtualAddress, POOLTAG);

        //
        // Zero out the structure
        //
        RtlZeroMemory(&g_SteppingsNopSledState, sizeof(DEBUGGER_STEPPINGS_NOP_SLED));
    }

    //
    // De-allocate the buffers relating to stepping mechanisms
    //
    TempList = &g_ThreadDebuggingStates;
    while (&g_ThreadDebuggingStates != TempList->Flink)
    {
        TempList                                                       = TempList->Flink;
        PDEBUGGER_STEPPING_THREAD_DETAILS CurrentThreadDebuggingDetail = CONTAINING_RECORD(TempList,
                                                                                           DEBUGGER_STEPPING_THREAD_DETAILS,
                                                                                           DebuggingThreadsList);

        //
        // Disable before de-allocation
        //
        CurrentThreadDebuggingDetail->Enabled = FALSE;

        //
        // De-allocate it
        //
        ExFreePoolWithTag(CurrentThreadDebuggingDetail->BufferAddressToFree, POOLTAG);
    }
}

// should be called from vmx non-root
VOID
SteppingsStartDebuggingThread(UINT32 ProcessId, UINT32 ThreadId)
{
    UINT32                            ProcessorCount;
    PDEBUGGER_STEPPING_THREAD_DETAILS ThreadDetailsBuffer;

    //
    // Find count of all the processors
    //
    ProcessorCount = KeQueryActiveProcessorCount(0);

    //
    // Allocate a buffer to hold the data relating to debugging thread
    //
    ThreadDetailsBuffer = (PDEBUGGER_STEPPING_THREAD_DETAILS)ExAllocatePoolWithTag(NonPagedPool,
                                                                                   sizeof(DEBUGGER_STEPPING_THREAD_DETAILS),
                                                                                   POOLTAG);

    if (ThreadDetailsBuffer == NULL)
    {
        return;
    }

    RtlZeroMemory(ThreadDetailsBuffer, sizeof(DEBUGGER_STEPPING_THREAD_DETAILS));

    //
    // Set to all cores that we are trying to find a special thread
    //
    for (size_t i = 0; i < ProcessorCount; i++)
    {
        //
        // Set that we are waiting for a special thread id
        //
        g_GuestState[i].DebuggerSteppingDetails.TargetProcessId = ProcessId;

        //
        // Set that we are waiting for a special process id
        //
        g_GuestState[i].DebuggerSteppingDetails.TargetThreadId = ThreadId;

        //
        // Set the kernel cr3 (save cr3 of kernel for future use)
        //
        g_GuestState[i].DebuggerSteppingDetails.TargetThreadKernelCr3 = GetCr3FromProcessId(ProcessId);

        //
        // We should not disable external-interrupts until we find the process
        //
        g_GuestState[i].DebuggerSteppingDetails.DisableExternalInterrupts = FALSE;

        //
        // Set that we are waiting for clock-interrupt on all cores
        //
        g_GuestState[i].DebuggerSteppingDetails.IsWaitingForClockInterrupt = TRUE;

        //
        // Set the buffer for saving the details about a debugging thread
        //
        g_GuestState[i].DebuggerSteppingDetails.BufferToSaveThreadDetails = ThreadDetailsBuffer;

        //
        // Enable external-interrupt exiting, because we are waiting for
        // either Clock-Interrupt (if we are in core 0) or IPI Interrupt if
        // we are not in core 0, it is because Windows only applies the clock
        // interrupt to core 0 (not other cores) and after that, it syncs other
        // cores with an IPI
        // Because of the above-mentioned reasons, we should enable external-interrupt
        // exiting on all cores
        //
        ExtensionCommandSetExternalInterruptExitingAllCores();
    }
}

// should be called in vmx root
VOID
SteppingsHandleClockInterruptOnTargetProcess(PGUEST_REGS GuestRegs, UINT32 ProcessorIndex, PVMEXIT_INTERRUPT_INFO InterruptExit)
{
    BOOLEAN                           IsOnTheTargetProcess = FALSE;
    CR3_TYPE                          ProcessKernelCr3     = {0};
    PDEBUGGER_STEPPING_THREAD_DETAILS ThreadDetailsBuffer  = 0;
    UINT32                            CurrentProcessId     = 0;
    UINT32                            CurrentThreadId      = 0;
    UINT32                            ProcessorCount       = 0;

    //
    // We only handle interrupts that are related to the clock-timer interrupt
    //
    if ((InterruptExit->Vector == CLOCK_INTERRUPT && ProcessorIndex == 0) ||
        (InterruptExit->Vector == IPI_INTERRUPT && ProcessorIndex != 0))
    {
        //
        //  Lock the spinlock
        //
        SpinlockLock(&ExternalInterruptFindProcessAndThreadId);

        //
        // Check whether we should handle the interrupt differently
        // because of debugger steppings mechanism or not and also check
        // whether the target process already found or not
        //
        if (g_GuestState[ProcessorIndex].DebuggerSteppingDetails.ChangeToPrimaryEptp)
        {
            //
            // Disable the flag of change eptp
            //
            g_GuestState[ProcessorIndex].DebuggerSteppingDetails.ChangeToPrimaryEptp = FALSE;

            //
            // Change EPTP to primary entry
            //
            __vmx_vmwrite(EPT_POINTER, g_EptState->EptPointer.Flags);

            //
            // The target process and thread is now changing
            // to a new process, no longer need to cause vm-exit
            // on external interrupts
            //
            HvSetExternalInterruptExiting(FALSE);
        }
        else if (g_GuestState[ProcessorIndex].DebuggerSteppingDetails.DisableExternalInterrupts)
        {
            //
            // The target process and thread already found, not need to
            // further external-interrupt exiting
            //
            HvSetExternalInterruptExiting(FALSE);

            //
            // When we reached here, other logical processor swapped
            // the thread page and we saved all the page entry details
            // (original & noped) into the thread buffer detail, as the
            // causing vm-exit for each external-interrupt is slow, so
            // we set vm-exit on cr3 changes to keep noping the target
            // page only if we are in the target process and thread and
            // after that we will configure to cause vm-exits again to
            // revert ept to its initial state when we finished with proocess
            //
            HvSetMovToCr3Vmexit(TRUE);
        }
        else if (g_GuestState[ProcessorIndex].DebuggerSteppingDetails.IsWaitingForClockInterrupt)
        {
            //
            // Read other details of process and thread
            //
            CurrentProcessId = PsGetCurrentProcessId();
            CurrentThreadId  = PsGetCurrentThreadId();

            if (g_GuestState[ProcessorIndex].DebuggerSteppingDetails.TargetProcessId == CurrentProcessId &&
                g_GuestState[ProcessorIndex].DebuggerSteppingDetails.TargetThreadId == CurrentThreadId)
            {
                //
                // *** The target process is found and we are in middle of the process ***
                //

                //
                // Save the kernel cr3 of target process
                //
                ProcessKernelCr3 = g_GuestState[ProcessorIndex].DebuggerSteppingDetails.TargetThreadKernelCr3;

                //
                // Save the buffer for saving thread details
                //
                ThreadDetailsBuffer = g_GuestState[ProcessorIndex].DebuggerSteppingDetails.BufferToSaveThreadDetails;

                //
                // Find count of all the processors
                //
                ProcessorCount = KeQueryActiveProcessorCount(0);

                //
                // Indicate that we are not waiting for anything on all cores
                //
                for (size_t i = 0; i < ProcessorCount; i++)
                {
                    g_GuestState[i].DebuggerSteppingDetails.IsWaitingForClockInterrupt  = FALSE;
                    g_GuestState[i].DebuggerSteppingDetails.TargetThreadId              = NULL;
                    g_GuestState[i].DebuggerSteppingDetails.TargetThreadId              = NULL;
                    g_GuestState[i].DebuggerSteppingDetails.TargetThreadKernelCr3.Flags = NULL;
                    g_GuestState[i].DebuggerSteppingDetails.BufferToSaveThreadDetails   = NULL;

                    //
                    // Because we disabled external-interrupts here in this function, no need
                    // to set this bit for current logical processor
                    //
                    if (i != ProcessorIndex)
                    {
                        g_GuestState[i].DebuggerSteppingDetails.DisableExternalInterrupts = TRUE;
                    }
                }

                //
                // We set the handler here because we want to handle the process
                // later after releasing the lock, it is because we don't need
                // to make all the processor to wait for us, however this idea
                // won't work on first core as if we wait on first core then
                // all the other cores are waiting for an IPI but if we get the
                // thread on any other processor, other than 0th, then it's better
                // to set the following value to TRUE and handle it later as
                // we released the lock and no other core is waiting for us
                //
                IsOnTheTargetProcess = TRUE;
            }
        }

        //
        //  Unlock the spinlock
        //
        SpinlockUnlock(&ExternalInterruptFindProcessAndThreadId);

        //
        // Check if we find the thread or not
        //
        if (IsOnTheTargetProcess)
        {
            //
            // Handle the thread
            //
            SteppingsHandleTargetThreadForTheFirstTime(GuestRegs, ThreadDetailsBuffer, ProcessorIndex, ProcessKernelCr3, CurrentProcessId, CurrentThreadId);
        }
    }
}

// should be called in vmx root
VOID
SteppingsHandleTargetThreadForTheFirstTime(PGUEST_REGS GuestRegs, PDEBUGGER_STEPPING_THREAD_DETAILS ThreadDetailsBuffer, UINT32 ProcessorIndex, CR3_TYPE KernelCr3, UINT32 ProcessId, UINT32 ThreadId)
{
    UINT64           GuestRip = 0;
    SEGMENT_SELECTOR Cs, Ss;
    UINT64           MsrValue;
    CR3_TYPE         GuestCr3;
    ULONG64          GuestRflags;

    //
    // read the guest's rip
    //
    __vmx_vmread(GUEST_RIP, &GuestRip);

    //
    // read the guest's cr3
    //
    __vmx_vmread(GUEST_CR3, &GuestCr3);

    //
    // Chnage the core's eptp to the new eptp
    //
    if (g_EptState->SecondaryInitialized)
    {
        __vmx_vmwrite(EPT_POINTER, g_EptState->SecondaryEptPointer.Flags);
    }
    else
    {
        //
        // Secondary page table is not initialized, what else to do !!!
        //
        return;
    }

    //
    // We should make the vmm to restore to the primary eptp whenever a
    // timing clock interrupt is received (external-interrupt exiting is
    // enabled at this moment)
    //
    g_GuestState[ProcessorIndex].DebuggerSteppingDetails.ChangeToPrimaryEptp = TRUE;

    //
    // Fill the details for target buffer, as long as available
    // not all details are filled here
    //
    ThreadDetailsBuffer->BufferAddressToFree = ThreadDetailsBuffer;
    ThreadDetailsBuffer->ProcessId           = ProcessId;
    ThreadDetailsBuffer->ThreadId            = ThreadId;
    ThreadDetailsBuffer->ThreadKernelCr3     = KernelCr3;
    ThreadDetailsBuffer->ThreadUserCr3       = GuestCr3;
    ThreadDetailsBuffer->ThreadRip           = GuestRip;

    //
    // Save the general purpose registers
    //
    RtlCopyMemory(&ThreadDetailsBuffer->ThreadRegisters, GuestRegs, sizeof(GUEST_REGS));

    //
    // Set it as active
    //
    ThreadDetailsBuffer->Enabled = TRUE;

    //
    // Swap the page with a nop-sled
    // this function will fill some of the fields relating
    // to ept entry and pfn on the ThreadDetailsBuffer
    //
    SteppingsSwapPageWithInfiniteLoop(GuestRip, KernelCr3, ProcessorIndex, ThreadDetailsBuffer);

    //
    // We swapped the page but the GuestRip might be at the two
    // ending bits of a page (4096 Byte ==> 4095th and 4096the byte)
    // and if this happens the HyperDbg ends on a two ending byte (jmp)
    // instruction bytes of the page so we should change the guest rip
    // to another but we saved the correct value before we reached here
    //
    if (GuestRip & 0xfff || GuestRip & 0xffe)
    {
        //
        // The guest rip is at the end of the page in jmp instruction
        // byte, we can decrease it's rip by 3, 4, 5, ..., whatever, to
        // point to the nop loop
        //
        GuestRip = GuestRip - 4;

        //
        // Apply it to the guest
        //
        __vmx_vmwrite(GUEST_RIP, GuestRip);
    }

    //
    // When we reached here, we're done swapping the target
    // GUEST_RIP to a swapped nop page and we saved all the
    // page entry details (original & noped) into the thread
    // buffer detail, as the causing vm-exit for each external-
    // interrupt is slow, so we set vm-exit on cr3 changes to
    // keep noping the target page only if we are in the target
    // process and thread and after that we will configure to
    // cause vm-exits again to revert ept to its initial state
    // when we finished with proocess
    //
    HvSetMovToCr3Vmexit(TRUE);

    //
    // Add the thread debugging details into the list
    //
    InsertHeadList(&g_ThreadDebuggingStates, &(ThreadDetailsBuffer->DebuggingThreadsList));
}

/**
 * @brief Swap the target page with an infinite loop
 * @details This function returns false in VMX Non-Root Mode if the VM is already initialized
 * 
 * @param TargetAddress The address of function or memory address to be swapped
 * @param ProcessCr3 The process cr3 to translate based on that process's kernel cr3
 * @param LogicalCoreIndex The logical core index 
 * @param ThreadDetailsBuffer The structure to fill details about ept entry and pfn 
 * @return BOOLEAN Returns true if the swap was successfull or false if there was an error
 */
BOOLEAN
SteppingsSwapPageWithInfiniteLoop(PVOID TargetAddress, CR3_TYPE ProcessCr3, UINT32 LogicalCoreIndex, PDEBUGGER_STEPPING_THREAD_DETAILS ThreadDetailsBuffer)
{
    EPT_PML1_ENTRY    ChangedEntry;
    INVEPT_DESCRIPTOR Descriptor;
    SIZE_T            PhysicalBaseAddress;
    PVOID             VirtualTarget;
    PVOID             TargetBuffer;
    PEPT_PML1_ENTRY   TargetPage;

    //
    // Check whether we are in VMX Root Mode or Not
    //
    if (g_GuestState[LogicalCoreIndex].IsOnVmxRootMode && !g_GuestState[LogicalCoreIndex].HasLaunched)
    {
        return FALSE;
    }

    //
    // Change the pfn to a nop-sled page if the nop-sled
    // is already initialized
    //
    if (!g_SteppingsNopSledState.IsNopSledInitialized)
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
    PhysicalBaseAddress = (SIZE_T)VirtualAddressToPhysicalAddressByProcessCr3(VirtualTarget, ProcessCr3);

    if (!PhysicalBaseAddress)
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

    //
    // Split 2 MB granularity to 4 KB granularity
    //
    if (!EptSplitLargePage(g_EptState->SecondaryEptPageTable, TargetBuffer, PhysicalBaseAddress, LogicalCoreIndex))
    {
        LogError("Could not split page for the address : 0x%llx", PhysicalBaseAddress);
        return FALSE;
    }

    //
    // Pointer to the page entry in the page table
    //
    TargetPage = EptGetPml1Entry(g_EptState->SecondaryEptPageTable, PhysicalBaseAddress);

    //
    // Ensure the target is valid
    //
    if (!TargetPage)
    {
        LogError("Failed to get PML1 entry of the target address");
        return FALSE;
    }

    //
    // Fill the thread details about page entry
    //
    ThreadDetailsBuffer->TargetEntryOnSecondaryPageTable = TargetPage;
    ThreadDetailsBuffer->OriginalEntryContent            = *TargetPage;

    //
    // Save the original permissions of the page
    //
    ChangedEntry = *TargetPage;

    //
    // Change the pfn to a nop-sled page
    //
    ChangedEntry.PageFrameNumber = g_SteppingsNopSledState.NopSledPhysicalAddress.QuadPart >> 12;

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
        EptSetPML1AndInvalidateTLB(TargetPage, ChangedEntry, INVEPT_ALL_CONTEXTS);
    }

    //
    // Fill the noped entry to the details relating to save thread state
    //
    ThreadDetailsBuffer->NopedEntryContent = ChangedEntry;

    //
    // Invalidate the page
    //
    __invlpg(TargetAddress);

    return TRUE;
}

VOID
SteppingsHandleCr3Vmexits(CR3_TYPE NewCr3, UINT32 ProcessorIndex)
{
    PLIST_ENTRY TempList  = 0;
    UINT32      ProcessId = 0;
    UINT32      ThreadId  = 0;

    ProcessId = PsGetCurrentProcessId();
    ThreadId  = PsGetCurrentThreadId();

    //
    // We have to iterate through the list of active debugging thread details
    //
    TempList = &g_ThreadDebuggingStates;
    while (&g_ThreadDebuggingStates != TempList->Flink)
    {
        TempList                                                       = TempList->Flink;
        PDEBUGGER_STEPPING_THREAD_DETAILS CurrentThreadDebuggingDetail = CONTAINING_RECORD(TempList,
                                                                                           DEBUGGER_STEPPING_THREAD_DETAILS,
                                                                                           DebuggingThreadsList);
        //
        // If its not enabled, we don't have to do anything
        //
        if (!CurrentThreadDebuggingDetail->Enabled)
        {
            continue;
        }

        //
        // Now, we should check whether the following thread is in the list of
        // threads that currently debugging or not (by it's GUEST_CR3 and process
        // id and thread id)
        //
        if (CurrentThreadDebuggingDetail->ThreadUserCr3.Flags == NewCr3.Flags &&
            CurrentThreadDebuggingDetail->ProcessId == ProcessId &&
            CurrentThreadDebuggingDetail->ThreadId == ThreadId)
        {
            //
            // This thread is on the debugging list
            //
            SteppingsHandlesDebuggedThread(CurrentThreadDebuggingDetail, ProcessorIndex);
        }
    }
}

/**
 * @brief Perform tasks relating to stepping (step-in & step-out) requests
 * 
 * @param DebuggerSteppingRequest Request to steppings
 * @return NTSTATUS 
 */
NTSTATUS
SteppingsPerformAction(PDEBUGGER_STEPPINGS DebuggerSteppingRequest)
{
    PLIST_ENTRY TempList            = 0;
    BOOLEAN     FoundAWorkingThread = FALSE;
    BOOLEAN     InvalidParameter    = FALSE;

    //
    // We have to iterate through the list of active debugging thread details
    // to find the thread's structures about steppings
    //
    TempList = &g_ThreadDebuggingStates;
    while (&g_ThreadDebuggingStates != TempList->Flink)
    {
        TempList                                                       = TempList->Flink;
        PDEBUGGER_STEPPING_THREAD_DETAILS CurrentThreadDebuggingDetail = CONTAINING_RECORD(TempList,
                                                                                           DEBUGGER_STEPPING_THREAD_DETAILS,
                                                                                           DebuggingThreadsList);
        //
        // If its not enabled, we don't have to do anything
        //
        if (!CurrentThreadDebuggingDetail->Enabled)
        {
            continue;
        }

        //
        // Check if the current thread matches the details or not
        //
        if (DebuggerSteppingRequest->ProcessId == CurrentThreadDebuggingDetail->ProcessId &&
            DebuggerSteppingRequest->ThreadId == CurrentThreadDebuggingDetail->ThreadId)
        {
            //
            // Add (and validate) the request to be handled by the debugger
            //
            if (DebuggerSteppingRequest->SteppingAction == STEPPINGS_ACTION_STEP_INTO)
            {
                CurrentThreadDebuggingDetail->SteppingAction.ACTION = STEPPINGS_ACTION_STEP_INTO;
            }
            else if (DebuggerSteppingRequest->SteppingAction == STEPPINGS_ACTION_STEP_OUT)
            {
                CurrentThreadDebuggingDetail->SteppingAction.ACTION = STEPPINGS_ACTION_STEP_OUT;
            }
            else
            {
                InvalidParameter = TRUE;
            }

            //
            // We found the thread a applied the steppings request
            //
            FoundAWorkingThread = TRUE;

            //
            // We just apply it to one thread
            //
            break;
        }
    }

    //
    // Set status
    //
    if (InvalidParameter)
    {
        DebuggerSteppingRequest->KernelStatus = DEBUGGER_ERROR_STEPPING_INVALID_PARAMETER;
    }
    else if (FoundAWorkingThread)
    {
        DebuggerSteppingRequest->KernelStatus = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
    }
    else
    {
        DebuggerSteppingRequest->KernelStatus = DEBUGGER_ERROR_STEPPINGS_EITHER_THREAD_NOT_FOUND_OR_DISABLED;
    }

    return STATUS_SUCCESS;
}

VOID
SteppingsHandlesDebuggedThread(PDEBUGGER_STEPPING_THREAD_DETAILS ThreadSteppingDetail, UINT32 ProcessorIndex)
{
    //
    // We should swap the page table into secondary ept page table again
    //
    ThreadSteppingDetail->TargetEntryOnSecondaryPageTable->Flags = ThreadSteppingDetail->NopedEntryContent.Flags;

    //
    // Change to the new eptp
    //
    __vmx_vmwrite(EPT_POINTER, g_EptState->SecondaryEptPointer.Flags);

    //
    // We should make the vmm to restore to the primary eptp whenever a
    // timing clock interrupt is received
    //
    g_GuestState[ProcessorIndex].DebuggerSteppingDetails.ChangeToPrimaryEptp = TRUE;

    //
    // At this moment, external interrupt exiting in the current core must be
    // disabled, we have to enable it
    //
    HvSetExternalInterruptExiting(TRUE);
}
