/**
 * @file DpcRoutines.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief All the dpc routines which relates to executing on a single core
 * for multi-core you can use Broadcast.c
 *
 * @version 0.1
 * @date 2020-04-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief lock for one core execution
 *
 */
volatile LONG OneCoreLock;

/**
 * @brief This function synchronize the function execution for a single core
 * You should only used it for one core, not in multiple threads simultaneously
 * The function that needs to use this feature (Routine parameter function) should
 * have the when it ends :
 *
 *              SpinlockUnlock(&OneCoreLock);
 *
 * @param CoreNumber core number that the target function should run on it
 * @param Routine the target function that should be ran
 * @param DeferredContext an optional parameter to Routine
 * @return NTSTATUS
 */
NTSTATUS
DpcRoutineRunTaskOnSingleCore(UINT32 CoreNumber, PVOID Routine, PVOID DeferredContext)
{
    PRKDPC Dpc;
    ULONG  ProcessorsCount;

    ProcessorsCount = KeQueryActiveProcessorCount(0);

    //
    // Check if the core number is not invalid
    //
    if (CoreNumber >= ProcessorsCount)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Allocate Memory for DPC
    //
    Dpc = CrsAllocateZeroedNonPagedPool(sizeof(KDPC));

    if (!Dpc)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Creating a DPC that will run on the target process
    //
    KeInitializeDpc(Dpc,                         // Dpc
                    (PKDEFERRED_ROUTINE)Routine, // DeferredRoutine
                    DeferredContext              // DeferredContext
    );

    //
    // Set the target core
    //
    KeSetTargetProcessorDpc(Dpc, (CCHAR)CoreNumber);

    //
    // it's sure will be executed, but we want to free the above
    // pool, so we have to wait on a spinlock that will be release
    // by the DPC routine, actually Affinity Thread but that
    // won't support more than 64 logical cores, I create a discussion
    // here, and explained the problem, but no one answers
    // link: https://community.osr.com/discussion/292064/putting-a-barrier-for-dpc-before-continuing-the-rest-of-code
    // we also can't use the spinlock routine of Windows as this function
    // raises the IRQL to DPC and we want to execute at DPC, means that
    // If we're currently on the right core, we never find a chance to
    // release the spinlock so a deadlock happens, all in all it's complicated :)
    //

    //
    // Set the lock to be freed by the other DPC routine
    //
    if (!SpinlockTryLock(&OneCoreLock))
    {
        //
        // We can't get the lock, probably sth goes wrong !
        //
        CrsFreePool(Dpc);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Fire the DPC
    //
    KeInsertQueueDpc(Dpc, NULL, NULL);

    //
    // spin on lock to be release, immediately after we get the lock, we'll
    // release it for because there is no need to it anymore and DPC is finished
    //
    SpinlockLock(&OneCoreLock);
    SpinlockUnlock(&OneCoreLock);

    //
    // Now it's safe to deallocate the bugger
    //
    CrsFreePool(Dpc);

    return STATUS_SUCCESS;
}

/**
 * @brief Broadcast msr write
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutinePerformWriteMsr(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    ULONG                       CurrentCore           = KeGetCurrentProcessorNumberEx(NULL);
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggingState = &g_DbgState[CurrentCore];

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //
    // write on MSR
    //
    __writemsr(CurrentDebuggingState->MsrState.Msr, CurrentDebuggingState->MsrState.Value);

    //
    // As this function is designed for a single,
    // we have to release the synchronization lock here
    //
    SpinlockUnlock(&OneCoreLock);
}

/**
 * @brief Broadcast msr read
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutinePerformReadMsr(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    ULONG                       CurrentCore           = KeGetCurrentProcessorNumberEx(NULL);
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggingState = &g_DbgState[CurrentCore];

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //
    // read on MSR
    //
    CurrentDebuggingState->MsrState.Value = __readmsr(CurrentDebuggingState->MsrState.Msr);

    //
    // As this function is designed for a single,
    // we have to release the synchronization lock here
    //
    SpinlockUnlock(&OneCoreLock);
}

/**
 * @brief Broadcast Msr Write
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineWriteMsrToAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    ULONG                       CurrentCore           = KeGetCurrentProcessorNumberEx(NULL);
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggingState = &g_DbgState[CurrentCore];

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // write on MSR
    //
    __writemsr(CurrentDebuggingState->MsrState.Msr, CurrentDebuggingState->MsrState.Value);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}

/**
 * @brief Broadcast Msr read
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineReadMsrToAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    ULONG                       CurrentCore           = KeGetCurrentProcessorNumberEx(NULL);
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggingState = &g_DbgState[CurrentCore];

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // read msr
    //
    CurrentDebuggingState->MsrState.Value = __readmsr(CurrentDebuggingState->MsrState.Msr);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}

/**
 * @brief vm-exit and halt the system
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineVmExitAndHaltSystemAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // vm-exit and halt current core
    //
    VmFuncVmxVmcall(DEBUGGER_VMCALL_VM_EXIT_HALT_SYSTEM, 0, 0, 0);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}
