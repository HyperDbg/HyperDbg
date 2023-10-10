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
HaltedCoreBroadcastTaskToAllCores(PROCESSOR_DEBUGGING_STATE * DbgState,
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
            // Activate running the halted task
            //
            g_DbgState[i].HaltedCoreTask.PerformHaltedTask = TRUE;

            g_DbgState[i].HaltedCoreTask.KernelStatus       = NULL;
            g_DbgState[i].HaltedCoreTask.LockAgainAfterTask = LockAgainAfterTask;
            g_DbgState[i].HaltedCoreTask.TargetTask         = TargetTask;

            //
            // Unlock halted core
            //
            KdUnlockTheHaltedCore(&g_DbgState[i]);
        }
    }

    //
    // Perform the task for the current core
    //
    HaltedCorePerformTargetTask(DbgState, TargetTask);

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
