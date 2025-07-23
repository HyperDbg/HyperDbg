/**
 * @file HyperEvade.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Hyperevade function wrappers
 * @details
 *
 * @version 0.14
 * @date 2025-06-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Wrapper for hiding debugger on transparent-mode (activate transparent-mode)
 *
 * @param HyperevadeCallbacks
 * @param TransparentModeRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
TransparentHideDebuggerWrapper(DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE * TransparentModeRequest)
{
    HYPEREVADE_CALLBACKS HyperevadeCallbacks = {0};

    //
    // *** Fill the callbacks ***
    //

    //
    // Fill the callbacks for using hyperlog in hyperevade
    // We use the callbacks directly to avoid two calls to the same function
    //
    HyperevadeCallbacks.LogCallbackPrepareAndSendMessageToQueueWrapper = g_Callbacks.LogCallbackPrepareAndSendMessageToQueueWrapper;
    HyperevadeCallbacks.LogCallbackSendMessageToQueue                  = g_Callbacks.LogCallbackSendMessageToQueue;
    HyperevadeCallbacks.LogCallbackSendBuffer                          = g_Callbacks.LogCallbackSendBuffer;
    HyperevadeCallbacks.LogCallbackCheckIfBufferIsFull                 = g_Callbacks.LogCallbackCheckIfBufferIsFull;

    //
    // Memory callbacks
    //
    HyperevadeCallbacks.CheckAccessValidityAndSafety               = CheckAccessValidityAndSafety;
    HyperevadeCallbacks.MemoryMapperReadMemorySafeOnTargetProcess  = MemoryMapperReadMemorySafeOnTargetProcess;
    HyperevadeCallbacks.MemoryMapperWriteMemorySafeOnTargetProcess = MemoryMapperWriteMemorySafeOnTargetProcess;

    //
    // Common callbacks
    //
    HyperevadeCallbacks.CommonGetProcessNameFromProcessControlBlock = CommonGetProcessNameFromProcessControlBlock;

    //
    // System call callbacks
    //
    HyperevadeCallbacks.SyscallCallbackSetTrapFlagAfterSyscall = SyscallCallbackSetTrapFlagAfterSyscall;

    //
    // VMX callbacks
    //
    HyperevadeCallbacks.HvHandleTrapFlag             = HvHandleTrapFlag;
    HyperevadeCallbacks.EventInjectGeneralProtection = EventInjectGeneralProtection;

    //
    // Initialize the syscall callback mechanism from hypervisor
    //
    if (!SyscallCallbackInitialize())
    {
        TransparentModeRequest->KernelStatus = DEBUGGER_ERROR_UNABLE_TO_HIDE_OR_UNHIDE_DEBUGGER;
        return FALSE;
    }

    //
    // Call the hyperevade hide debugger function
    //
    if (TransparentHideDebugger(&HyperevadeCallbacks, TransparentModeRequest))
    {
        //
        // Status is set within the transparent mode (hyperevade) module
        //
        g_CheckForFootprints = TRUE;
        return TRUE;
    }
    else
    {
        //
        // Status is set within the transparent mode (hyperevade) module
        //
        g_CheckForFootprints = FALSE;
        return FALSE;
    }
}

/**
 * @brief Deactivate transparent-mode
 * @param TransparentModeRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
TransparentUnhideDebuggerWrapper(DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE * TransparentModeRequest)
{
    //
    // Unitialize the syscall callback mechanism from hypervisor
    //
    SyscallCallbackUninitialize();

    if (TransparentUnhideDebugger())
    {
        //
        // Unset transparent mode for the VMM module
        //
        g_CheckForFootprints = FALSE;

        if (TransparentModeRequest != NULL)
        {
            TransparentModeRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
        }

        return TRUE;
    }
    else
    {
        if (TransparentModeRequest != NULL)
        {
            TransparentModeRequest->KernelStatus = DEBUGGER_ERROR_DEBUGGER_ALREADY_UNHIDE;
        }
        return FALSE;
    }
}
