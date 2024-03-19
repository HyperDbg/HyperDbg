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
    Dpc = CrsAllocateNonPagedPool(sizeof(KDPC));

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
 * @brief Broadcast VmxPerformVirtualizationOnSpecificCore
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return BOOLEAN
 */
BOOLEAN
DpcRoutinePerformVirtualization(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Allocates Vmx regions for all logical cores (Vmxon region and Vmcs region)
    //
    VmxPerformVirtualizationOnSpecificCore();

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);

    return TRUE;
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
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //
    // change msr bitmap (read)
    //
    AsmVmxVmcall(VMCALL_CHANGE_MSR_BITMAP_READ, (UINT64)DeferredContext, 0, 0);

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
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //
    // change msr bitmap (write)
    //
    AsmVmxVmcall(VMCALL_CHANGE_MSR_BITMAP_WRITE, (UINT64)DeferredContext, 0, 0);

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
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

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
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

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
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //
    // change exception bitmap
    //
    AsmVmxVmcall(VMCALL_SET_EXCEPTION_BITMAP, (UINT64)DeferredContext, 0, 0);

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
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

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
 * @brief Set the Mov to Control Registers Exitings
 *
 * @param Dpc
 * @param Event
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutinePerformEnableMovToControlRegisterExiting(KDPC * Dpc, DEBUGGER_EVENT_OPTIONS * EventOptions, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //
    // enable Mov to Control Registers Exitings
    //
    AsmVmxVmcall(VMCALL_ENABLE_MOV_TO_CONTROL_REGS_EXITING, EventOptions->OptionalParam1, EventOptions->OptionalParam2, 0);

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
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //
    // Enable external interrupt exiting
    //
    AsmVmxVmcall(VMCALL_ENABLE_EXTERNAL_INTERRUPT_EXITING, NULL64_ZERO, 0, 0);

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
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //
    // Enable syscall hook EFER
    //
    AsmVmxVmcall(VMCALL_ENABLE_SYSCALL_HOOK_EFER, NULL64_ZERO, 0, 0);

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
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //
    // change I/O bitmap
    //
    AsmVmxVmcall(VMCALL_CHANGE_IO_BITMAP, (UINT64)DeferredContext, 0, 0);

    //
    // As this function is designed for a single,
    // we have to release the synchronization lock here
    //
    SpinlockUnlock(&OneCoreLock);
}

/**
 * @brief Broadcast to enable mov-to-cr3 exitings
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineEnableMovToCr3Exiting(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Enable mov-to-cr3 exiting from vmx-root
    //
    AsmVmxVmcall(VMCALL_ENABLE_MOV_TO_CR3_EXITING, 0, 0, 0);

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
 * @brief Broadcast to change to MBEC supported EPTP
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineChangeToMbecSupportedEptp(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Change to a MBEC supported EPTP from vmx-root
    //
    AsmVmxVmcall(VMCALL_CHANGE_TO_MBEC_SUPPORTED_EPTP, 0, 0, 0);

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
 * @brief Broadcast to restore to normal EPTP
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineRestoreToNormalEptp(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Restore to normal EPTP from vmx-root
    //
    AsmVmxVmcall(VMCALL_RESTORE_TO_NORMAL_EPTP, 0, 0, 0);

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
 * @brief Broadcast to enable or disable MBEC
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineEnableOrDisableMbec(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Enable/Disable MBEC from vmx-root
    //
    AsmVmxVmcall(VMCALL_DISABLE_OR_ENABLE_MBEC, (UINT64)DeferredContext, 0, 0);

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
 * @brief Broadcast to disable mov-to-cr3 exitings
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineDisableMovToCr3Exiting(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Disable mov-to-cr3 exiting from vmx-root
    //
    AsmVmxVmcall(VMCALL_DISABLE_MOV_TO_CR3_EXITING, 0, 0, 0);

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
 * @brief Broadcast syscall hook to all cores
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineEnableEferSyscallEvents(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Enable Syscall hook from vmx-root
    //
    AsmVmxVmcall(VMCALL_ENABLE_SYSCALL_HOOK_EFER, 0, 0, 0);

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
 * @brief Broadcast syscall unhook to all cores
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineDisableEferSyscallEvents(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Disable Syscall hook from vmx-root
    //
    AsmVmxVmcall(VMCALL_DISABLE_SYSCALL_HOOK_EFER, 0, 0, 0);

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
 * @brief Broadcast enable PML on all cores
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineEnablePml(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Enable PML from vmx-root
    //
    AsmVmxVmcall(VMCALL_ENABLE_DIRTY_LOGGING_MECHANISM, 0, 0, 0);

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
 * @brief Broadcast disable PML on all cores
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineDisablePml(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Disable PML from vmx-root
    //
    AsmVmxVmcall(VMCALL_DISABLE_DIRTY_LOGGING_MECHANISM, 0, 0, 0);

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
 * @brief Disable Msr Bitmaps on all cores (vm-exit on all msrs)
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineChangeMsrBitmapReadOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);

    //
    // Disable msr bitmaps from vmx-root
    //
    AsmVmxVmcall(VMCALL_CHANGE_MSR_BITMAP_READ, (UINT64)DeferredContext, 0, 0);

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
 * @brief Reset Msr Bitmaps on all cores (vm-exit on all msrs)
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineResetMsrBitmapReadOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Reset msr bitmaps from vmx-root
    //
    AsmVmxVmcall(VMCALL_RESET_MSR_BITMAP_READ, NULL64_ZERO, 0, 0);

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
 * @brief Disable Msr Bitmaps on all cores (vm-exit on all msrs)
 *
 * @param Dpc
 * @param DeferredContext Msr index to be masked on msr bitmap
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineChangeMsrBitmapWriteOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);

    //
    // Disable msr bitmaps from vmx-root
    //
    AsmVmxVmcall(VMCALL_CHANGE_MSR_BITMAP_WRITE, (UINT64)DeferredContext, 0, 0);

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
 * @brief Reset Msr Bitmaps on all cores (vm-exit on all msrs)
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineResetMsrBitmapWriteOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Reset msr bitmaps from vmx-root
    //
    AsmVmxVmcall(VMCALL_RESET_MSR_BITMAP_WRITE, NULL64_ZERO, 0, 0);

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
 * @brief Enables rdtsc/rdtscp exiting in primary cpu-based controls
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineEnableRdtscExitingAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Enables rdtsc/rdtscp exiting in primary cpu-based controls
    //
    AsmVmxVmcall(VMCALL_SET_RDTSC_EXITING, 0, 0, 0);

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
 * @brief Disables rdtsc/rdtscp exiting in primary cpu-based controls
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineDisableRdtscExitingAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Disables rdtsc/rdtscp exiting in primary cpu-based controls
    //
    AsmVmxVmcall(VMCALL_UNSET_RDTSC_EXITING, 0, 0, 0);

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
 * @brief Disables rdtsc/rdtscp exiting in primary cpu-based controls
 * ONLY for clearing !tsc events
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineDisableRdtscExitingForClearingTscEventsAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Disables rdtsc/rdtscp exiting in primary cpu-based controls
    // ONLY for clearing events
    //
    AsmVmxVmcall(VMCALL_DISABLE_RDTSC_EXITING_ONLY_FOR_TSC_EVENTS, 0, 0, 0);

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
 * @brief Disables mov to debug registers exiting
 * ONLY for clearing !dr events
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineDisableMov2DrExitingForClearingDrEventsAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Disables mov 2 hw debug regs exiting ONLY for clearing events
    //
    AsmVmxVmcall(VMCALL_DISABLE_MOV_TO_HW_DR_EXITING_ONLY_FOR_DR_EVENTS, 0, 0, 0);

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
 * @brief Disables mov to control registers exiting
 * ONLY for clearing !crwrite events
 *
 * @param Dpc
 * @param Event
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineDisableMov2CrExitingForClearingCrEventsAllCores(KDPC * Dpc, DEBUGGER_EVENT_OPTIONS * EventOptions, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);

    //
    // Disables mov 2 control regs exiting ONLY for clearing events
    //
    AsmVmxVmcall(VMCALL_DISABLE_MOV_TO_CR_EXITING_ONLY_FOR_CR_EVENTS, EventOptions->OptionalParam1, EventOptions->OptionalParam2, 0);

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
 * @brief Enables rdpmc exiting in primary cpu-based controls
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineEnableRdpmcExitingAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Enables rdpmc exiting in primary cpu-based controls
    //
    AsmVmxVmcall(VMCALL_SET_RDPMC_EXITING, 0, 0, 0);

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
 * @brief Disable rdpmc exiting in primary cpu-based controls
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineDisableRdpmcExitingAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Disable rdpmc exiting in primary cpu-based controls
    //
    AsmVmxVmcall(VMCALL_UNSET_RDPMC_EXITING, 0, 0, 0);

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
 * @brief Enable Exception Bitmaps on all cores
 *
 * @param Dpc
 * @param DeferredContext Exception index on IDT
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineSetExceptionBitmapOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);

    //
    // Enable Exception Bitmaps from vmx-root
    //
    AsmVmxVmcall(VMCALL_SET_EXCEPTION_BITMAP, (UINT64)DeferredContext, 0, 0);

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
 * @brief Disable Exception Bitmaps on all cores
 *
 * @param Dpc
 * @param DeferredContext Exception index on IDT
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineUnsetExceptionBitmapOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);

    //
    // Disable Exception Bitmaps from vmx-root
    //
    AsmVmxVmcall(VMCALL_UNSET_EXCEPTION_BITMAP, (UINT64)DeferredContext, 0, 0);

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
 * @brief Reset Exception Bitmaps on all cores
 * @details This function should ONLY be used in clearing !exception events
 *
 * @param Dpc
 * @param DeferredContext Exception index on IDT
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineResetExceptionBitmapOnlyOnClearingExceptionEventsOnAllCores(KDPC * Dpc,
                                                                      PVOID  DeferredContext,
                                                                      PVOID  SystemArgument1,
                                                                      PVOID  SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Reset Exception Bitmaps from vmx-root
    //
    AsmVmxVmcall(VMCALL_RESET_EXCEPTION_BITMAP_ONLY_ON_CLEARING_EXCEPTION_EVENTS, NULL64_ZERO, 0, 0);

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
 * @brief Enables mov debug registers exitings
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineEnableMovDebigRegisterExitingAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Enables mov debug registers exitings in primary cpu-based controls
    //
    AsmVmxVmcall(VMCALL_ENABLE_MOV_TO_DEBUG_REGS_EXITING, 0, 0, 0);

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
 * @brief Enables mov control registers exitings
 *
 * @param Dpc
 * @param Event
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineEnableMovControlRegisterExitingAllCores(KDPC * Dpc, DEBUGGER_EVENT_OPTIONS * EventOptions, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);

    //
    // Enables mov control registers exitings in primary cpu-based controls
    //
    AsmVmxVmcall(VMCALL_ENABLE_MOV_TO_CONTROL_REGS_EXITING, EventOptions->OptionalParam1, EventOptions->OptionalParam2, 0);

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
 * @brief Disables mov control registers exitings
 *
 * @param Dpc
 * @param Event
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineDisableMovControlRegisterExitingAllCores(KDPC * Dpc, DEBUGGER_EVENT_OPTIONS * EventOptions, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);

    //
    // Disable mov control registers exitings in primary cpu-based controls
    //
    AsmVmxVmcall(VMCALL_DISABLE_MOV_TO_CONTROL_REGS_EXITING, EventOptions->OptionalParam1, EventOptions->OptionalParam2, 0);

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
 * @brief Disables mov debug registers exitings
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineDisableMovDebigRegisterExitingAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Disable mov debug registers exitings in primary cpu-based controls
    //
    AsmVmxVmcall(VMCALL_DISABLE_MOV_TO_DEBUG_REGS_EXITING, 0, 0, 0);

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
 * @brief Enable vm-exit on all cores for external interrupts
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineSetEnableExternalInterruptExitingOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Enable External Interrupts vm-exit from vmx-root
    //
    AsmVmxVmcall(VMCALL_ENABLE_EXTERNAL_INTERRUPT_EXITING, 0, 0, 0);

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
 * @brief Disable vm-exit on all cores for external interrupts only for clearing !interrupt events
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineSetDisableExternalInterruptExitingOnlyOnClearingInterruptEventsOnAllCores(KDPC * Dpc,
                                                                                    PVOID  DeferredContext,
                                                                                    PVOID  SystemArgument1,
                                                                                    PVOID  SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Disable External Interrupts vm-exit from vmx-root
    //
    AsmVmxVmcall(VMCALL_DISABLE_EXTERNAL_INTERRUPT_EXITING_ONLY_TO_CLEAR_INTERRUPT_COMMANDS, 0, 0, 0);

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
 * @brief Change I/O Bitmaps on all cores
 *
 * @param Dpc
 * @param DeferredContext I/O Port index
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineChangeIoBitmapOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);

    //
    // Change I/O Bitmaps on all cores
    //
    AsmVmxVmcall(VMCALL_CHANGE_IO_BITMAP, (UINT64)DeferredContext, 0, 0);

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
 * @brief Reset I/O Bitmaps on all cores
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineResetIoBitmapOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Reset I/O Bitmaps on all cores
    //
    AsmVmxVmcall(VMCALL_RESET_IO_BITMAP, NULL64_ZERO, 0, 0);

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
 * @brief Enable breakpoint exiting on exception bitmaps on all cores
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineEnableBreakpointOnExceptionBitmapOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Change exception bitmap's #BP bit
    //
    AsmVmxVmcall(VMCALL_SET_EXCEPTION_BITMAP, EXCEPTION_VECTOR_BREAKPOINT, 0, 0);

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
 * @brief Disable breakpoint exiting on exception bitmaps on all cores
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineDisableBreakpointOnExceptionBitmapOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Change exception bitmap
    //
    AsmVmxVmcall(VMCALL_UNSET_EXCEPTION_BITMAP, EXCEPTION_VECTOR_BREAKPOINT, 0, 0);

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
 * @brief Enable vm-exit on NMIs on all cores
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineEnableNmiVmexitOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Cause vm-exit on NMIs
    //
    AsmVmxVmcall(VMCALL_SET_VM_EXIT_ON_NMIS, NULL64_ZERO, 0, 0);

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
 * @brief Disable vm-exit on NMIs on all cores
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineDisableNmiVmexitOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Cause no vm-exit on NMIs
    //
    AsmVmxVmcall(VMCALL_UNSET_VM_EXIT_ON_NMIS, NULL64_ZERO, 0, 0);

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
 * @brief Enable vm-exit on #DBs and #BPs on all cores
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineEnableDbAndBpExitingOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Cause vm-exit on #BPs
    //
    AsmVmxVmcall(VMCALL_SET_EXCEPTION_BITMAP, EXCEPTION_VECTOR_BREAKPOINT, 0, 0);

    //
    // Cause vm-exit on #DBs
    //
    AsmVmxVmcall(VMCALL_SET_EXCEPTION_BITMAP, EXCEPTION_VECTOR_DEBUG_BREAKPOINT, 0, 0);

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
 * @brief Disable vm-exit on #DBs and #BPs on all cores
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineDisableDbAndBpExitingOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Cause no vm-exit on #BPs
    //
    AsmVmxVmcall(VMCALL_UNSET_EXCEPTION_BITMAP, EXCEPTION_VECTOR_BREAKPOINT, 0, 0);

    //
    // Cause no vm-exit on #DBs
    //
    AsmVmxVmcall(VMCALL_UNSET_EXCEPTION_BITMAP, EXCEPTION_VECTOR_DEBUG_BREAKPOINT, 0, 0);

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
 * @brief The broadcast function which removes all the hooks and invalidate TLB
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineRemoveHookAndInvalidateAllEntriesOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Execute the VMCALL to remove the hook and invalidate
    //
    AsmVmxVmcall(VMCALL_UNHOOK_ALL_PAGES, NULL64_ZERO, NULL64_ZERO, NULL64_ZERO);

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
 * @brief The broadcast function which removes the single hook and invalidate TLB
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineRemoveHookAndInvalidateSingleEntryOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    EPT_SINGLE_HOOK_UNHOOKING_DETAILS * UnhookingDetail = (EPT_SINGLE_HOOK_UNHOOKING_DETAILS *)DeferredContext;

    //
    // Execute the VMCALL to remove the hook and invalidate
    //
    AsmVmxVmcall(VMCALL_UNHOOK_SINGLE_PAGE, UnhookingDetail->PhysicalAddress, UnhookingDetail->OriginalEntry, NULL64_ZERO);

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
 * @brief The broadcast function which invalidate EPT using Vmcall
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineInvalidateEptOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);

    if (DeferredContext == NULL)
    {
        //
        // We have to invalidate all contexts
        //
        AsmVmxVmcall(VMCALL_INVEPT_ALL_CONTEXTS, NULL64_ZERO, NULL64_ZERO, NULL64_ZERO);
    }
    else
    {
        //
        // We have to invalidate all contexts
        //
        AsmVmxVmcall(VMCALL_INVEPT_SINGLE_CONTEXT,
                     g_GuestState[KeGetCurrentProcessorNumberEx(NULL)].EptPointer.AsUInt,
                     NULL64_ZERO,
                     NULL64_ZERO);
    }

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
 * @brief The broadcast function which initialize the guest
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineInitializeGuest(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Save the vmx state and prepare vmcs setup and finally execute vmlaunch instruction
    //
    AsmVmxSaveState();

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
 * @brief The broadcast function which terminate the guest
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return VOID
 */
VOID
DpcRoutineTerminateGuest(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Terminate Vmx using vmcall
    //
    if (!VmxTerminate())
    {
        LogError("Err, there were an error terminating vmx");
    }

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}
