/**
 * @file PtApi.c
 * @author
 * @brief Tracing routines for HyperTrace module (Intel Processor Trace)
 * @details
 * @version 0.19
 * @date 2026-04-25
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#include "pch.h"

/**
 * @brief Example of performing PT trace
 *
 * @return BOOLEAN
 */
VOID
HyperTracePtExample()
{
}

/**
 * @brief Enable PT tracing for HyperTrace
 *
 * @param PtOperationRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTracePtEnable(HYPERTRACE_PT_OPERATION_PACKETS * PtOperationRequest)
{
    //
    // Check if PT is already enabled or not
    //
    if (g_ProcessorTraceEnabled)
    {
        PtOperationRequest->KernelStatus = DEBUGGER_ERROR_PT_ALREADY_ENABLED;
        return FALSE;
    }

    //
    // Check PT support on CPU
    //
    if (!PtCheck())
    {
        PtOperationRequest->KernelStatus = DEBUGGER_ERROR_PT_NOT_SUPPORTED;
        return FALSE;
    }

    //
    // Set the flag to indicate that PT tracing is enabled
    //
    g_ProcessorTraceEnabled = TRUE;

    //
    // Set successful status
    //
    PtOperationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    return TRUE;
}

/**
 * @brief Disable PT tracing for HyperTrace
 *
 * @param PtOperationRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTracePtDisable(HYPERTRACE_PT_OPERATION_PACKETS * PtOperationRequest)
{
    //
    // Check if LBR is already disabled or not
    //
    if (!g_LastBranchRecordEnabled)
    {
        if (PtOperationRequest != NULL)
        {
            PtOperationRequest->KernelStatus = DEBUGGER_ERROR_PT_ALREADY_DISABLED;
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
    if (PtOperationRequest != NULL)
    {
        PtOperationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    }

    return TRUE;
}

/**
 * @brief Perform actions related to HyperTrace PT
 *
 * @param PtOperationRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTracePtPerformOperation(HYPERTRACE_PT_OPERATION_PACKETS * PtOperationRequest)
{
    BOOLEAN Status = TRUE;

    //
    // Check if the hypertrace module is initialized before performing any operation
    //
    if (!g_HyperTraceCallbacksInitialized)
    {
        PtOperationRequest->KernelStatus = DEBUGGER_ERROR_HYPERTRACE_NOT_INITIALIZED;
        return FALSE;
    }

    //
    // Perform the requested operation
    //
    switch (PtOperationRequest->PtOperationType)
    {
    case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_ENABLE:

        LogInfo("HyperTrace: Enabling LBR tracing...\n");

        HyperTracePtEnable(PtOperationRequest);

        break;

    case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DISABLE:

        LogInfo("HyperTrace: Disabling PT tracing...\n");

        HyperTracePtDisable(PtOperationRequest);

        break;

    case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_SAVE:

        LogInfo("HyperTrace: Saving LBR tracing (Not implemented)\n");

        break;

    case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DUMP:

        LogInfo("HyperTrace: Showing PT tracing (Not implemented)\n");

        break;

    default:
        Status                           = FALSE;
        PtOperationRequest->KernelStatus = DEBUGGER_ERROR_INVALID_HYPERTRACE_OPERATION_TYPE;
        break;
    }

    return Status;
}
