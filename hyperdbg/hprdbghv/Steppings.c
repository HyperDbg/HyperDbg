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
    // Enable the cr3-exiting (mov to cr3)
    //
    // DebuggerEventEnableMovToCr3ExitingOnAllProcessors();
}

VOID
SteppingsUninitialize()
{
    //
    // Disable debugger stepping mechanism
    //
    g_EnableDebuggerSteppings = FALSE;

    //
    // Disable the cr3-exiting (mov to cr3)
    //
    DebuggerEventDisableMovToCr3ExitingOnAllProcessors();
}

VOID
SteppingsStartDebuggingThread(UINT32 ProcessId, UINT32 ThreadId)
{
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
        //
        // Set that we are waiting for clock-interrupt
        //
        g_GuestState[ProcessorIndex].DebuggerSteppingDetails.IsWaitingForClockInterrupt = TRUE;

        //
        // Enable external-interrupt exiting, because we are waiting for
        // either Clock-Interrupt (if we are in core 0) or IPI Interrupt if
        // we are not in core 0, it is because Windows only applies the clock
        // interrupt to core 0 (not other cores) and after that, it syncs other
        // cores with an IPI
        //
        HvSetExternalInterruptExiting(TRUE);

        //
        // Save the details of the current process and thread
        //
        g_GuestState[ProcessorIndex].DebuggerSteppingDetails.CurrentProcessId = PsGetCurrentProcessId();
        g_GuestState[ProcessorIndex].DebuggerSteppingDetails.CurrentCr3       = NewCr3;
        g_GuestState[ProcessorIndex].DebuggerSteppingDetails.CurrentThreadId  = PsGetCurrentThreadId();
    }
}

// should be called in vmx root
VOID
SteppingsHandleClockInterruptOnTargetProcess(PGUEST_REGS GuestRegs, UINT32 ProcessorIndex)
{
    UINT64 GuestRip = 0;

    //
    // read the guest's rip
    //
    __vmx_vmread(GUEST_RIP, &GuestRip);

    //
    // Disable external-interrupt exitings as we are not intersted on
    // receiving external-interrupts (clock and ipi) anymore
    //
    HvSetExternalInterruptExiting(FALSE);

    //
    // Indicate that we are not waiting for anything
    //
    g_GuestState[ProcessorIndex].DebuggerSteppingDetails.IsWaitingForClockInterrupt = FALSE;

    UINT32 CurrentProcessId = PsGetCurrentProcessId();
    UINT32 CurrentThreadId  = PsGetCurrentThreadId();
    DbgBreakPoint();
}
