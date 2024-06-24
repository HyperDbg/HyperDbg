/**
 * @file HaltedCore.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of applying events in halted cores
 * @details
 *
 * @version 0.7
 * @date 2023-09-30
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Perform the test task on halted core
 *
 * @param DbgState The state of the debugger on the current core
 * @param Context optional parameter passed to the functions
 *
 * @return VOID
 */
VOID
HaltedCoreTaskTest(PROCESSOR_DEBUGGING_STATE * DbgState, PVOID Context)
{
    UNREFERENCED_PARAMETER(DbgState);

    //
    // Test target task
    //
    LogInfo("Target test task executed on halted core, context: %llx", Context);
}

/**
 * @brief Perform the task on halted core
 * @details This function should be called from VMX root-mode
 *
 * @param DbgState The state of the debugger on the current core
 * @param TargetTask The target task
 * @param Context optional parameter passed to the functions
 *
 * @return VOID
 */
VOID
HaltedCorePerformTargetTask(PROCESSOR_DEBUGGING_STATE * DbgState,
                            UINT64                      TargetTask,
                            PVOID                       Context)
{
    switch (TargetTask)
    {
    case DEBUGGER_HALTED_CORE_TASK_TEST:
    {
        //
        // Perform the test task
        //
        HaltedCoreTaskTest(DbgState, Context);
        break;
    }

    case DEBUGGER_HALTED_CORE_TASK_RUN_VMCALL:
    {
        //
        // Call the direct VMCALL test function
        //
        DirectVmcallTest(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_SET_PROCESS_INTERCEPTION:
    {
        //
        // Enable process change detection
        //
        ProcessEnableOrDisableThreadChangeMonitor(DbgState, TRUE, PVOID_TO_BOOLEAN(Context));

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_SET_THREAD_INTERCEPTION:
    {
        //
        // Enable alert for thread changes
        //
        ThreadEnableOrDisableThreadChangeMonitor(DbgState, TRUE, PVOID_TO_BOOLEAN(Context));

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_CHANGE_MSR_BITMAP_READ:
    {
        //
        // Change MSR bitmap for read (RDMSR)
        //
        DirectVmcallChangeMsrBitmapRead(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_CHANGE_MSR_BITMAP_WRITE:
    {
        //
        // Change MSR bitmap for write (WRMSR)
        //
        DirectVmcallChangeMsrBitmapWrite(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_CHANGE_IO_BITMAP:
    {
        //
        // Change I/O bitmap
        //
        DirectVmcallChangeIoBitmap(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_SET_RDPMC_EXITING:
    {
        //
        // Enable rdpmc exiting
        //
        DirectVmcallEnableRdpmcExiting(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_SET_RDTSC_EXITING:
    {
        //
        // Enable rdtsc/rdtscp exiting
        //
        DirectVmcallEnableRdtscpExiting(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_ENABLE_MOV_TO_DEBUG_REGS_EXITING:
    {
        //
        // Enable mov to debug registers exiting
        //
        DirectVmcallEnableMov2DebugRegsExiting(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_SET_EXCEPTION_BITMAP:
    {
        //
        // Set exception bitmap
        //
        DirectVmcallSetExceptionBitmap(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_ENABLE_EXTERNAL_INTERRUPT_EXITING:
    {
        //
        // enable external interrupt exiting
        //
        DirectVmcallEnableExternalInterruptExiting(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_ENABLE_MOV_TO_CONTROL_REGS_EXITING:
    {
        //
        // enable mov to CR exiting
        //
        DirectVmcallEnableMovToCrExiting(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_ENABLE_SYSCALL_HOOK_EFER:
    {
        //
        // enable syscall hook using EFER SCE bit
        //
        DirectVmcallEnableEferSyscall(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_INVEPT_ALL_CONTEXTS:
    {
        //
        // invalidate EPT (All Contexts)
        //
        DirectVmcallInvalidateEptAllContexts(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_INVEPT_SINGLE_CONTEXT:
    {
        //
        // invalidate EPT (A Single Context)
        //
        DirectVmcallInvalidateSingleContext(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_UNSET_EXCEPTION_BITMAP:
    {
        //
        // unset exception bitmap on VMCS
        //
        DirectVmcallUnsetExceptionBitmap(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_UNHOOK_SINGLE_PAGE:
    {
        //
        // restore a single EPT entry and invalidate EPT cache
        //
        DirectVmcallUnhookSinglePage(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_DISABLE_EXTERNAL_INTERRUPT_EXITING_ONLY_TO_CLEAR_INTERRUPT_COMMANDS:
    {
        //
        // disable external interrupt exiting only to clear !interrupt commands
        //
        DirectVmcallSetDisableExternalInterruptExitingOnlyOnClearingInterruptEvents(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_RESET_MSR_BITMAP_READ:
    {
        //
        // reset MSR Bitmap Read
        //
        DirectVmcallResetMsrBitmapRead(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_RESET_MSR_BITMAP_WRITE:
    {
        //
        // reset MSR Bitmap Write
        //
        DirectVmcallResetMsrBitmapWrite(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_RESET_EXCEPTION_BITMAP_ONLY_ON_CLEARING_EXCEPTION_EVENTS:
    {
        //
        // reset exception bitmap on VMCS
        //
        DirectVmcallResetExceptionBitmapOnlyOnClearingExceptionEvents(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_RESET_IO_BITMAP:
    {
        //
        // reset I/O Bitmaps (A & B)
        //
        DirectVmcallResetIoBitmap(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_DISABLE_RDTSC_EXITING_ONLY_FOR_TSC_EVENTS:
    {
        //
        // clear rdtsc exiting bit ONLY in the case of disabling the events for !tsc command
        //
        DirectVmcallDisableRdtscExitingForClearingTscEvents(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_UNSET_RDPMC_EXITING:
    {
        //
        // disable rdpmc exiting in primary cpu-based controls
        //
        DirectVmcallDisableRdpmcExiting(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_DISABLE_SYSCALL_HOOK_EFER:
    {
        //
        // disable syscall hook using EFER SCE bit
        //
        DirectVmcallDisableEferSyscallEvents(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_DISABLE_MOV_TO_HW_DR_EXITING_ONLY_FOR_DR_EVENTS:
    {
        //
        // clear mov 2 hw dr exiting bit ONLY in the case of disabling the events for !dr command
        //
        DirectVmcallDisableMov2DrExitingForClearingDrEvents(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_DISABLE_MOV_TO_CR_EXITING_ONLY_FOR_CR_EVENTS:
    {
        //
        // clear mov 2 cr exiting bit ONLY in the case of disabling the events for !crwrite command
        //
        DirectVmcallDisableMov2CrExitingForClearingCrEvents(DbgState->CoreId, (DIRECT_VMCALL_PARAMETERS *)Context);

        break;
    }
    default:
        LogWarning("Warning, unknown broadcast on halted core received");
        break;
    }
}

/**
 * @brief Run the task on a single halted core
 * @details This function should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 * @param TargetTask The target task
 * @param LockAgainAfterTask Lock the core after the task
 * @param Context optional parameter passed to the functions
 *
 * @return VOID
 */
VOID
HaltedCoreApplyTaskOnTargetCore(UINT32  TargetCoreId,
                                UINT64  TargetTask,
                                BOOLEAN LockAgainAfterTask,
                                PVOID   Context)
{
    PROCESSOR_DEBUGGING_STATE * DbgState = &g_DbgState[TargetCoreId];

    //
    // Activate running the halted task
    //
    DbgState->HaltedCoreTask.PerformHaltedTask = TRUE;

    DbgState->HaltedCoreTask.KernelStatus       = (UINT64)NULL;
    DbgState->HaltedCoreTask.LockAgainAfterTask = LockAgainAfterTask;
    DbgState->HaltedCoreTask.TargetTask         = TargetTask;
    DbgState->HaltedCoreTask.Context            = Context;

    //
    // Unlock halted core
    //
    KdUnlockTheHaltedCore(DbgState);
}

/**
 * @brief Run the task on a single halted core
 * @details This function should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 * @param TargetTask The target task
 * @param LockAgainAfterTask Lock the core after the task
 * @param Context optional parameter passed to the functions
 *
 * @return VOID
 */
VOID
HaltedCoreRunTaskOnSingleCore(UINT32  TargetCoreId,
                              UINT64  TargetTask,
                              BOOLEAN LockAgainAfterTask,
                              PVOID   Context)
{
    //
    // Check if the task needs to be executed for the current
    // core or any other cores
    //
    if (TargetCoreId == KeGetCurrentProcessorNumberEx(NULL))
    {
        //
        // *** Perform the task for the current core ***
        //
        HaltedCorePerformTargetTask(&g_DbgState[TargetCoreId], TargetTask, Context);
    }
    else
    {
        //
        // *** Perform the task for another core ***
        //

        //
        // apply task to the target core
        //
        HaltedCoreApplyTaskOnTargetCore(TargetCoreId, TargetTask, LockAgainAfterTask, Context);
    }
}

/**
 * @brief Broadcast tasks to halted cores
 * @details This function should be called from VMX root-mode
 *
 * @param DbgState The state of the debugger on the current core
 * @param TargetTask The target task
 * @param LockAgainAfterTask Lock the core after the task
 * @param Synchronize Whether the function should wait for all cores to synchronize
 * and lock again or not
 * @param Context optional parameter passed to the functions
 *
 * @return BOOLEAN
 */
BOOLEAN
HaltedCoreBroadcastTaskAllCores(PROCESSOR_DEBUGGING_STATE * DbgState,
                                UINT64                      TargetTask,
                                BOOLEAN                     LockAgainAfterTask,
                                BOOLEAN                     Synchronize,
                                PVOID                       Context)
{
    ULONG ProcessorsCount;

    ProcessorsCount = KeQueryActiveProcessorCount(0);

    //
    // Synchronization is not possible when the locking after the task is
    // not expected
    //
    if (Synchronize && !LockAgainAfterTask)
    {
        LogWarning("Synchronization is not possible when the locking after the task is not expected");
        return FALSE;
    }

    //
    // Apply the task to all cores except current core
    //
    for (UINT32 i = 0; i < ProcessorsCount; i++)
    {
        if (DbgState->CoreId != i)
        {
            //
            // apply task to the target core
            //
            HaltedCoreApplyTaskOnTargetCore(i, TargetTask, LockAgainAfterTask, Context);
        }
        else
        {
            //
            // Perform the task for the current core
            //
            HaltedCorePerformTargetTask(DbgState, TargetTask, Context);
        }
    }

    //
    // If synchronization is expected, we need to check to make sure
    // all cores are synchronized (locked) at this point or not
    //
    if (Synchronize)
    {
        for (size_t i = 0; i < ProcessorsCount; i++)
        {
            if (DbgState->CoreId != i)
            {
                //
                // Wait until the core is locked again
                //
                while (TRUE)
                {
                    //
                    // Keep checking to make sure the target core finished the
                    // execution its task and locked again
                    //
                    if (KdCheckTheHaltedCore(&g_DbgState[i]) == FALSE)
                    {
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }

    //
    // All cores locked again
    //
    return TRUE;
}
