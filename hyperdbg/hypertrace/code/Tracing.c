/**
 * @file Tracing.c
 * @author Hari Mishal (harimishal6@gmail.com)
 * @brief Tracing routines for HyperTrace module
 * @details
 * @version 0.18
 * @date 2025-12-02
 *
 * @copyright This project is released under the GNU Public License v3.
 */

#include "pch.h"

/**
 * @brief Hide debugger on transparent-mode (activate transparent-mode)
 *
 * @param HypertraceCallbacks
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceInit(HYPERTRACE_CALLBACKS * HypertraceCallbacks)
{
    //
    // Check if any of the required callbacks are NULL
    //
    for (UINT32 i = 0; i < sizeof(HYPERTRACE_CALLBACKS) / sizeof(UINT64); i++)
    {
        if (((PVOID *)HypertraceCallbacks)[i] == NULL)
        {
            //
            // The callback has null entry, so we cannot proceed
            //
            return FALSE;
        }
    }

    //
    // Save the callbacks
    //
    RtlCopyMemory(&g_Callbacks, HypertraceCallbacks, sizeof(HYPERTRACE_CALLBACKS));

    return TRUE;
}
