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
 * @brief Initialize the hyper trace module
 *
 * @param RunningOnHypervisorEnvironment Whether the initialization is being done for hypervisor environment or not
 *
 * @return BOOLEAN
 */
BOOLEAN
LoaderInitHyperTrace(BOOLEAN RunningOnHypervisorEnvironment)
{
    HYPERTRACE_CALLBACKS HyperTraceCallbacks = {0};

    //
    // *** Fill the callbacks for using hypertrace ***
    //

    //
    // Fill the callbacks for using hyperlog in hypertrace
    // We use the callbacks directly to avoid two calls to the same function
    //
    HyperTraceCallbacks.LogCallbackPrepareAndSendMessageToQueueWrapper = LogCallbackPrepareAndSendMessageToQueueWrapper;
    HyperTraceCallbacks.LogCallbackSendMessageToQueue                  = LogCallbackSendMessageToQueue;
    HyperTraceCallbacks.LogCallbackSendBuffer                          = LogCallbackSendBuffer;
    HyperTraceCallbacks.LogCallbackCheckIfBufferIsFull                 = LogCallbackCheckIfBufferIsFull;

    //
    // Fill the callbacks for using hyperhv in hypertrace
    //
    HyperTraceCallbacks.VmFuncVmxGetCurrentExecutionMode = VmFuncVmxGetCurrentExecutionMode;

    //
    // *** Legacy LBR callbacks ***
    //

    HyperTraceCallbacks.VmFuncCheckCpuSupportForSaveAndLoadDebugControls = VmFuncCheckCpuSupportForSaveAndLoadDebugControls;

    HyperTraceCallbacks.VmFuncGetDebugctl                   = VmFuncGetDebugctl;
    HyperTraceCallbacks.VmFuncGetDebugctlVmcallOnTargetCore = VmFuncGetDebugctlVmcallOnTargetCore;
    HyperTraceCallbacks.VmFuncSetDebugctl                   = VmFuncSetDebugctl;
    HyperTraceCallbacks.VmFuncSetDebugctlVmcallOnTargetCore = VmFuncSetDebugctlVmcallOnTargetCore;

    HyperTraceCallbacks.VmFuncSetLoadDebugControls                   = VmFuncSetLoadDebugControls;
    HyperTraceCallbacks.VmFuncSetLoadDebugControlsVmcallOnTargetCore = VmFuncSetLoadDebugControlsVmcallOnTargetCore;
    HyperTraceCallbacks.VmFuncSetSaveDebugControls                   = VmFuncSetSaveDebugControls;
    HyperTraceCallbacks.VmFuncSetSaveDebugControlsVmcallOnTargetCore = VmFuncSetSaveDebugControlsVmcallOnTargetCore;

    HyperTraceCallbacks.VmFuncSetLbrSelect                   = VmFuncSetLbrSelect;
    HyperTraceCallbacks.VmFuncSetLbrSelectVmcallOnTargetCore = VmFuncSetLbrSelectVmcallOnTargetCore;

    //
    // *** Architectural LBR callbacks ***
    //

    HyperTraceCallbacks.VmFuncCheckCpuSupportForLoadAndClearGuestIa32LbrCtlControls = VmFuncCheckCpuSupportForLoadAndClearGuestIa32LbrCtlControls;

    HyperTraceCallbacks.VmFuncGetGuestIa32LbrCtl                   = VmFuncGetGuestIa32LbrCtl;
    HyperTraceCallbacks.VmFuncGetGuestIa32LbrCtlVmcallOnTargetCore = VmFuncGetGuestIa32LbrCtlVmcallOnTargetCore;
    HyperTraceCallbacks.VmFuncSetGuestIa32LbrCtl                   = VmFuncSetGuestIa32LbrCtl;
    HyperTraceCallbacks.VmFuncSetGuestIa32LbrCtlVmcallOnTargetCore = VmFuncSetGuestIa32LbrCtlVmcallOnTargetCore;

    HyperTraceCallbacks.VmFuncSetLoadGuestIa32LbrCtl                    = VmFuncSetLoadGuestIa32LbrCtl;
    HyperTraceCallbacks.VmFuncSetLoadGuestIa32LbrCtlVmcallOnTargetCore  = VmFuncSetLoadGuestIa32LbrCtlVmcallOnTargetCore;
    HyperTraceCallbacks.VmFuncSetClearGuestIa32LbrCtl                   = VmFuncSetClearGuestIa32LbrCtl;
    HyperTraceCallbacks.VmFuncSetClearGuestIa32LbrCtlVmcallOnTargetCore = VmFuncSetClearGuestIa32LbrCtlVmcallOnTargetCore;

    //
    // Initialize hypertrace module
    //
    if (HyperTraceInitCallback(&HyperTraceCallbacks, RunningOnHypervisorEnvironment))
    {
        LogDebugInfo("HyperDbg's hypertrace loaded successfully");
        return TRUE;
    }
    else
    {
        //
        // We won't fail the loading just because of hypertrace, so we just log the error and continue without loading hypertrace
        //
        LogDebugInfo("Err, HyperDbg's hypertrace was not loaded");
        return FALSE;
    }
}

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
    g_AllowIoctlFromUsermode = TRUE;

    //
    // *** Fill the callbacks for the message tracer ***
    //
    MsgTracingCallbacks.VmxOperationCheck            = VmFuncVmxGetCurrentExecutionMode;
    MsgTracingCallbacks.CheckImmediateMessageSending = KdCheckImmediateMessagingMechanism;
    MsgTracingCallbacks.SendImmediateMessage         = KdLoggingResponsePacketToDebugger;

    //
    // *** Fill the callbacks for using hyperlog in VMM ***
    //
    VmmCallbacks.LogCallbackPrepareAndSendMessageToQueueWrapper = LogCallbackPrepareAndSendMessageToQueueWrapper;
    VmmCallbacks.LogCallbackSendMessageToQueue                  = LogCallbackSendMessageToQueue;
    VmmCallbacks.LogCallbackSendBuffer                          = LogCallbackSendBuffer;
    VmmCallbacks.LogCallbackCheckIfBufferIsFull                 = LogCallbackCheckIfBufferIsFull;

    //
    // Fill the VMM callbacks
    //
    VmmCallbacks.VmmCallbackTriggerEvents                   = DebuggerTriggerEvents;
    VmmCallbacks.VmmCallbackSetLastError                    = DebuggerSetLastError;
    VmmCallbacks.VmmCallbackVmcallHandler                   = DebuggerVmcallHandler;
    VmmCallbacks.VmmCallbackRegisteredMtfHandler            = KdHandleRegisteredMtfCallback;
    VmmCallbacks.VmmCallbackNmiBroadcastRequestHandler      = KdHandleNmiBroadcastDebugBreaks;
    VmmCallbacks.VmmCallbackQueryTerminateProtectedResource = TerminateQueryDebuggerResource;
    VmmCallbacks.VmmCallbackRestoreEptState                 = UserAccessCheckForLoadedModuleDetails;
    VmmCallbacks.VmmCallbackCheckUnhandledEptViolations     = AttachingCheckUnhandledEptViolation;

    //
    // Fill the debugging callbacks
    //
    VmmCallbacks.DebuggingCallbackHandleBreakpointException                = BreakpointHandleBreakpoints;
    VmmCallbacks.DebuggingCallbackHandleDebugBreakpointException           = BreakpointCheckAndHandleDebugBreakpoint;
    VmmCallbacks.BreakpointCheckAndHandleReApplyingBreakpoint              = BreakpointCheckAndHandleReApplyingBreakpoint;
    VmmCallbacks.DebuggerCheckProcessOrThreadChange                        = DebuggerCheckProcessOrThreadChange;
    VmmCallbacks.DebuggingCallbackCheckThreadInterception                  = AttachingCheckThreadInterceptionWithUserDebugger;
    VmmCallbacks.KdCheckAndHandleNmiCallback                               = KdCheckAndHandleNmiCallback;
    VmmCallbacks.KdQueryDebuggerQueryThreadOrProcessTracingDetailsByCoreId = KdQueryDebuggerQueryThreadOrProcessTracingDetailsByCoreId;

    //
    // Fill the interception callbacks
    //
    VmmCallbacks.InterceptionCallbackTriggerCr3ProcessChange = ProcessTriggerCr3ProcessChange;

    //
    // Initialize message tracer
    //
    if (LogInitialize(&MsgTracingCallbacks))
    {
        //
        // Initialize VMX
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
    g_AllowIoctlFromUsermode = FALSE;

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
