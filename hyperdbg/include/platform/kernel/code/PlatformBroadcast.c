/**
 * @file PlatformBroadcast.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of cross platform APIs for broadcasting routines
 * @details
 * @version 0.19
 * @date 2026-05-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/PlatformBroadcast.h"
#endif // defined(__linux__)

/**
 * @brief This function synchronize the function execution for a single core
 *
 * @return VOID
 */
VOID
PlatformBroadcastSynchronizeEndOfRoutine(PVOID SystemArgument1, PVOID SystemArgument2)
{
#if defined(_WIN32) || defined(_WIN64)
    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);

#elif defined(__linux__)
    //
    // Not needed for Linux
    //
#else
#    error "Unsupported platform"
#endif
}
