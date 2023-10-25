/**
 * @file DebuggerEvents.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of Debugger events (triggers and enable events)
 *
 * @version 0.1
 * @date 2020-05-12
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief routines for !syscall command (enable syscall hook)
 *
 * @return VOID
 */
VOID
DebuggerEventEnableEferOnAllProcessors()
{
    ConfigureEnableEferSyscallEventsOnAllProcessors();
}

/**
 * @brief routines for !syscall command (disable syscall hook)
 *
 * @return VOID
 */
VOID
DebuggerEventDisableEferOnAllProcessors()
{
    ConfigureDisableEferSyscallEventsOnAllProcessors();
}

/**
 * @brief routines for debugging threads (enable mov-to-cr3 exiting)
 *
 * @return VOID
 */
VOID
DebuggerEventEnableMovToCr3ExitingOnAllProcessors()
{
    ConfigureEnableMovToCr3ExitingOnAllProcessors();
}

/**
 * @brief routines for debugging threads (disable mov-to-cr3 exiting)
 *
 * @return VOID
 */
VOID
DebuggerEventDisableMovToCr3ExitingOnAllProcessors()
{
    ConfigureDisableMovToCr3ExitingOnAllProcessors();
}

/**
 * @brief Event for address, we don't use address range here,
 * address ranges should be check in event section
 *
 * @param Address
 * @param ProcessId
 * @param EnableForRead
 * @param EnableForWrite
 * @param EnableForExecute
 * @param ApplyDirectlyFromVmxRoot
 *
 * @return VOID
 */
BOOLEAN
DebuggerEventEnableMonitorReadWriteExec(UINT64  Address,
                                        UINT32  ProcessId,
                                        BOOLEAN EnableForRead,
                                        BOOLEAN EnableForWrite,
                                        BOOLEAN EnableForExecute,
                                        BOOLEAN ApplyDirectlyFromVmxRoot)
{
    //
    // Check if the detail is ok for either read or write or both
    //
    if (!EnableForRead && !EnableForWrite && !EnableForExecute)
    {
        return FALSE;
    }

    //
    // If the read is FALSE and WRITE is TRUE, then the processor doesn't support
    // such a thing, we will enable the Read silently here if, this problem will be
    // solved when the trigger works, the trigger routines won't enable reads
    //
    if (EnableForWrite)
    {
        EnableForRead = TRUE;
    }

    //
    // Perform the EPT Hook
    //
    return ConfigureEptHook2(KeGetCurrentProcessorNumberEx(NULL),
                             Address,
                             NULL,
                             ProcessId,
                             EnableForRead,
                             EnableForWrite,
                             EnableForExecute,
                             FALSE,
                             ApplyDirectlyFromVmxRoot);
}

/**
 * @brief Handle process or thread switches
 *
 * @param CoreId
 *
 * @return BOOLEAN
 */
BOOLEAN
DebuggerCheckProcessOrThreadChange(_In_ UINT32 CoreId)
{
    PROCESSOR_DEBUGGING_STATE * DbgState = &g_DbgState[CoreId];

    //
    // Check whether intercepting this process or thread is active or not
    //
    if (DbgState->ThreadOrProcessTracingDetails.InterceptClockInterruptsForThreadChange ||
        DbgState->ThreadOrProcessTracingDetails.InterceptClockInterruptsForProcessChange)

    {
        //
        // We only handle interrupts that are related to the clock-timer interrupt
        //
        if (DbgState->ThreadOrProcessTracingDetails.InterceptClockInterruptsForThreadChange)
        {
            return ThreadHandleThreadChange(DbgState);
        }
        else
        {
            return ProcessHandleProcessChange(DbgState);
        }
    }

    //
    // Not handled here
    //
    return FALSE;
}
