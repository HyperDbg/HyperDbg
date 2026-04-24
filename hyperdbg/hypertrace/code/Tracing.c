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
    if (LbrStartLbr(ApplyFromVmxRootMode, ApplyByVmcall))
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

        LogInfo("Dumping LBR Buffer...\n");

        LbrStopLbr(ApplyFromVmxRootMode, ApplyByVmcall);
        LbrDumpLbr(); // This will print the collected LBR branches to the log
    }
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
    return LbrStartLbr(ApplyFromVmxRootMode, ApplyByVmcall);
}

/**
 * @brief Stop LBR tracing for HyperTrace
 * @param ApplyFromVmxRootMode
 * @param ApplyByVmcall
 *
 * @return VOID
 */
VOID
HyperTraceStopLbr(BOOLEAN ApplyFromVmxRootMode, BOOLEAN ApplyByVmcall)
{
    LbrStopLbr(ApplyFromVmxRootMode, ApplyByVmcall);
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
    UINT32 ProcessorsCount = 0;

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
    // Query the number of processors in the system to initialize the global LBR state list accordingly
    //
    ProcessorsCount = KeQueryActiveProcessorCount(0);

    //
    // Initialize the global LBR state list to hold LBR states for each core
    //
    g_LbrStateList = (LBR_STACK_ENTRY *)xmalloc(sizeof(LBR_STACK_ENTRY) * ProcessorsCount);

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
HyperTraceEnableLbr(HYPERTRACE_OPERATION_PACKETS * HyperTraceOperationRequest,
                    BOOLEAN                        ApplyFromVmxRootMode)
{
    UNREFERENCED_PARAMETER(ApplyFromVmxRootMode);

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
HyperTraceDisableLbr(HYPERTRACE_OPERATION_PACKETS * HyperTraceOperationRequest,
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
 * @brief Save LBR tracing for HyperTrace
 *
 * @param HyperTraceOperationRequest
 * @param ApplyFromVmxRootMode
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceSaveLbr(HYPERTRACE_OPERATION_PACKETS * HyperTraceOperationRequest,
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

    LogInfo("Saving LBR Buffer...\n");

    //
    // Save the LBR state
    //
    LbrSaveLbr();

    //
    // The operation was successful
    //
    if (HyperTraceOperationRequest != NULL)
    {
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    }

    return TRUE;
}

/**
 * @brief Dump LBR tracing for HyperTrace
 *
 * @param HyperTraceOperationRequest
 * @param ApplyFromVmxRootMode
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceDumpLbr(HYPERTRACE_OPERATION_PACKETS * HyperTraceOperationRequest,
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

    LogInfo("Dumping LBR Buffer...\n");

    //
    // This will print the collected LBR branches to the log
    //
    LbrDumpLbr();

    //
    // The operation was successful
    //
    if (HyperTraceOperationRequest != NULL)
    {
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    }

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
    HyperTraceDisableLbr(NULL, FALSE);

    //
    // Set callbacks to not initialized
    //
    g_HyperTraceCallbacksInitialized = FALSE;

    //
    // Unallocate the global LBR state list if it is allocated
    //
    if (g_LbrStateList != NULL)
    {
        xfree(g_LbrStateList);
        g_LbrStateList = NULL;
    }
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

        HyperTraceEnableLbr(HyperTraceOperationRequest, ApplyFromVmxRootMode);

        break;

    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_DISABLE:

        LogInfo("HyperTrace: Disabling LBR tracing...\n");

        HyperTraceDisableLbr(HyperTraceOperationRequest, ApplyFromVmxRootMode);

        break;

    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_SAVE:

        LogInfo("HyperTrace: Saving LBR tracing...\n");

        HyperTraceSaveLbr(HyperTraceOperationRequest, ApplyFromVmxRootMode);

        break;

    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_DUMP:

        LogInfo("HyperTrace: Showing LBR tracing...\n");

        HyperTraceDumpLbr(HyperTraceOperationRequest, ApplyFromVmxRootMode);

        break;

    default:
        Status                                   = FALSE;
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_INVALID_HYPERTRACE_OPERATION_TYPE;
        break;
    }

    return Status;
}
