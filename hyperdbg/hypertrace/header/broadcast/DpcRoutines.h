
/**
 * @file DpcRoutines.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Definition for DPC functions
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

BOOLEAN
DpcRoutineEnableLbr(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

BOOLEAN
DpcRoutineDisableLbr(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

BOOLEAN
DpcRoutineFlushLbr(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);
