/**
 * @file PlatformDpc.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of cross platform APIs for Deferred Procedure Call (DPC) management
 * @details
 * @version 0.19
 * @date 2026-05-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/PlatformDpc.h"
#endif // defined(__linux__)

/**
 * @brief Initialize a DPC object
 *
 * @param Dpc Pointer to the KDPC structure to initialize
 * @param DeferredRoutine The deferred procedure to be called
 * @param DeferredContext Optional context passed to the deferred routine
 * @return VOID
 */
VOID
PlatformDpcInitialize(PRKDPC Dpc, PKDEFERRED_ROUTINE DeferredRoutine, PVOID DeferredContext)
{
#if defined(_WIN32) || defined(_WIN64)

    KeInitializeDpc(Dpc, DeferredRoutine, DeferredContext);

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Insert a DPC into the system DPC queue for execution
 *
 * @param Dpc Pointer to the initialized KDPC structure
 * @param SystemArgument1 First system-defined argument passed to the deferred routine
 * @param SystemArgument2 Second system-defined argument passed to the deferred routine
 * @return BOOLEAN TRUE if the DPC was successfully queued, FALSE if it was already in the queue
 */
BOOLEAN
PlatformDpcInsertQueueDpc(PRKDPC Dpc, PVOID SystemArgument1, PVOID SystemArgument2)
{
#if defined(_WIN32) || defined(_WIN64)

    return KeInsertQueueDpc(Dpc, SystemArgument1, SystemArgument2);

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}
