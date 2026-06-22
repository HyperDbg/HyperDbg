/**
 * @file PerfApi.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief PMU routines for HyperPerf module
 * @details
 * @version 0.21
 * @date 2026-06-22
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#include "pch.h"

/**
 * @brief Initialize the hyperperf module callbacks
 * @details This only for callback initialization, not for PMU, etc. initialization
 *
 * @param HyperPerfCallbacks Pointer to the HyperPerf callbacks structure to be registered
 * @param RunningOnHypervisorEnvironment Whether the initialization is being done for hypervisor environment or not,
 * it can be used to skip some of the initialization steps if it is not for hypervisor environment and behave differently based on that
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperPerfInitCallback(HYPERPERF_CALLBACKS * HyperPerfCallbacks,
                      BOOLEAN               RunningOnHypervisorEnvironment)
{
    //
    // Check if any of the required callbacks are NULL
    //
    for (UINT32 i = 0; i < sizeof(HYPERPERF_CALLBACKS) / sizeof(UINT64); i++)
    {
        if (((PVOID *)HyperPerfCallbacks)[i] == NULL)
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
    PlatformWriteMemory(&g_Callbacks, HyperPerfCallbacks, sizeof(HYPERPERF_CALLBACKS));

    //
    // Set the flag to indicate whether the initialization is being done for hypervisor environment or not
    //
    g_RunningOnHypervisorEnvironment = RunningOnHypervisorEnvironment;

    //
    // Enable callbacks and set the initialized flag
    //
    g_HyperPerfCallbacksInitialized = TRUE;

    return TRUE;
}

/**
 * @brief Uninitialize the hypertrace module
 *
 * @return VOID
 */
VOID
HyperPerfUninit()
{
    //
    // Check if the callbacks are initialized, if not, we don't need to handle anymore
    //
    if (!g_HyperPerfCallbacksInitialized)
    {
        return;
    }

    //
    // Reset the environment flag to default value
    //
    g_RunningOnHypervisorEnvironment = FALSE;

    //
    // Set callbacks to not initialized
    //
    g_HyperPerfCallbacksInitialized = FALSE;
}
