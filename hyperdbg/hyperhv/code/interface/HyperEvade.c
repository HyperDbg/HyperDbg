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
    // Fill the callbacks for using hyperlog in VMM
    //
    // HyperevadeCallbacks.LogCallbackPrepareAndSendMessageToQueueWrapper = LogCallbackPrepareAndSendMessageToQueue;
    HyperevadeCallbacks.LogCallbackSendMessageToQueue  = LogCallbackSendMessageToQueue;
    HyperevadeCallbacks.LogCallbackSendBuffer          = LogCallbackSendBuffer;
    HyperevadeCallbacks.LogCallbackCheckIfBufferIsFull = LogCallbackCheckIfBufferIsFull;

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

    // SYSCALL_CALLBACK_INITIALIZE                  SyscallCallbackInitialize;
    // SYSCALL_CALLBACK_UNINITIALIZE                SyscallCallbackUninitialize;

    //
    // Call the hyperevade hide debugger function
    //
    return TransparentHideDebugger(&HyperevadeCallbacks, TransparentModeRequest);
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
    return TransparentUnhideDebugger(TransparentModeRequest);
}
