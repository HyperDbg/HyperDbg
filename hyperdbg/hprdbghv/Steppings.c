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

VOID
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

    UINT32      ProcessId       = 0x1c60;
    UINT32      ThreadId        = 3852;
    CR3_TYPE    TargetKernelCr3 = GetCr3FromProcessId(ProcessId);
    PPAGE_ENTRY TargetPte;

    //UINT64 ReservedAddress = MemoryMapperReserveUsermodeAddressInTargetProcess(ProcessId, TRUE);

    //
    // Find the PTE of target address
    //

    //
    // Test on grabbing a thread-state
    //
    // SteppingsStartDebuggingThread(ProcessId, ThreadId);
}

VOID
SteppingsUninitialize()
{
    //
    // Disable debugger stepping mechanism
    //
    g_EnableDebuggerSteppings = FALSE;
}

// should be called from vmx non-root
VOID
SteppingsStartDebuggingThread(UINT32 ProcessId, UINT32 ThreadId)
{
    UINT32 ProcessorCount;

    //
    // Find count of all the processors
    //
    ProcessorCount = KeQueryActiveProcessorCount(0);

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
    BOOLEAN IsOnTheTargetProcess = FALSE;
    UINT32  CurrentProcessId;
    UINT32  CurrentThreadId;
    UINT32  ProcessorCount;

    //
    //  Lock the spinlock
    //
    SpinlockLock(&ExternalInterruptFindProcessAndThreadId);

    //
    // Check whether we should handle the interrupt differently
    // because of debugger steppings mechanism or not and also check
    // whether the target process already found or not
    //
    if (g_GuestState[ProcessorIndex].DebuggerSteppingDetails.DisableExternalInterrupts)
    {
        //
        // The target process and thread already found, not need to
        // further external-interrupt exiting
        //
        HvSetExternalInterruptExiting(FALSE);
    }
    else if (g_GuestState[ProcessorIndex].DebuggerSteppingDetails.IsWaitingForClockInterrupt &&
             ((InterruptExit->Vector == CLOCK_INTERRUPT && ProcessorIndex == 0) ||
              (InterruptExit->Vector == IPI_INTERRUPT && ProcessorIndex != 0)))
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
            // Disable external-interrupt exitings as we are not intersted on
            // receiving external-interrupts (clock and ipi) anymore
            //
            HvSetExternalInterruptExiting(FALSE);

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
        SteppingsHandleTargetThreadForTheFirstTime(GuestRegs, ProcessorIndex);
    }
}

// should be called in vmx root
VOID
SteppingsHandleTargetThreadForTheFirstTime(PGUEST_REGS GuestRegs, UINT32 ProcessorIndex)
{
    UINT64           GuestRip = 0;
    SEGMENT_SELECTOR Cs, Ss;
    UINT64           MsrValue;
    ULONG64          GuestRflags;
    CR3_TYPE         GuestCr3;

    DbgBreakPoint();

    //
    // read the guest's rip
    //
    __vmx_vmread(GUEST_RIP, &GuestRip);

    //
    // read the guest's cr3
    //
    __vmx_vmread(GUEST_CR3, &GuestCr3);

    //
    // Swap the page with a nop-sled
    //
    SteppingsSwapPageWithInfiniteLoop(GuestRip, GuestCr3, ProcessorIndex);
}

/**
 * @brief Swap the target page with an infinite loop
 * @details This function returns false in VMX Non-Root Mode if the VM is already initialized
 * 
 * @param TargetAddress The address of function or memory address to be swapped
 * @param ProcessCr3 The process cr3 to translate based on that process's cr3
 * @param LogicalCoreIndex The logical core index 
 * @return BOOLEAN Returns true if the swap was successfull or false if there was an error
 */
BOOLEAN
SteppingsSwapPageWithInfiniteLoop(PVOID TargetAddress, CR3_TYPE ProcessCr3, UINT32 LogicalCoreIndex)
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

    if (!EptSplitLargePage(g_EptState->EptPageTable, TargetBuffer, PhysicalBaseAddress, LogicalCoreIndex))
    {
        LogError("Could not split page for the address : 0x%llx", PhysicalBaseAddress);
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
        LogError("Failed to get PML1 entry of the target address");
        return FALSE;
    }

    //
    // Save the original permissions of the page
    //
    ChangedEntry = *TargetPage;

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
