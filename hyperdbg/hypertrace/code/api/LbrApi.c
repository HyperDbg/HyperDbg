/**
 * @file LbrApi.c
 * @author Hari Mishal (harimishal6@gmail.com)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Tracing routines for HyperTrace module (Intel Last Branch Record)
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
 * @return BOOLEAN
 */
VOID
HyperTraceLbrExamplePerformTrace()
{
    if (LbrStart(LBR_SELECT))
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

        LbrStop();
        LbrDump(); // This will print the collected LBR branches to the log
    }
}

/**
 * @brief Query the state of LBR save and load VM exit and entry controls
 *
 * @param CoreId
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceLbrQueryStateOfLbrSaveAndLoadVmExitAndEntryControls(UINT32 CoreId)
{
    UNREFERENCED_PARAMETER(CoreId); // Right now there is no core specifc controls for LBR

    return g_LastBranchRecordEnabled;
}

/**
 * @brief Enable LBR tracing for HyperTrace
 *
 * @param HyperTraceOperationRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceLbrEnable(HYPERTRACE_LBR_OPERATION_PACKETS * HyperTraceOperationRequest)
{
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
    if (g_RunningOnHypervisorEnvironment && !g_Callbacks.VmFuncCheckCpuSupportForSaveAndLoadDebugControls())
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
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceLbrDisable(HYPERTRACE_LBR_OPERATION_PACKETS * HyperTraceOperationRequest)
{
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
    // Disabling LBR
    //
    g_LastBranchRecordEnabled = FALSE;

    //
    // Broadcast disabling LBR on all cores
    //
    BroadcastDisableLbrOnAllCores();

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
 * @brief Flush LBR tracing for HyperTrace
 *
 * @param HyperTraceOperationRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceLbrFlush(HYPERTRACE_LBR_OPERATION_PACKETS * HyperTraceOperationRequest)
{
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
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceLbrSave(HYPERTRACE_LBR_OPERATION_PACKETS * HyperTraceOperationRequest)
{
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
    LbrSave();

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
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceLbrDump(HYPERTRACE_LBR_OPERATION_PACKETS * HyperTraceOperationRequest)
{
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
    LbrDump();

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
 * @brief Update LBR filter options for HyperTrace
 *
 * @param HyperTraceOperationRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceLbrUpdateFilterOptions(HYPERTRACE_LBR_OPERATION_PACKETS * HyperTraceOperationRequest)
{
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
    // Update the LBR filter options based on the request
    //
    BroadcastFilterLbrOptionsOnAllCores((UINT64)HyperTraceOperationRequest->LbrFilterOptions);

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
 * @brief Perform actions related to HyperTrace LBR
 *
 * @param LbrOperationRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceLbrPerformOperation(HYPERTRACE_LBR_OPERATION_PACKETS * LbrOperationRequest)
{
    BOOLEAN Status = TRUE;

    //
    // Check if the hypertrace module is initialized before performing any operation
    //
    if (!g_HyperTraceCallbacksInitialized)
    {
        LbrOperationRequest->KernelStatus = DEBUGGER_ERROR_HYPERTRACE_NOT_INITIALIZED;
        return FALSE;
    }

    //
    // Perform the requested operation
    //
    switch (LbrOperationRequest->LbrOperationType)
    {
    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_ENABLE:

        LogInfo("HyperTrace: Enabling LBR tracing...\n");

        HyperTraceLbrEnable(LbrOperationRequest);

        break;

    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_DISABLE:

        LogInfo("HyperTrace: Disabling LBR tracing...\n");

        HyperTraceLbrDisable(LbrOperationRequest);

        break;

    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_SAVE:

        LogInfo("HyperTrace: Saving LBR tracing...\n");

        HyperTraceLbrSave(LbrOperationRequest);

        break;

    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_DUMP:

        LogInfo("HyperTrace: Showing LBR tracing...\n");

        HyperTraceLbrDump(LbrOperationRequest);

        break;

    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_FLUSH:

        LogInfo("HyperTrace: Flushing LBR tracing...\n");

        HyperTraceLbrFlush(LbrOperationRequest);

        break;

    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_FILTER:

        LogInfo("HyperTrace: Updating LBR filter options...\n");

        HyperTraceLbrUpdateFilterOptions(LbrOperationRequest);

        break;

    default:
        Status                            = FALSE;
        LbrOperationRequest->KernelStatus = DEBUGGER_ERROR_INVALID_HYPERTRACE_OPERATION_TYPE;
        break;
    }

    return Status;
}
