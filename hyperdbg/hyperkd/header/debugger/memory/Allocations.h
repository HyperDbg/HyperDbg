/**
 * @file Allocations.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for management of global variables
 * @details
 * @version 0.2
 * @date 2023-01-17
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

BOOLEAN
GlobalDebuggingStateAllocateZeroedMemory(VOID);

BOOLEAN
GlobalEventsAllocateZeroedMemory(VOID);

VOID
    GlobalEventsFreeMemory(VOID);

VOID
    GlobalDebuggingStateFreeMemory(VOID);
