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
#include <ntddk.h>

NTSTATUS
DpcRoutineRunTaskOnSingleCore(UINT32 CoreNumber, PVOID Routine, PVOID DeferredContext);

VOID
DpcRoutinePerformWriteMsr(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
DpcRoutinePerformReadMsr(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);
