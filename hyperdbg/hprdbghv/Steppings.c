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

    //
    // Test on grabbing a thread-state
    //
    //SteppingsStartDebuggingThread(568, 3216);
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
SteppingsHandleMovToCr3Exiting(PGUEST_REGS GuestRegs, UINT32 ProcessorIndex, UINT64 NewCr3)
{
    UINT64 GuestRip = 0;

    //
    // read the guest's rip
    //
    __vmx_vmread(GUEST_RIP, &GuestRip);

    NT_KPROCESS * CurrentProcess = (NT_KPROCESS *)(PsGetCurrentProcess());

    // LogInfo("cr3 change to %llx = %llx and process id is %llx", NewCr3, CurrentProcess->DirectoryTableBase, PsGetCurrentProcessId());
    if (NewCr3 == CurrentProcess->DirectoryTableBase && PsGetCurrentProcessId() == 4)
    {
    }
}

// should be called in vmx root
VOID
SteppingsHandleClockInterruptOnTargetProcess(PGUEST_REGS GuestRegs, UINT32 ProcessorIndex, PVMEXIT_INTERRUPT_INFO InterruptExit)
{
    UINT64 GuestRip = 0;
    UINT32 CurrentProcessId;
    UINT32 CurrentThreadId;
    UINT32 ProcessorCount;

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
        // read the guest's rip
        //
        __vmx_vmread(GUEST_RIP, &GuestRip);

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
            DbgBreakPoint();

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
                g_GuestState[i].DebuggerSteppingDetails.IsWaitingForClockInterrupt = FALSE;
                g_GuestState[i].DebuggerSteppingDetails.TargetThreadId             = NULL;
                g_GuestState[i].DebuggerSteppingDetails.TargetThreadId             = NULL;

                //
                // Because we disabled external-interrupts here in this function, no need
                // to set this bit for current logical processor
                //
                if (i != ProcessorIndex)
                {
                    g_GuestState[i].DebuggerSteppingDetails.DisableExternalInterrupts = TRUE;
                }
            }
        }
    }

    //
    //  Unlock the spinlock
    //
    SpinlockUnlock(&ExternalInterruptFindProcessAndThreadId);
}
