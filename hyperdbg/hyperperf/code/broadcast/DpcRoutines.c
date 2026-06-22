/**
 * @file DpcRoutines.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief DPC routines
 * @details
 * @version 0.21
 * @date 2026-06-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Broadcast enabling LBR
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return BOOLEAN
 */
BOOLEAN
DpcRoutineTestPmu(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    // ------------------------------------------------------------------------------
    // Synchronize the end of this routine with the caller
    //
    PlatformBroadcastSynchronizeEndOfRoutine(SystemArgument1, SystemArgument2);

    return TRUE;
}
