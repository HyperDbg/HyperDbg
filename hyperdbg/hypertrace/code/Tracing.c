/**
 * @file Tracing.c
 * @author Hari Mishal (harimishal6@gmail.com)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Tracing routines for HyperTrace module
 * @details
 * @version 0.18
 * @date 2025-12-02
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#include "pch.h"

/**
 * @brief Example of performing LBR trace
 *
 * @param ApplyFromVmxRootMode
 * @param ApplyByVmcall
 *
 * @return BOOLEAN
 */
VOID
HyperTraceExamplePerformLbrTrace(BOOLEAN ApplyFromVmxRootMode, BOOLEAN ApplyByVmcall)
{
    LBR_IOCTL_REQUEST Request = {0};

    KAFFINITY Affinity = 1;
    KeSetSystemAffinityThread(Affinity);

    LbrInitialize();

    if (!LbrCheck())
    {
        return;
    }

    Request.LbrConfig.Pid       = 0;
    Request.LbrConfig.LbrSelect = LBR_SELECT;

    if (LbrStartLbr(&Request, ApplyFromVmxRootMode, ApplyByVmcall))
    {
        for (volatile int i = 0; i < 50; i++)
        {
            if (i % 2)
            {
                int a = i * 2;
                a += 5;
            }
            else
            {
                __nop();
                __nop();
            }
        }

        LBR_STATE * State = LbrFindLbrState(0);

        if (State)
        {
            LbrGetLbr(State, ApplyFromVmxRootMode, ApplyByVmcall);
        }

        LogInfo("Dumping LBR Buffer...\n");

        LbrDumpLbr(&Request, ApplyFromVmxRootMode, ApplyByVmcall);
        LbrStopLbr(&Request, ApplyFromVmxRootMode, ApplyByVmcall);
    }

    KeRevertToUserAffinityThread();
}

/**
 * @brief Start LBR tracing for HyperTrace
 * @param ApplyFromVmxRootMode
 * @param ApplyByVmcall
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceStartLbr(BOOLEAN ApplyFromVmxRootMode, BOOLEAN ApplyByVmcall)
{
    LBR_IOCTL_REQUEST Request = {0};

    Request.LbrConfig.Pid       = 0;
    Request.LbrConfig.LbrSelect = LBR_SELECT;

    return LbrStartLbr(&Request, ApplyFromVmxRootMode, ApplyByVmcall);
}

/**
 * @brief Stop LBR tracing for HyperTrace
 * @param ApplyFromVmxRootMode
 * @param ApplyByVmcall
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceStopLbr(BOOLEAN ApplyFromVmxRootMode, BOOLEAN ApplyByVmcall)
{
    LBR_IOCTL_REQUEST Request = {0};

    Request.LbrConfig.Pid       = 0;
    Request.LbrConfig.LbrSelect = LBR_SELECT;

    LBR_STATE * State = LbrFindLbrState(0);

    if (State)
    {
        LbrGetLbr(State, ApplyFromVmxRootMode, ApplyByVmcall);
    }

    LogInfo("Dumping LBR Buffer...\n");
    LbrDumpLbr(&Request, ApplyFromVmxRootMode, ApplyByVmcall);

    return LbrStopLbr(&Request, ApplyFromVmxRootMode, ApplyByVmcall);
}

/**
 * @brief Initialize the hyper trace module callbacks
 * @details This only for callback initialization, not for LBR initialization
 *
 * @param HypertraceCallbacks
 * @param InitForHypervisorEnvironment Whether the initialization is being done for hypervisor environment or not,
 * it can be used to skip some of the initialization steps if it is not for hypervisor environment and behave differently based on that
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceInitCallback(HYPERTRACE_CALLBACKS * HypertraceCallbacks,
                       BOOLEAN                InitForHypervisorEnvironment)
{
    ULONG ProcessorsCount;

    //
    // Check if the LBR is supported on this CPU before initializing the hypertrace module,
    //
    if (!LbrCheck())
    {
        return FALSE;
    }

    //
    // Check if any of the required callbacks are NULL
    //
    for (UINT32 i = 0; i < sizeof(HYPERTRACE_CALLBACKS) / sizeof(UINT64); i++)
    {
        if (((PVOID *)HypertraceCallbacks)[i] == NULL)
        {
            //
            // The callback has null entry, so we cannot proceed
            //
            return FALSE;
        }
    }

    //
    // Save the callbacks
    //
    RtlCopyMemory(&g_Callbacks, HypertraceCallbacks, sizeof(HYPERTRACE_CALLBACKS));

    //
    // Read number of cores
    //
    ProcessorsCount = KeQueryActiveProcessorCount(0);

    //
    // Initialize the memory for LBR requests on all cores
    //
    g_LbrRequestState = PlatformAllocateMemory(sizeof(LBR_IOCTL_REQUEST) * ProcessorsCount);

    //
    // Set the flag to indicate whether the initialization is being done for hypervisor environment or not
    //
    g_InitForHypervisorEnvironment = InitForHypervisorEnvironment;

    //
    // It is initialized, but LBR is disabled at this stage
    //
    g_LastBranchRecordEnabled = FALSE;

    //
    // Enable callbacks and set the initialized flag
    //
    g_HyperTraceCallbacksInitialized = TRUE;

    return TRUE;
}

/**
 * @brief Query the state of LBR save and load VM exit and entry controls
 *
 * @param CoreId
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceQueryStateOfLbrSaveAndLoadVmExitAndEntryControls(UINT32 CoreId)
{
    UNREFERENCED_PARAMETER(CoreId); // Right now there is no core specifc controls for LBR

    return g_LastBranchRecordEnabled;
}

/**
 * @brief Enable LBR tracing for HyperTrace
 *
 * @param HyperTraceOperationRequest
 * @param ApplyFromVmxRootMode
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceEnableLbrTracing(HYPERTRACE_OPERATION_PACKETS * HyperTraceOperationRequest,
                           BOOLEAN                        ApplyFromVmxRootMode)
{
    UNREFERENCED_PARAMETER(ApplyFromVmxRootMode);

    ULONG ProcessorsCount;

    //
    // Check if LBR is already enabled or not
    //
    if (g_LastBranchRecordEnabled)
    {
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_LBR_ALREADY_ENABLED;
        return FALSE;
    }

    //
    // Check LBR support on CPU
    //
    if (!LbrCheck())
    {
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_LBR_NOT_SUPPORTED;
        return FALSE;
    }

    //
    // Check VMCS support for LBR if the initialization is being done for hypervisor environment
    //
    if (g_InitForHypervisorEnvironment && !g_Callbacks.VmFuncCheckCpuSupportForSaveAndLoadDebugControls())
    {
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_DEBUGCTL_NOT_SUPPORTED_ON_VMCS;
        return FALSE;
    }

    //
    // Enabling LBR
    //
    LbrInitialize();

    //
    // Read number of cores
    //
    ProcessorsCount = KeQueryActiveProcessorCount(0);

    for (size_t i = 0; i < ProcessorsCount; i++)
    {
        g_LbrRequestState[i].LbrConfig.Pid       = 0;
        g_LbrRequestState[i].LbrConfig.LbrSelect = LBR_SELECT;
    }

    //
    // Broadcast enabling LBR on all cores
    //
    BroadcastEnableLbrOnAllCores();

    //
    // Set the flag to indicate that LBR tracing is enabled
    //
    g_LastBranchRecordEnabled = TRUE;

    //
    // Set successful status
    //
    HyperTraceOperationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    return TRUE;
}

/**
 * @brief Disable LBR tracing for HyperTrace
 *
 * @param HyperTraceOperationRequest
 * @param ApplyFromVmxRootMode
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceDisableLbrTracing(HYPERTRACE_OPERATION_PACKETS * HyperTraceOperationRequest,
                            BOOLEAN                        ApplyFromVmxRootMode)
{
    UNREFERENCED_PARAMETER(ApplyFromVmxRootMode);

    //
    // Check if LBR is already disabled or not
    //
    if (!g_LastBranchRecordEnabled)
    {
        if (HyperTraceOperationRequest != NULL)
        {
            HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_LBR_ALREADY_DISABLED;
        }

        return FALSE;
    }

    //
    // Broadcast disabling LBR on all cores
    //
    BroadcastDisableLbrOnAllCores();

    //
    // Disabling LBR
    //
    g_LastBranchRecordEnabled = FALSE;

    //
    // Set successful status
    //
    if (HyperTraceOperationRequest != NULL)
    {
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    }

    return TRUE;
}

/**
 * @brief Show LBR tracing for HyperTrace
 *
 * @param HyperTraceOperationRequest
 * @param ApplyFromVmxRootMode
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceShowLbrTracing(HYPERTRACE_OPERATION_PACKETS * HyperTraceOperationRequest,
                         BOOLEAN                        ApplyFromVmxRootMode)
{
    UNREFERENCED_PARAMETER(ApplyFromVmxRootMode);

    LBR_IOCTL_REQUEST * CurrentRequest;

    //
    // Check if LBR is already disabled or not
    //
    if (!g_LastBranchRecordEnabled)
    {
        if (HyperTraceOperationRequest != NULL)
        {
            HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_LBR_ALREADY_DISABLED;
        }

        return FALSE;
    }

    //
    // Get the current request (for current core)
    //
    CurrentRequest = &g_LbrRequestState[KeGetCurrentProcessorNumberEx(NULL)];

    LBR_STATE * State = LbrFindLbrState(0);

    if (State)
    {
        LbrGetLbr(State, TRUE, TRUE);
    }

    LogInfo("Dumping LBR Buffer...\n");

    LbrDumpLbr(CurrentRequest, TRUE, TRUE);

    return TRUE;
}

/**
 * @brief Uninitialize the hyper trace module
 *
 * @return VOID
 */
VOID
HyperTraceUninit()
{
    //
    // Disable LBR tracing if it is still enabled
    //
    HyperTraceDisableLbrTracing(NULL, FALSE);

    //
    // Set callbacks to not initialized
    //
    g_HyperTraceCallbacksInitialized = FALSE;

    //
    // UnAllocate the state buffer
    //
    PlatformFreeMemory(g_LbrRequestState);

    //
    // Set LBR request buffer to zero
    //
    g_LbrRequestState = NULL64_ZERO;
}

/**
 * @brief Perform actions related to HyperTrace
 *
 * @param HyperTraceOperationRequest
 * @param ApplyFromVmxRootMode
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTracePerformOperation(HYPERTRACE_OPERATION_PACKETS * HyperTraceOperationRequest,
                           BOOLEAN                        ApplyFromVmxRootMode)
{
    BOOLEAN Status = TRUE;

    //
    // Check if the hypertrace module is initialized before performing any operation
    //
    if (!g_HyperTraceCallbacksInitialized)
    {
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_HYPERTRACE_NOT_INITIALIZED;
        return FALSE;
    }

    //
    // Perform the requested operation
    //
    switch (HyperTraceOperationRequest->HyperTraceOperationType)
    {
    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_ENABLE:

        LogInfo("HyperTrace: Enabling LBR tracing...\n");

        HyperTraceEnableLbrTracing(HyperTraceOperationRequest, ApplyFromVmxRootMode);

        break;

    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_DISABLE:

        LogInfo("HyperTrace: Disabling LBR tracing...\n");

        HyperTraceDisableLbrTracing(HyperTraceOperationRequest, ApplyFromVmxRootMode);

        break;

    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_SHOW:

        LogInfo("HyperTrace: Showing LBR tracing...\n");

        HyperTraceShowLbrTracing(HyperTraceOperationRequest, ApplyFromVmxRootMode);

        break;

    default:
        Status                                   = FALSE;
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_INVALID_HYPERTRACE_OPERATION_TYPE;
        break;
    }

    return Status;
}
