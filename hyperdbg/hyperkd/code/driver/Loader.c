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
LoaderInitHyperTrace(PDEBUGGER_INIT_HYPERTRACE_PACKET InitHyperTracePacket, BOOLEAN RunningOnHypervisorEnvironment)
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

        //
        // Mark hypertrace as initialized
        //
        g_HyperTraceInitialized = TRUE;

        //
        // Set the kernel status to success
        //
        InitHyperTracePacket->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

        return TRUE;
    }
    else
    {
        //
        // We won't fail the loading just because of hypertrace, so we just log the error and continue without loading hypertrace
        //
        LogDebugInfo("Err, HyperDbg's hypertrace was not loaded");

        //
        // Set the kernel status to indicate failure
        //
        InitHyperTracePacket->KernelStatus = DEBUGGER_ERROR_HYPERTRACE_NOT_INITIALIZED;

        return FALSE;
    }
}

/**
 * @brief Initialize the hyper log module
 *
 * @return BOOLEAN
 */
BOOLEAN
LoaderInitHyperLog()
{
    MESSAGE_TRACING_CALLBACKS MsgTracingCallbacks = {0};

    //
    // *** Fill the callbacks for the message tracer ***
    //
    MsgTracingCallbacks.VmxOperationCheck            = VmFuncVmxGetCurrentExecutionMode;
    MsgTracingCallbacks.CheckImmediateMessageSending = KdCheckImmediateMessagingMechanism;
    MsgTracingCallbacks.SendImmediateMessage         = KdLoggingResponsePacketToDebugger;

    //
    // Initialize message tracer (if not already initialized)
    //
    if (g_HyperLogInitialized == FALSE && LogInitialize(&MsgTracingCallbacks))
    {
        g_HyperLogInitialized = TRUE;

        LogDebugInfo("HyperDbg's hyperlog loaded successfully");

        return TRUE;
    }
    else
    {
        //
        // We use DbgPrint here because if the hyperlog is not loaded we can't use it to log the error
        // so we just log the error with DbgPrint and continue without loading hyperlog
        //
        DbgPrint("Err, HyperDbg's hyperlog was not loaded or already loaded");
        return FALSE;
    }
}

/**
 * @brief Initialize the VMM
 *
 * @param InitVmmPacket The packet to fill the result of the initialization
 *
 * @return BOOLEAN
 */
BOOLEAN
LoaderInitVmm(PDEBUGGER_INIT_VMM_PACKET InitVmmPacket)
{
    VMM_CALLBACKS VmmCallbacks = {0};

    //
    // Check if KD is not already initialized, if so we cannot initialize VMM
    //
    if (!g_KdInitialized)
    {
        InitVmmPacket->KernelStatus = DEBUGGER_ERROR_VMM_CANNOT_BE_INITIALIZED_IF_DEBUGGER_IS_NOT_LOADED;
        return FALSE;
    }

    //
    // Check if HyperTrace is already initialized, if so we cannot initialize VMM
    //
    if (g_HyperTraceInitialized)
    {
        InitVmmPacket->KernelStatus = DEBUGGER_ERROR_VMM_CANNOT_BE_INITIALIZED_IF_HYPERTRACE_IS_LOADED;
        return FALSE;
    }

    //
    // *** Fill the callbacks for using hyperlog in VMM ***
    //
    VmmCallbacks.LogCallbackPrepareAndSendMessageToQueueWrapper = LogCallbackPrepareAndSendMessageToQueueWrapper;
    VmmCallbacks.LogCallbackSendMessageToQueue                  = LogCallbackSendMessageToQueue;
    VmmCallbacks.LogCallbackSendBuffer                          = LogCallbackSendBuffer;
    VmmCallbacks.LogCallbackCheckIfBufferIsFull                 = LogCallbackCheckIfBufferIsFull;

    //
    // Fill the HyperTrace callback(s)
    //
    VmmCallbacks.HyperTraceLbrIsSupported = HyperTraceLbrIsSupported;

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
    // Fill the pool manager callbacks
    //
    VmmCallbacks.PoolManagerRequestAllocation = PoolManagerRequestAllocation;
    VmmCallbacks.PoolManagerRequestPool       = PoolManagerRequestPool;
    VmmCallbacks.PoolManagerFreePool          = PoolManagerFreePool;

    //
    // Fill the interception callbacks
    //
    VmmCallbacks.InterceptionCallbackTriggerCr3ProcessChange = ProcessTriggerCr3ProcessChange;

    //
    // Initialize VMX
    //
    if (VmFuncInitVmm(&VmmCallbacks))
    {
        LogDebugInfo("HyperDbg's hypervisor loaded successfully");

        //
        // Initialize VMM opeartions (event related state from the debugger)
        //
        if (!DebuggerInitializeVmmOperations())
        {
            return FALSE;
        }

        //
        // VMM module initialized
        //
        g_VmmInitialized = TRUE;

        return TRUE;
    }
    else
    {
        LogError("Err, HyperDbg's hypervisor was not loaded");
    }

    return FALSE;
}

/**
 * @brief Initialize the debugger
 *
 * @return BOOLEAN
 */
BOOLEAN
LoaderInitKd()
{
    //
    // If the debugger is already initialized, we don't need to initialize it again
    // and simply return true
    //
    if (g_KdInitialized)
    {
        return TRUE;
    }

    //
    // The debugger is not initialized, so we try to initialize it
    //
    if (DebuggerInitialize())
    {
        LogDebugInfo("HyperDbg's debugger loaded successfully");

        //
        // KD module initialized
        //
        g_KdInitialized = TRUE;

        return TRUE;
    }

    LogError("Err, HyperDbg's debugger was not loaded");
    return FALSE;
}

/**
 * @brief Initialize the debugger and the vmm
 *
 * @param InitVmmPacket The packet to fill the result of the initialization
 *
 * @return BOOLEAN
 */
BOOLEAN
LoaderInitDebuggerAndVmm(PDEBUGGER_INIT_VMM_PACKET InitVmmPacket)
{
    //
    // First we need to initialize the debugger
    // because the VMM relies on the debugger for some of its functionalities,
    // so if we cannot initialize the debugger we cannot initialize the VMM
    //
    if (!LoaderInitKd())
    {
        //
        // Unable to initialize the debugger, so we cannot initialize the VMM, and we return false
        //
        InitVmmPacket->KernelStatus = DEBUGGER_ERROR_CANNOT_INITIALIZE_DEBUGGER;

        return FALSE;
    }

    //
    // Now we can initialize the VMM
    //
    if (!LoaderInitVmm(InitVmmPacket))
    {
        return FALSE;
    }

    //
    // Set the kernel status to success
    //
    InitVmmPacket->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    return TRUE;
}

/**
 * @brief Uninitialize the VMM
 *
 * @return VOID
 */
VOID
LoaderUninitVmm()
{
    //
    // Mark VMM as uninitialized before uninitializing it to avoid any potential reentrancy issues during the uninitialization process
    //
    g_VmmInitialized = FALSE;

    //
    // First remove all VMM related state from the debugger
    //
    DebuggerUninitializeVmmOperations();

    //
    // Terminate VMM and its sub-mechanisms
    //
    VmFuncUninitVmm();
}

/**
 * @brief Uninitialize the hyper trace module
 *
 * @return VOID
 */
VOID
LoaderUninitHyperTrace()
{
    //
    // Mark hypertrace as uninitialized before uninitializing it to avoid any potential reentrancy issues during the uninitialization process
    //
    g_HyperTraceInitialized = FALSE;

    //
    // Uninitialize the hypertrace
    //
    HyperTraceUninit();
}

/**
 * @brief Uninitialize the debugger
 *
 * @return VOID
 */
VOID
LoaderUninitKd()
{
    //
    // Mark KD as uninitialized before uninitializing it to avoid any potential reentrancy issues during the uninitialization process
    //
    g_KdInitialized = FALSE;

    //
    // Uninitialize the debugger and its sub-mechanisms
    //
    DebuggerUninitialize();
}

/**
 * @brief Uninitialize the VMM and the debugger
 *
 * @return VOID
 */
VOID
LoaderUninitVmmAndDebugger()
{
    //
    // Uninitialize the VMM first because it relies on the debugger for some
    //
    LoaderUninitVmm();

    //
    // Uninitialize the debugger
    //
    LoaderUninitKd();
}

/**
 * @brief Uninitialize the log tracer
 *
 * @return VOID
 */
VOID
LoaderUninitLogTracer()
{
#if !UseDbgPrintInsteadOfUsermodeMessageTracking

    LogDebugInfo("Unloading hyperlog...\n");

    //
    // Uinitialize log buffer if it was initialized
    //
    if (g_HyperLogInitialized)
    {
        g_HyperLogInitialized = FALSE;
        LogUnInitialize();
    }
#endif
}
