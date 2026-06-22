
/**
 * @file DpcRoutines.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Definition for DPC functions
 * @details
 * @version 0.21
 * @date 2026-06-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				    Functions					//
//////////////////////////////////////////////////

BOOLEAN
DpcRoutineTestPmu(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);
