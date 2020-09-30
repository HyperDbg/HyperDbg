/**
 * @file DpcRoutines.c
 * @author Sina Karvandi (sina@rayanfam.com)
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
 * The function that needs to use this featue (Routine parameter function) should 
 * have the when it ends :
 *  
 *              SpinlockUnlock(&OneCoreLock); 
 * 
 * @param CoreNumber core number that the target function should run on it
 * @param Routine the target function that should be runned
 * @param DeferredContext an optional parameter to Routine
 * @return NTSTATUS 
 */
NTSTATUS
DpcRoutineRunTaskOnSingleCore(UINT32 CoreNumber, PVOID Routine, PVOID DeferredContext)
{
    PRKDPC Dpc;
    UINT32 ProcessorCount;

    ProcessorCount = KeQueryActiveProcessorCount(0);

    //
    // Check if the core number is not invalid
    //
    if (CoreNumber >= ProcessorCount)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Allocate Memory for DPC
    //
    Dpc = ExAllocatePoolWithTag(NonPagedPool, sizeof(KDPC), POOLTAG);

    if (!Dpc)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Creating a DPC that will run on the target process
    //
    KeInitializeDpc(Dpc,            // Dpc
                    Routine,        // DeferredRoutine
                    DeferredContext // DeferredContext
    );

    //
    // Set the target core
    //
    KeSetTargetProcessorDpc(Dpc, CoreNumber);

    //
    // it's sure will be executed, but we want to free the above
    // pool, so we have to wait on a spinlock that will be release
    // by the the DPC routine, actually Affinity Thread but that
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
    ExFreePoolWithTag(Dpc, POOLTAG);

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
    ULONG CurrentProcessorIndex = 0;

    CurrentProcessorIndex = KeGetCurrentProcessorNumber();

    //
    // write on MSR
    //
    __writemsr(g_GuestState[CurrentProcessorIndex].DebuggingState.MsrState.Msr, g_GuestState[CurrentProcessorIndex].DebuggingState.MsrState.Value);

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
    ULONG CurrentProcessorIndex = 0;

    CurrentProcessorIndex = KeGetCurrentProcessorNumber();

    //
    // read on MSR
    //
    g_GuestState[CurrentProcessorIndex].DebuggingState.MsrState.Value = __readmsr(g_GuestState[CurrentProcessorIndex].DebuggingState.MsrState.Msr);

    //
    // As this function is designed for a single,
    // we have to release the synchronization lock here
    //
    SpinlockUnlock(&OneCoreLock);
}

/**
 * @brief change msr bitmap read on a single core
 * 
 * @param Dpc 
 * @param DeferredContext 
 * @param SystemArgument1 
 * @param SystemArgument2 
 * @return VOID 
 */
VOID
DpcRoutinePerformChangeMsrBitmapReadOnSingleCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // change msr bitmap (read)
    //
    AsmVmxVmcall(VMCALL_CHANGE_MSR_BITMAP_READ, DeferredContext, 0, 0);

    //
    // As this function is designed for a single,
    // we have to release the synchronization lock here
    //
    SpinlockUnlock(&OneCoreLock);
}

/**
 * @brief change msr bitmap write on a single core
 * 
 * @param Dpc 
 * @param DeferredContext 
 * @param SystemArgument1 
 * @param SystemArgument2 
 * @return VOID 
 */
VOID
DpcRoutinePerformChangeMsrBitmapWriteOnSingleCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // change msr bitmap (write)
    //
    AsmVmxVmcall(VMCALL_CHANGE_MSR_BITMAP_WRITE, DeferredContext, 0, 0);

    //
    // As this function is designed for a single,
    // we have to release the synchronization lock here
    //
    SpinlockUnlock(&OneCoreLock);
}

/**
 * @brief set rdtsc/rdtscp exiting
 * 
 * @param Dpc 
 * @param DeferredContext 
 * @param SystemArgument1 
 * @param SystemArgument2 
 * @return VOID 
 */
VOID
DpcRoutinePerformEnableRdtscExitingOnSingleCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // enable rdtsc/rdtscp exiting
    //
    AsmVmxVmcall(VMCALL_SET_RDTSC_EXITING, 0, 0, 0);

    //
    // As this function is designed for a single,
    // we have to release the synchronization lock here
    //
    SpinlockUnlock(&OneCoreLock);
}

/**
 * @brief set rdpmc exiting
 * 
 * @param Dpc 
 * @param DeferredContext 
 * @param SystemArgument1 
 * @param SystemArgument2 
 * @return VOID 
 */
VOID
DpcRoutinePerformEnableRdpmcExitingOnSingleCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // enable rdtsc/rdtscp exiting
    //
    AsmVmxVmcall(VMCALL_SET_RDPMC_EXITING, 0, 0, 0);

    //
    // As this function is designed for a single,
    // we have to release the synchronization lock here
    //
    SpinlockUnlock(&OneCoreLock);
}

/**
 * @brief change exception bitmap on a single core
 * 
 * @param Dpc 
 * @param DeferredContext 
 * @param SystemArgument1 
 * @param SystemArgument2 
 * @return VOID 
 */
VOID
DpcRoutinePerformSetExceptionBitmapOnSingleCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // change exception bitmap
    //
    AsmVmxVmcall(VMCALL_SET_EXCEPTION_BITMAP, DeferredContext, 0, 0);

    //
    // As this function is designed for a single,
    // we have to release the synchronization lock here
    //
    SpinlockUnlock(&OneCoreLock);
}

/**
 * @brief Set the Mov to Debug Registers Exitings
 * 
 * @param Dpc 
 * @param DeferredContext 
 * @param SystemArgument1 
 * @param SystemArgument2 
 * @return VOID 
 */
VOID
DpcRoutinePerformEnableMovToDebugRegistersExiting(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // enable Mov to Debug Registers Exitings
    //
    AsmVmxVmcall(VMCALL_ENABLE_MOV_TO_DEBUG_REGS_EXITING, 0, 0, 0);

    //
    // As this function is designed for a single,
    // we have to release the synchronization lock here
    //
    SpinlockUnlock(&OneCoreLock);
}

/**
 * @brief Enable external interrupt exiting on a single core
 * 
 * @param Dpc 
 * @param DeferredContext 
 * @param SystemArgument1 
 * @param SystemArgument2 
 * @return VOID 
 */
VOID
DpcRoutinePerformSetExternalInterruptExitingOnSingleCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Enable external interrupt exiting
    //
    AsmVmxVmcall(VMCALL_ENABLE_EXTERNAL_INTERRUPT_EXITING, NULL, 0, 0);

    //
    // As this function is designed for a single,
    // we have to release the synchronization lock here
    //
    SpinlockUnlock(&OneCoreLock);
}

/**
 * @brief Enable syscall hook EFER on a single core
 * 
 * @param Dpc 
 * @param DeferredContext 
 * @param SystemArgument1 
 * @param SystemArgument2 
 * @return VOID 
 */
VOID
DpcRoutinePerformEnableEferSyscallHookOnSingleCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Enable syscall hook EFER
    //
    AsmVmxVmcall(VMCALL_ENABLE_SYSCALL_HOOK_EFER, NULL, 0, 0);

    //
    // As this function is designed for a single,
    // we have to release the synchronization lock here
    //
    SpinlockUnlock(&OneCoreLock);
}

/**
 * @brief change I/O bitmap on a single core
 * 
 * @param Dpc 
 * @param DeferredContext 
 * @param SystemArgument1 
 * @param SystemArgument2 
 * @return VOID 
 */
VOID
DpcRoutinePerformChangeIoBitmapOnSingleCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // change I/O bitmap
    //
    AsmVmxVmcall(VMCALL_CHANGE_IO_BITMAP, DeferredContext, 0, 0);

    //
    // As this function is designed for a single,
    // we have to release the synchronization lock here
    //
    SpinlockUnlock(&OneCoreLock);
}
