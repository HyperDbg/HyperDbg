/**
 * @file Synchronization.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Routines synchronization objects
 * @details
 * @version 0.16
 * @date 2025-08-30
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Initialize a waiting event
 *
 * @param Event
 * @return VOID
 */
VOID
SynchronizationInitializeEvent(PRKEVENT Event)
{
    //
    // Initialize an event
    //
    KeInitializeEvent(Event, SynchronizationEvent, FALSE);
}

/**
 * @brief Set (signal) a waiting event
 *
 * @param Event
 * @return VOID
 */
VOID
SynchronizationSetEvent(PRKEVENT Event)
{
    //
    // Set (signal) an event
    //
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
}

/**
 * @brief Wait for a waiting event
 *
 * @param Event
 * @return VOID
 */
VOID
SynchronizationWaitForEvent(PRKEVENT Event)
{
    //
    // Wait for an event
    //
    KeWaitForSingleObject(Event,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
}
