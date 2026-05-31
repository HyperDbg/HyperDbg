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
 * @return VOID
 */
VOID
HyperTraceLbrExamplePerformTrace()
{
    if (LbrStart(LBR_SELECT_WITHOUT_FILTER))
    {
        for (volatile INT i = 0; i < 50; i++)
        {
            if (i % 2)
            {
                INT A = i * 2;
                A += 5;
            }
            else
            {
                CpuNop();
                CpuNop();
            }
        }

        LogInfo("Dumping LBR Buffer...\n");

        LbrStop();
        LbrPrint(); // This will print the collected LBR branches to the log
    }
}

/**
 * @brief Query the state of LBR save and load VM exit and entry controls
 *
 * @param CoreId The index of the processor core to query
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
 * @brief Set the kernel status in the HyperTrace LBR operation request structure
 *
 * @param HyperTraceLbrOperationRequest Pointer to the HyperTrace LBR operation request packet
 * @param Status The kernel status code to write into the request
 *
 * @return VOID
 */
VOID
HyperTraceLbrSetKernelStatus(
    HYPERTRACE_LBR_OPERATION_PACKETS * HyperTraceLbrOperationRequest,
    UINT32                             Status)
{
    if (HyperTraceLbrOperationRequest != NULL)
    {
        HyperTraceLbrOperationRequest->KernelStatus = Status;
    }
}

/**
 * @brief Set the kernel status in the HyperTrace LBR dump operation request structure
 *
 * @param HyperTraceLbrDumpOperationRequest Pointer to the HyperTrace LBR dump operation request packet
 * @param Status The kernel status code to write into the request
 *
 * @return VOID
 */
VOID
HyperTraceLbrDumpSetKernelStatus(
    HYPERTRACE_LBR_DUMP_PACKETS * HyperTraceLbrDumpOperationRequest,
    UINT32                        Status)
{
    if (HyperTraceLbrDumpOperationRequest != NULL)
    {
        HyperTraceLbrDumpOperationRequest->KernelStatus = Status;
    }
}

/**
 * @brief Check if LBR is supported and enabled on the current core
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceLbrCheck()
{
    //
    // Only check LBR once it is already initialized
    //
    if (!g_LastBranchRecordEnabled)
    {
        return FALSE;
    }

    return LbrCheck();
}

/**
 * @brief Restore (re-enable) LBR collection on the current core with the specified filter options
 * @param FilterOptions A bitmask of filter options to apply to the LBR branches
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceLbrRestoreByFilter(UINT64 FilterOptions)
{
    //
    // Only restore (re-enable) LBR once it is already initialized
    //
    if (!g_LastBranchRecordEnabled)
    {
        return FALSE;
    }

    return LbrStart(FilterOptions);
}

/**
 * @brief Restore (re-enable) LBR collection on the current core with previous filter options
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceLbrRestore()
{
    return HyperTraceLbrRestoreByFilter(g_LbrFilterOptions);
}

/**
 * @brief Check if LBR is supported on the current CPU and get its capacity
 *
 * @param Capacity Pointer to a variable to receive the LBR capacity (number of entries)
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceLbrIsSupported(UINT32 * Capacity)
{
    //
    // Check for ARCHITECTURAL LBR support first, if not supported then check for LEGACY LBR support
    //
    if (!LbrCheckAndReadArchitecturalLbrDetails())
    {
        //
        // If the CPU does not support architectural LBR, we can check for legacy LBR support as a fallback
        //
        if (!LbrCheckAndReadLegacyLbrDetails())
        {
            return FALSE;
        }
    }

    //
    // Set capacity when the pointer is valid
    //
    if (Capacity != NULL)
    {
        *Capacity = (UINT32)g_LbrCapacity;
    }

    return TRUE;
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
    // We allow re-enabling LBR even if it is already enabled to support scenarios where
    // the LBR is deactivated as a result of a #DB and wants to re-enable it again
    /*
    if (g_LastBranchRecordEnabled)
    {
        HyperTraceLbrSetKernelStatus(HyperTraceOperationRequest, DEBUGGER_ERROR_LBR_ALREADY_ENABLED);
        return FALSE;
    }
    */

    //
    // Check for ARCHITECTURAL LBR support first, if not supported then check for LEGACY LBR support
    //
    if (!LbrCheckAndReadArchitecturalLbrDetails())
    {
        //
        // If the CPU does not support architectural LBR, we can check for legacy LBR support as a fallback
        //
        if (!LbrCheckAndReadLegacyLbrDetails())
        {
            HyperTraceLbrSetKernelStatus(HyperTraceOperationRequest, DEBUGGER_ERROR_LBR_NOT_SUPPORTED);
            return FALSE;
        }
    }

    //
    // Check VMCS support for LBR if the initialization is being done for hypervisor environment
    //
    if (g_RunningOnHypervisorEnvironment)
    {
        if ((g_ArchBasedLastBranchRecord && !g_Callbacks.VmFuncCheckCpuSupportForLoadAndClearGuestIa32LbrCtlControls()) ||
            (!g_ArchBasedLastBranchRecord && !g_Callbacks.VmFuncCheckCpuSupportForSaveAndLoadDebugControls()))
        {
            HyperTraceLbrSetKernelStatus(HyperTraceOperationRequest, DEBUGGER_ERROR_LBR_NOT_SUPPORTED_ON_VMCS);
            return FALSE;
        }
    }

    //
    // Broadcast enabling LBR on all cores
    //
    BroadcastEnableLbrOnAllCores();

    //
    // Check if LBR is enabled or not (for example in VMs, LBR flags are usually masked)
    //
    if (!LbrCheck())
    {
        HyperTraceLbrSetKernelStatus(HyperTraceOperationRequest, DEBUGGER_ERROR_LBR_NOT_SUPPORTED);
        return FALSE;
    }

    //
    // Set the flag to indicate that LBR tracing is enabled
    //
    g_LastBranchRecordEnabled = TRUE;

    //
    // Set successful status
    //
    HyperTraceLbrSetKernelStatus(HyperTraceOperationRequest, DEBUGGER_OPERATION_WAS_SUCCESSFUL);

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
        HyperTraceLbrSetKernelStatus(HyperTraceOperationRequest, DEBUGGER_ERROR_LBR_ALREADY_DISABLED);
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
    HyperTraceLbrSetKernelStatus(HyperTraceOperationRequest, DEBUGGER_OPERATION_WAS_SUCCESSFUL);

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
        HyperTraceLbrSetKernelStatus(HyperTraceOperationRequest, DEBUGGER_ERROR_LBR_ALREADY_DISABLED);
        return FALSE;
    }

    //
    // Disabling LBR
    //
    g_LastBranchRecordEnabled = FALSE;

    //
    // Broadcast flushing LBR on all cores
    //
    BroadcastFlushLbrOnAllCores();

    //
    // Set successful status
    //
    HyperTraceLbrSetKernelStatus(HyperTraceOperationRequest, DEBUGGER_OPERATION_WAS_SUCCESSFUL);

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
        HyperTraceLbrSetKernelStatus(HyperTraceOperationRequest, DEBUGGER_ERROR_LBR_ALREADY_DISABLED);
        return FALSE;
    }

    // LogInfo("Saving LBR Buffer...\n");

    //
    // Save the LBR state
    //
    LbrSave();

    //
    // The operation was successful
    //
    HyperTraceLbrSetKernelStatus(HyperTraceOperationRequest, DEBUGGER_OPERATION_WAS_SUCCESSFUL);

    return TRUE;
}

/**
 * @brief Print LBR tracing for HyperTrace
 *
 * @param HyperTraceOperationRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceLbrPrint(HYPERTRACE_LBR_OPERATION_PACKETS * HyperTraceOperationRequest)
{
    //
    // Check if LBR is already disabled or not
    //
    if (!g_LastBranchRecordEnabled)
    {
        HyperTraceLbrSetKernelStatus(HyperTraceOperationRequest, DEBUGGER_ERROR_LBR_ALREADY_DISABLED);
        return FALSE;
    }

    // LogInfo("Dumping LBR Buffer...\n");

    //
    // Save the LBR state
    //
    LbrSave();

    //
    // This will print the collected LBR branches to the log
    //
    LbrPrint();

    //
    // The operation was successful
    //
    HyperTraceLbrSetKernelStatus(HyperTraceOperationRequest, DEBUGGER_OPERATION_WAS_SUCCESSFUL);

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
        HyperTraceLbrSetKernelStatus(HyperTraceOperationRequest, DEBUGGER_ERROR_LBR_ALREADY_DISABLED);
        return FALSE;
    }

    //
    // Update the LBR filter options based on the request
    //
    BroadcastFilterLbrOptionsOnAllCores((UINT64)HyperTraceOperationRequest->LbrFilterOptions);

    //
    // The operation was successful
    //
    HyperTraceLbrSetKernelStatus(HyperTraceOperationRequest, DEBUGGER_OPERATION_WAS_SUCCESSFUL);

    return TRUE;
}

/**
 * @brief Perform actions related to HyperTrace LBR dumping
 *
 * @param LbrDumpRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceLbrPerformDump(HYPERTRACE_LBR_DUMP_PACKETS * LbrDumpRequest)
{
    ULONG ProcessorsCount;

    //
    // Check if LBR is enabled or not before dumping
    //
    if (!g_LastBranchRecordEnabled)
    {
        HyperTraceLbrDumpSetKernelStatus(LbrDumpRequest, DEBUGGER_ERROR_LBR_ALREADY_DISABLED);
        return FALSE;
    }

    //
    // Get the number of processors in the system to validate the requested core id for dumping
    //
    ProcessorsCount = KeQueryActiveProcessorCount(0);

    //
    // Check if core id is valid
    //
    if (LbrDumpRequest->CoreId >= ProcessorsCount)
    {
        HyperTraceLbrDumpSetKernelStatus(LbrDumpRequest, DEBUGGER_ERROR_INVALID_CORE_ID);
        return FALSE;
    }

    //
    // Check if next core is valid in the case of dumping all cores
    //
    if (LbrDumpRequest->CoreId == ProcessorsCount - 1)
    {
        LbrDumpRequest->NextCoreIsValid = FALSE;
    }
    else
    {
        LbrDumpRequest->NextCoreIsValid = TRUE;
    }

    //
    // Set ARCH or LEGACY LBR flag in the dump request structure based on the type of LBR supported by the CPU
    //
    LbrDumpRequest->ArchBasedLBR = g_ArchBasedLastBranchRecord;

    //
    // Set the current LBR capacity in the dump request structure based on the type of LBR supported by the CPU
    //
    LbrDumpRequest->CurrentLbrCapacity = (UINT8)g_LbrCapacity;

    //
    // Copy the LBR stack entries of the requested core into the dump request structure to be sent back to usermode
    //
    RtlCopyMemory(&LbrDumpRequest->LbrStack, &g_LbrStateList[LbrDumpRequest->CoreId], sizeof(LBR_STACK_ENTRY));

    //
    // Set successful status
    //
    HyperTraceLbrDumpSetKernelStatus(LbrDumpRequest, DEBUGGER_OPERATION_WAS_SUCCESSFUL);

    return TRUE;
}

/**
 * @brief Perform actions related to HyperTrace LBR operations
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
        HyperTraceLbrSetKernelStatus(LbrOperationRequest, DEBUGGER_ERROR_HYPERTRACE_NOT_INITIALIZED);
        return FALSE;
    }

    //
    // Perform the requested operation
    //
    switch (LbrOperationRequest->LbrOperationType)
    {
    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_ENABLE:

        // LogInfo("HyperTrace: Enabling LBR tracing...\n");

        HyperTraceLbrEnable(LbrOperationRequest);

        break;

    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_DISABLE:

        // LogInfo("HyperTrace: Disabling LBR tracing...\n");

        HyperTraceLbrDisable(LbrOperationRequest);

        break;

    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_FLUSH:

        // LogInfo("HyperTrace: Flushing LBR tracing...\n");

        HyperTraceLbrFlush(LbrOperationRequest);

        break;

    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_FILTER:

        // LogInfo("HyperTrace: Updating LBR filter options...\n");

        HyperTraceLbrUpdateFilterOptions(LbrOperationRequest);

        break;

    default:

        Status = FALSE;
        HyperTraceLbrSetKernelStatus(LbrOperationRequest, DEBUGGER_ERROR_INVALID_HYPERTRACE_OPERATION_TYPE);

        break;
    }

    return Status;
}
