/**
 * @file Allocations.c
 * @author Behrooz Abbassi (BehroozAbbassi@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Management of global variables memory relating to debugger
 * @details
 * @version 0.2
 * @date 2023-01-17
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Allocate debugging state memory
 *
 * @return BOOLEAN
 */
BOOLEAN
GlobalDebuggingStateAllocateZeroedMemory(VOID)
{
    SSIZE_T BufferSizeInByte = sizeof(PROCESSOR_DEBUGGING_STATE) * KeQueryActiveProcessorCount(0);

    //
    // Allocate global variable to hold Debugging(s) state
    //
    g_DbgState = CrsAllocateZeroedNonPagedPool(BufferSizeInByte);

    if (!g_DbgState)
    {
        //
        // we use DbgPrint as the vmx-root or non-root is not initialized
        //

        LogInfo("err, insufficient memory for allocating debugging state\n");
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Free debugging state memory
 *
 * @return VOID
 */
VOID
GlobalDebuggingStateFreeMemory(VOID)
{
    CrsFreePool(g_DbgState);
    g_DbgState = NULL;
}

/**
 * @brief Allocate event store memory
 *
 * @return BOOLEAN
 */
BOOLEAN
GlobalEventsAllocateZeroedMemory(VOID)
{
    //
    // Allocate buffer for saving events
    //
    if (!g_Events)
    {
        g_Events = CrsAllocateNonPagedPool(sizeof(DEBUGGER_CORE_EVENTS));
    }

    if (g_Events)
    {
        //
        // Zero the buffer
        //
        RtlZeroBytes(g_Events, sizeof(DEBUGGER_CORE_EVENTS));
    }

    return g_Events != NULL;
}

/**
 * @brief Free event store memory
 *
 * @return VOID
 */
VOID
GlobalEventsFreeMemory(VOID)
{
    if (g_Events != NULL)
    {
        CrsFreePool(g_Events);
        g_Events = NULL;
    }
}
