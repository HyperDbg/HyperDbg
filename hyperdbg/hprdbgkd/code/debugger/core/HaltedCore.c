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
                            UINT32                      TargetTask,
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
        ProcessEnableOrDisableThreadChangeMonitor(DbgState, TRUE, (BOOLEAN)Context);

        break;
    }
    case DEBUGGER_HALTED_CORE_TASK_SET_THREAD_INTERCEPTION:
    {
        //
        // Enable alert for thread changes
        //
        ThreadEnableOrDisableThreadChangeMonitor(DbgState, TRUE, (BOOLEAN)Context);

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
                                UINT32  TargetTask,
                                BOOLEAN LockAgainAfterTask,
                                PVOID   Context)
{
    PROCESSOR_DEBUGGING_STATE * DbgState = &g_DbgState[TargetCoreId];

    //
    // Activate running the halted task
    //
    DbgState->HaltedCoreTask.PerformHaltedTask = TRUE;

    DbgState->HaltedCoreTask.KernelStatus       = NULL;
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
                              UINT32  TargetTask,
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
                                UINT32                      TargetTask,
                                BOOLEAN                     LockAgainAfterTask,
                                BOOLEAN                     Synchronize,
                                PVOID                       Context)
{
    ULONG CoreCount;

    CoreCount = KeQueryActiveProcessorCount(0);

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
    for (size_t i = 0; i < CoreCount; i++)
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
        for (size_t i = 0; i < CoreCount; i++)
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
