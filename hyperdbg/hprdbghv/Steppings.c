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

UINT64 TempStack;
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

    TempStack = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOLTAG);
    //
    // Test on grabbing a thread-state
    //
    // SteppingsStartDebuggingThread(1908, 7120);
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
    BOOLEAN  IsOnTheTargetProcess = FALSE;
    UINT32   CurrentProcessId;
    UINT32   CurrentThreadId;
    UINT32   ProcessorCount;
    CR3_TYPE ProcessKernelCr3;

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
            // Set the kernel cr3 for future use
            //
            ProcessKernelCr3 = g_GuestState[ProcessorIndex].DebuggerSteppingDetails.TargetThreadKernelCr3;

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
        SteppingsHandleTargetThreadForTheFirstTime(GuestRegs, ProcessorIndex, &ProcessKernelCr3);
    }
}

// should be called in vmx root
VOID
SteppingsHandleTargetThreadForTheFirstTime(PGUEST_REGS GuestRegs, UINT32 ProcessorIndex, PCR3_TYPE KernelCr3)
{
    UINT64           GuestRip = 0;
    SEGMENT_SELECTOR Cs, Ss;
    UINT64           MsrValue;
    ULONG64          GuestRflags;

    DbgBreakPoint();

    //
    // read the guest's rip
    //
    __vmx_vmread(GUEST_RIP, &GuestRip);

    //
    // Set the target thread to spinner
    //
    __vmx_vmwrite(GUEST_RIP, AsmDebuggerSpinOnThread);

    //
    // Change the RSP
    //
    __vmx_vmwrite(GUEST_RSP, TempStack);

    //
    // Change the cr3 to the kernel cr3
    //
    __vmx_vmwrite(GUEST_CR3, KernelCr3->Flags);

    //
    // Reading guest's Rflags
    //
    __vmx_vmread(GUEST_RFLAGS, &GuestRflags);

    //
    // Save RFLAGS into R11 and then mask RFLAGS using MSR_FMASK
    //
    MsrValue = __readmsr(MSR_FMASK);
    GuestRflags &= ~(MsrValue | X86_FLAGS_RF);

    __vmx_vmwrite(GUEST_RFLAGS, GuestRflags);

    //
    // Load the CS and SS selectors with values derived from bits 47:32 of MSR_STAR
    //
    MsrValue             = __readmsr(MSR_STAR);
    Cs.SEL               = (UINT16)((MsrValue >> 32) & ~3); // STAR[47:32] & ~RPL3
    Cs.BASE              = 0;                               // flat segment
    Cs.LIMIT             = (UINT32)~0;                      // 4GB limit
    Cs.ATTRIBUTES.UCHARs = 0xA09B;                          // L+DB+P+S+DPL0+Code
    SetGuestCs(&Cs);

    Ss.SEL               = (UINT16)(((MsrValue >> 32) & ~3) + 8); // STAR[47:32] + 8
    Ss.BASE              = 0;                                     // flat segment
    Ss.LIMIT             = (UINT32)~0;                            // 4GB limit
    Ss.ATTRIBUTES.UCHARs = 0xC093;                                // G+DB+P+S+DPL0+Data
    SetGuestSs(&Ss);
}
