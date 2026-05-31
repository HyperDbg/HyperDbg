/**
 * @file Broadcast.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Broadcasting functions
 *
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

/**
 * @brief Routines to enable PT on all cores
 *
 * @return VOID
 */
VOID
BroadcastEnablePtOnAllCores()
{
    KeGenericCallDpc(DpcRoutineEnablePt, NULL);
}

/**
 * @brief Routines to disable PT on all cores
 *
 * @return VOID
 */
VOID
BroadcastDisablePtOnAllCores()
{
    KeGenericCallDpc(DpcRoutineDisablePt, NULL);
}

/**
 * @brief Routines to pause PT tracing on all cores
 *
 * @return VOID
 */
VOID
BroadcastPausePtOnAllCores()
{
    KeGenericCallDpc(DpcRoutinePausePt, NULL);
}

/**
 * @brief Routines to resume PT tracing on all cores
 *
 * @return VOID
 */
VOID
BroadcastResumePtOnAllCores()
{
    KeGenericCallDpc(DpcRoutineResumePt, NULL);
}

/**
 * @brief Routines to snapshot per-CPU PT output positions. The DPC
 *        writes its own core's byte count into Sizes[CoreId]; the
 *        caller's UINT64 array must hold at least one slot per CPU.
 *
 * @return VOID
 */
VOID
BroadcastSizePtOnAllCores(UINT64 * Sizes)
{
    KeGenericCallDpc(DpcRoutineSizePt, (PVOID)Sizes);
}

/**
 * @brief Routines to dump PT state on all cores
 *
 * @return VOID
 */
VOID
BroadcastDumpPtOnAllCores()
{
    KeGenericCallDpc(DpcRoutineDumpPt, NULL);
}

/**
 * @brief Routines to flush PT state on all cores
 *
 * @return VOID
 */
VOID
BroadcastFlushPtOnAllCores()
{
    KeGenericCallDpc(DpcRoutineFlushPt, NULL);
}

/**
 * @brief Routines to apply a PT filter on all cores. The same Options
 *        pointer is passed to every per-core DPC; KeGenericCallDpc is
 *        synchronous so the caller's storage is valid throughout.
 *
 * @return VOID
 */
VOID
BroadcastFilterPtOnAllCores(PT_FILTER_OPTIONS * FilterOptions)
{
    KeGenericCallDpc(DpcRoutineFilterPt, (PVOID)FilterOptions);
}
