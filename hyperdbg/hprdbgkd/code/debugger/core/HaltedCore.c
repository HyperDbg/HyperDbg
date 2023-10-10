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
 * @brief Perform the task on halted core
 * @details This function should be called from VMX root-mode
 *
 * @param DbgState The state of the debugger on the current core
 * @param TargetTask The target task
 *
 * @return VOID
 */
VOID
HaltedCorePerformTargetTask(PROCESSOR_DEBUGGING_STATE * DbgState,
                            UINT32                      TargetTask)
{
    //
    // Test target task
    //
    LogInfo("Target task executed: %x", TargetTask);
}

/**
 * @brief Run the task on a single halted core
 * @details This function should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 * @param TargetTask The target task
 * @param LockAgainAfterTask Lock the core after the task
 *
 * @return VOID
 */
VOID
HaltedCoreApplyTaskOnTargetCore(UINT32  TargetCoreId,
                                UINT32  TargetTask,
                                BOOLEAN LockAgainAfterTask)
{
    PROCESSOR_DEBUGGING_STATE * DbgState = &g_DbgState[TargetCoreId];

    //
    // Activate running the halted task
    //
    DbgState->HaltedCoreTask.PerformHaltedTask = TRUE;

    DbgState->HaltedCoreTask.KernelStatus       = NULL;
    DbgState->HaltedCoreTask.LockAgainAfterTask = LockAgainAfterTask;
    DbgState->HaltedCoreTask.TargetTask         = TargetTask;

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
 *
 * @return VOID
 */
VOID
HaltedCoreRunTaskOnSingleCore(UINT32  TargetCoreId,
                              UINT32  TargetTask,
                              BOOLEAN LockAgainAfterTask)
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
        HaltedCorePerformTargetTask(&g_DbgState[TargetCoreId], TargetTask);
    }
    else
    {
        //
        // *** Perform the task for another core ***
        //

        //
        // apply task to the target core
        //
        HaltedCoreApplyTaskOnTargetCore(TargetCoreId, TargetTask, LockAgainAfterTask);
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
 *
 * @return BOOLEAN
 */
BOOLEAN
HaltedCoreBroadcastTaskAllCores(PROCESSOR_DEBUGGING_STATE * DbgState,
                                UINT32                      TargetTask,
                                BOOLEAN                     LockAgainAfterTask,
                                BOOLEAN                     Synchronize)
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
            HaltedCoreApplyTaskOnTargetCore(i, TargetTask, LockAgainAfterTask);
        }
        else
        {
            //
            // Perform the task for the current core
            //
            HaltedCorePerformTargetTask(DbgState, TargetTask);
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
