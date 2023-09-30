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
 *
 * @return VOID
 */
VOID
HaltedCoreBroadcasTaskToAllCores(PROCESSOR_DEBUGGING_STATE * DbgState,
                                 UINT32                      TargetTask,
                                 BOOLEAN                     LockAgainAfterTask)
{
    ULONG CoreCount;

    CoreCount = KeQueryActiveProcessorCount(0);

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
}
