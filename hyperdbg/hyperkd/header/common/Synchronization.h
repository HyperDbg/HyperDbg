/**
 * @file Synchronization.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Routines for synchronization objects
 * @details
 *
 * @version 0.16
 * @date 2025-08-30
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				   Functions	    			//
//////////////////////////////////////////////////

VOID
SynchronizationInitializeEvent(PRKEVENT Event);

VOID
SynchronizationSetEvent(PRKEVENT Event);

VOID
SynchronizationWaitForEvent(PRKEVENT Event);
