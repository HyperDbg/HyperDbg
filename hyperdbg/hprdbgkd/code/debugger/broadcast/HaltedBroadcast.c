/**
 * @file HaltedBroadcast.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Broadcasting functions in case of halted cores
 *
 * @version 0.7
 * @date 2023-10-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief This function broadcasts MSR changes to all cores
 * @details Should be called from VMX root-mode
 *
 * @param BitmapMask
 *
 * @return VOID
 */
VOID
HaltedBroadcastChangeAllMsrBitmapReadAllCores(UINT64 BitmapMask)
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
    HaltedCoreBroadcastTaskAllCores(&g_DbgState[KeGetCurrentProcessorNumberEx(NULL)],
                                    HaltedCoreTask,
                                    TRUE,
                                    TRUE,
                                    &DirectVmcallOptions);
}
