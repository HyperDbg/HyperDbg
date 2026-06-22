/**
 * @file Broadcast.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Broadcasting functions
 * @details
 * @version 0.21
 * @date 2026-06-22
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
    KeGenericCallDpc(DpcRoutineTestPmu, NULL);
}
