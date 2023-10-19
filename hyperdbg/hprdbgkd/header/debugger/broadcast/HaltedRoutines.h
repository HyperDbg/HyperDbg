/**
 * @file HaltedRoutines.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for all single core broadcasting functions in case of halted core
 *
 * @version 0.7
 * @date 2023-10-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				    Functions					//
//////////////////////////////////////////////////

VOID
HaltedRoutineChangeAllMsrBitmapReadOnSingleCore(UINT32 TargetCoreId, UINT64 BitmapMask);

VOID
HaltedRoutineChangeAllMsrBitmapWriteOnSingleCore(UINT32 TargetCoreId, UINT64 BitmapMask);

VOID
HaltedRoutineChangeIoBitmapOnSingleCore(UINT32 TargetCoreId, UINT64 Port);
