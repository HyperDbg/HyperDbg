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
    // Enable the cr3-exiting (mov to cr3)
    //
    // DebuggerEventEnableMovToCr3ExitingOnAllProcessors();
}

VOID
SteppingsUninitialize()
{
    //
    // Disable the cr3-exiting (mov to cr3)
    //
    DebuggerEventDisableMovToCr3ExitingOnAllProcessors();
}

VOID
SteppingsStartDebuggingThread(UINT32 ProcessId, UINT32 ThreadId)
{
}

VOID
SteppingsHandleMovToCr3Exiting(PGUEST_REGS GuestRegs, UINT64 GuestRip)
{
}
