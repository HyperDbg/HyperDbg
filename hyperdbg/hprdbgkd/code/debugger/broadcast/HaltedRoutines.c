/**
 * @file HaltedRoutines.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief All single core broadcasting functions in case of halted core
 *
 * @version 0.7
 * @date 2023-10-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief This function performs running MSR changes (RDMSR) on a single core
 * @details Should be called from VMX root-mode
 *
 * @param TargetCoreId The target core's ID (to just run on this core)
 * @param BitmapMask
 *
 * @return VOID
 */
VOID
HaltedRoutineChangeAllMsrBitmapReadOnSingleCore(UINT32 TargetCoreId, UINT64 BitmapMask)
{
    DIRECT_VMCALL_PARAMETERS DirectVmcallOptions = {0};
    UINT64                   HaltedCoreTask      = NULL;

    //
    // Set the target task
    //
    HaltedCoreTask = DEBUGGER_HALTED_CORE_TASK_CHANGE_MSR_BITMAP_READ;

    //
    // Set the parameters for the direct VMCALL
    //
    DirectVmcallOptions.OptionalParam1 = BitmapMask;

    //
    // Send request for the target task to the halted cores (synchronized)
    //
    HaltedCoreRunTaskOnSingleCore(TargetCoreId,
                                  HaltedCoreTask,
                                  TRUE,
                                  TRUE,
                                  &DirectVmcallOptions);
}
