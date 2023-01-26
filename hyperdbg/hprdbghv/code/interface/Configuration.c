/**
 * @file Configuration.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Configuration interface for hypervisor events
 * @details
 *
 * @version 0.2
 * @date 2023-01-26
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief routines for debugging threads (enable mov-to-cr3 exiting)
 *
 * @return VOID
 */
VOID
ConfigureEnableMovToCr3ExitingOnAllProcessors()
{
    //
    // Indicate that the future #PFs should or should not be checked with user debugger
    //
    g_CheckPageFaultsAndMov2Cr3VmexitsWithUserDebugger = TRUE;

    BroadcastEnableMovToCr3ExitingOnAllProcessors();
}

/**
 * @brief routines for debugging threads (disable mov-to-cr3 exiting)
 *
 * @return VOID
 */
VOID
ConfigureDisableMovToCr3ExitingOnAllProcessors()
{
    //
    // Indicate that the future #PFs should or should not be checked with user debugger
    //
    g_CheckPageFaultsAndMov2Cr3VmexitsWithUserDebugger = FALSE;

    BroadcastDisableMovToCr3ExitingOnAllProcessors();
}

/**
 * @brief routines for enabling syscall hooks on all cores
 * @param SyscallHookType
 *
 * @return VOID
 */
VOID
ConfigureEnableEferSyscallEventsOnAllProcessors(DEBUGGER_EVENT_SYSCALL_SYSRET_TYPE SyscallHookType)
{
    if (SyscallHookType == DEBUGGER_EVENT_SYSCALL_SYSRET_HANDLE_ALL_UD)
    {
        g_IsUnsafeSyscallOrSysretHandling = TRUE;
    }
    else if (SyscallHookType == DEBUGGER_EVENT_SYSCALL_SYSRET_SAFE_ACCESS_MEMORY)
    {
        g_IsUnsafeSyscallOrSysretHandling = FALSE;
    }

    BroadcastEnableEferSyscallEventsOnAllProcessors();
}

/**
 * @brief routines for disabling syscall hooks on all cores
 *
 * @return VOID
 */
VOID
ConfigureDisableEferSyscallEventsOnAllProcessors()
{
    BroadcastDisableEferSyscallEventsOnAllProcessors();
}
