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

VOID
HaltedRoutineEnableRdpmcExitingOnSingleCore(UINT32 TargetCoreId);

VOID
HaltedRoutineEnableRdtscExitingOnSingleCore(UINT32 TargetCoreId);

VOID
HaltedRoutineEnableMov2DebugRegsExitingOnSingleCore(UINT32 TargetCoreId);

VOID
HaltedRoutineEnableExternalInterruptExiting(UINT32 TargetCoreId);

VOID
HaltedRoutineSetExceptionBitmapOnSingleCore(UINT32 TargetCoreId, UINT64 ExceptionIndex);

VOID
HaltedRoutineUnSetExceptionBitmapOnSingleCore(UINT32 TargetCoreId, UINT64 ExceptionIndex);

VOID
HaltedRoutineEnableMovToCrExitingOnSingleCore(UINT32 TargetCoreId, DEBUGGER_EVENT_OPTIONS * BroadcastingOption);

VOID
HaltedRoutineEnableEferSyscallHookOnSingleCore(UINT32 TargetCoreId);

VOID
HaltedRoutineInvalidateEptAllContextsOnSingleCore(UINT32 TargetCoreId);

VOID
HaltedRoutineInvalidateSingleContextOnSingleCore(UINT32 TargetCoreId);
