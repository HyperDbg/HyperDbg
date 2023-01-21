/**
 * @file Loader.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The functions used in loading the debugger and VMM
 * @version 0.2
 * @date 2023-01-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Initialize the VMM and Debugger
 *
 * @return BOOLEAN
 */
BOOLEAN
LoaderInitVmmAndDebugger()
{
    MESSAGE_TRACING_CALLBACKS MsgTracingCallbacks = {0};
    VMM_CALLBACKS             VmmCallbacks        = {0};

    //
    // Allow to server IOCTL
    //
    g_AllowIOCTLFromUsermode = TRUE;

    //
    // Fill the callbacks for the message tracer
    //
    MsgTracingCallbacks.VmxOpeationCheck             = VmFuncVmxGetCurrentExecutionMode;
    MsgTracingCallbacks.CheckImmediateMessageSending = KdCheckImmediateMessagingMechanism;
    MsgTracingCallbacks.SendImmediateMessage         = KdLoggingResponsePacketToDebugger;

    //
    // Fill the callbacks for using hyperlog in VMM
    //
    VmmCallbacks.LogPrepareAndSendMessageToQueue = LogPrepareAndSendMessageToQueue;
    VmmCallbacks.LogSendMessageToQueue           = LogSendMessageToQueue;
    VmmCallbacks.LogSendBuffer                   = LogSendBuffer;

    //
    // Fill the callbacks for the VMM module
    //
    VmmCallbacks.DebuggerTriggerEvents                          = DebuggerTriggerEvents;
    VmmCallbacks.DebuggerSetLastError                           = DebuggerSetLastError;
    VmmCallbacks.BreakpointCheckAndHandleDebugBreakpoint        = BreakpointCheckAndHandleDebugBreakpoint;
    VmmCallbacks.BreakpointCheckAndHandleReApplyingBreakpoint   = BreakpointCheckAndHandleReApplyingBreakpoint;
    VmmCallbacks.BreakpointHandleBpTraps                        = BreakpointHandleBpTraps;
    VmmCallbacks.UdCheckForCommand                              = UdCheckForCommand;
    VmmCallbacks.ProcessTriggerCr3ProcessChange                 = ProcessTriggerCr3ProcessChange;
    VmmCallbacks.DebuggerCheckProcessOrThreadChange             = DebuggerCheckProcessOrThreadChange;
    VmmCallbacks.AttachingCheckPageFaultsWithUserDebugger       = AttachingCheckPageFaultsWithUserDebugger;
    VmmCallbacks.AttachingHandleCr3VmexitsForThreadInterception = AttachingHandleCr3VmexitsForThreadInterception;
    VmmCallbacks.UserAccessCheckForLoadedModuleDetails          = UserAccessCheckForLoadedModuleDetails;
    VmmCallbacks.KdHandleNmiBroadcastDebugBreaks                = KdHandleNmiBroadcastDebugBreaks;
    VmmCallbacks.KdCheckAndHandleNmiCallback                    = KdCheckAndHandleNmiCallback;

    //
    // Initialize message tracer
    //
    if (LogInitialize(&MsgTracingCallbacks))
    {
        //
        // Initialize Vmx
        //
        if (VmFuncInitVmm(&VmmCallbacks))
        {
            LogDebugInfo("HyperDbg's hypervisor loaded successfully");

            //
            // Initialize the debugger
            //
            if (DebuggerInitialize())
            {
                LogDebugInfo("HyperDbg's debugger loaded successfully");

                //
                // Set the variable so no one else can get a handle anymore
                //
                g_HandleInUse = TRUE;

                return TRUE;
            }
            else
            {
                LogError("Err, HyperDbg's debugger was not loaded");
            }
        }
        else
        {
            LogError("Err, HyperDbg's hypervisor was not loaded");
        }
    }
    else
    {
        LogError("Err, HyperDbg's message tracing module was not loaded");
    }

    //
    // Not loaded
    //
    g_AllowIOCTLFromUsermode = FALSE;

    return FALSE;
}

/**
 * @brief Uninitialize the log tracer
 *
 * @return VOID
 */
VOID
LoaderUninitializeLogTracer()
{
    LogDebugInfo("Unloading HyperDbg's debugger...\n");

#if !UseDbgPrintInsteadOfUsermodeMessageTracking

    //
    // Uinitialize log buffer
    //
    LogDebugInfo("Uninitializing logs\n");
    LogUnInitialize();
#endif
}
