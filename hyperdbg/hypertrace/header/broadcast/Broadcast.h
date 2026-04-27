
/**
 * @file Broadcast.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for broadcasting functions
 * @details
 * @version 0.19
 * @date 2026-04-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				    Functions					//
//////////////////////////////////////////////////

VOID
BroadcastEnableLbrOnAllCores();

VOID
BroadcastDisableLbrOnAllCores();

VOID
BroadcastFlushLbrOnAllCores();

VOID
BroadcastFilterLbrOptionsOnAllCores(UINT64 LbrFilterOptions);
