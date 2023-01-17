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
        g_Events = ExAllocatePoolWithTag(NonPagedPool, sizeof(DEBUGGER_CORE_EVENTS), POOLTAG);
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
        ExFreePoolWithTag(g_Events, POOLTAG);
        g_Events = NULL;
    }

    return g_Events == NULL;
}
