/**
 * @file Broadcast.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Broadcasting functions
 * @details
 * @version 0.19
 * @date 2026-04-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Routines to enable LBR on all cores
 *
 * @return VOID
 */
VOID
BroadcastEnableLbrOnAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineEnableLbr, NULL);
}

/**
 * @brief Routines to disable LBR on all cores
 *
 * @return VOID
 */
VOID
BroadcastDisableLbrOnAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableLbr, NULL);
}

/**
 * @brief Routines to flush LBR on all cores
 *
 * @return VOID
 */
VOID
BroadcastFlushLbrOnAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineFlushLbr, NULL);
}

/**
 * @brief Routines to filter LBR option on all cores
 *
 * @param LbrFilterOptions A bitmask of filter options to apply to the LBR branches
 *
 * @return VOID
 */
VOID
BroadcastFilterLbrOptionsOnAllCores(UINT64 LbrFilterOptions)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineFilterLbrOptions, (PVOID)(UINT_PTR)LbrFilterOptions);
}
