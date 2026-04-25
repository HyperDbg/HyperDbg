/**
 * @file Pt.c
 * @author
 * @brief Processor Trace (PT) tracing implementation for HyperTrace module
 * @details
 * @version 0.19
 * @date 2026-04-25
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#include "pch.h"

/**
 * @brief Check if the current CPU supports PT
 *
 * @return BOOLEAN
 */
BOOLEAN
PtCheck()
{
    return FALSE;
}

/**
 * @brief Enable PT tracing for HyperTrace
 *
 * @param HyperTraceOperationRequest
 * @param ApplyFromVmxRootMode
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTracePtEnable(HYPERTRACE_OPERATION_PACKETS * HyperTraceOperationRequest,
                   BOOLEAN                        ApplyFromVmxRootMode)
{
    UNREFERENCED_PARAMETER(ApplyFromVmxRootMode);

    //
    // Check if PT is already enabled or not
    //
    if (g_ProcessorTraceEnabled)
    {
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_PT_ALREADY_ENABLED;
        return FALSE;
    }

    //
    // Check PT support on CPU
    //
    if (!PtCheck())
    {
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_PT_NOT_SUPPORTED;
        return FALSE;
    }

    //
    // Set the flag to indicate that PT tracing is enabled
    //
    g_ProcessorTraceEnabled = TRUE;

    //
    // Set successful status
    //
    HyperTraceOperationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    return TRUE;
}

/**
 * @brief Disable PT tracing for HyperTrace
 *
 * @param HyperTraceOperationRequest
 * @param ApplyFromVmxRootMode
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTracePtDisable(HYPERTRACE_OPERATION_PACKETS * HyperTraceOperationRequest,
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
            HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_PT_ALREADY_DISABLED;
        }

        return FALSE;
    }

    //
    // Disabling PT
    //
    g_ProcessorTraceEnabled = FALSE;

    //
    // Set successful status
    //
    if (HyperTraceOperationRequest != NULL)
    {
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    }

    return TRUE;
}
