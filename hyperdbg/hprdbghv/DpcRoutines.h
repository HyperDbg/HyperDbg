/**
 * @file DpcRoutines.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief headers of all the dpc routines which relates to executing on a single core
 * 
 * @version 0.1
 * @date 2020-04-29
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

#pragma once

//////////////////////////////////////////////////
//				    Functions					//
//////////////////////////////////////////////////

NTSTATUS
DpcRoutineRunTaskOnSingleCore(UINT32 CoreNumber, PVOID Routine, PVOID DeferredContext);

VOID
DpcRoutinePerformWriteMsr(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
DpcRoutinePerformReadMsr(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
DpcRoutinePerformChangeMsrBitmapReadOnSingleCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
DpcRoutinePerformChangeMsrBitmapWriteOnSingleCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
DpcRoutinePerformEnableRdtscExitingOnSingleCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
DpcRoutinePerformEnableRdpmcExitingOnSingleCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
DpcRoutinePerformSetExceptionBitmapOnSingleCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
DpcRoutinePerformEnableMovToDebugRegistersExiting(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
DpcRoutinePerformSetExternalInterruptExitingOnSingleCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
DpcRoutinePerformEnableEferSyscallHookOnSingleCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
DpcRoutinePerformChangeIoBitmapOnSingleCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);
