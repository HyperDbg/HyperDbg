/**
 * @file DpcRoutines.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief DPC routines
 * @details
 * @version 0.19
 * @date 2026-04-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Broadcast enabling LBR
 *
 * @param Dpc The DPC object (unused)
 * @param DeferredContext Context data passed to the DPC (unused)
 * @param SystemArgument1 First system argument used for DPC synchronization
 * @param SystemArgument2 Second system argument used for DPC synchronization
 * @return BOOLEAN
 */
BOOLEAN
DpcRoutineEnableLbr(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Check if the initialization is being done for hypervisor environment or not
    // If it is, then we need to perform some additional steps to enable LBR in VMX
    //
    if (g_RunningOnHypervisorEnvironment)
    {
        if (g_ArchBasedLastBranchRecord)
        {
            //
            // Perform VMX-root mode specific operations to load and clear guest
            // IA32_LBR_CTL MSR (VMCS_GUEST_LBR_CTL) for LBR
            //
            g_Callbacks.VmFuncSetLoadGuestIa32LbrCtlVmcallOnTargetCore(TRUE);
            g_Callbacks.VmFuncSetClearGuestIa32LbrCtlVmcallOnTargetCore(TRUE);
        }
        else
        {
            //
            // Perform VMX-root mode specific operations to enable load and save
            // VM-exit and VM-entry controls for IA32_DEBUGCTL for LBR
            //
            g_Callbacks.VmFuncSetSaveDebugControlsVmcallOnTargetCore(TRUE);
            g_Callbacks.VmFuncSetLoadDebugControlsVmcallOnTargetCore(TRUE);
        }
    }

    //
    // Enable LBR on all cores from VMX-root mode by VMCALL
    // By default, all filter options are disabled, which means all branch types will be captured
    //
    LbrStart(LBR_SELECT_WITHOUT_FILTER);

    // ------------------------------------------------------------------------------
    // Synchronize the end of this routine with the caller
    //
    PlatformBroadcastSynchronizeEndOfRoutine(SystemArgument1, SystemArgument2);

    return TRUE;
}

/**
 * @brief Broadcast disabling LBR
 *
 * @param Dpc The DPC object (unused)
 * @param DeferredContext Context data passed to the DPC (unused)
 * @param SystemArgument1 First system argument used for DPC synchronization
 * @param SystemArgument2 Second system argument used for DPC synchronization
 * @return BOOLEAN
 */
BOOLEAN
DpcRoutineDisableLbr(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Disable LBR on all cores from VMX-root mode by VMCALL
    //
    LbrStop();

    //
    // Check if the initialization is being done for hypervisor environment or not
    // If it is, then we need to perform some additional steps to enable LBR in VMX
    //
    if (g_RunningOnHypervisorEnvironment)
    {
        if (g_ArchBasedLastBranchRecord)
        {
            //
            // Perform VMX-root mode specific operations to disable load and clear guest
            // IA32_LBR_CTL MSR (VMCS_GUEST_LBR_CTL) for LBR
            //
            g_Callbacks.VmFuncSetLoadGuestIa32LbrCtlVmcallOnTargetCore(FALSE);
            g_Callbacks.VmFuncSetClearGuestIa32LbrCtlVmcallOnTargetCore(FALSE);
        }
        else
        {
            //
            // Perform VMX-root mode specific operations to disable load and save
            // VM-exit and VM-entry controls for IA32_DEBUGCTL for LBR
            //
            g_Callbacks.VmFuncSetSaveDebugControlsVmcallOnTargetCore(FALSE);
            g_Callbacks.VmFuncSetLoadDebugControlsVmcallOnTargetCore(FALSE);
        }
    }

    // ------------------------------------------------------------------------------
    // Synchronize the end of this routine with the caller
    //
    PlatformBroadcastSynchronizeEndOfRoutine(SystemArgument1, SystemArgument2);

    return TRUE;
}

/**
 * @brief Broadcast flushing LBR
 *
 * @param Dpc The DPC object (unused)
 * @param DeferredContext Context data passed to the DPC (unused)
 * @param SystemArgument1 First system argument used for DPC synchronization
 * @param SystemArgument2 Second system argument used for DPC synchronization
 * @return BOOLEAN
 */
BOOLEAN
DpcRoutineFlushLbr(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Flush LBR on all cores
    //
    LbrFlush();

    // ------------------------------------------------------------------------------
    // Synchronize the end of this routine with the caller
    //
    PlatformBroadcastSynchronizeEndOfRoutine(SystemArgument1, SystemArgument2);

    return TRUE;
}

/**
 * @brief Broadcast updating LBR filter options
 *
 * @param Dpc The DPC object (unused)
 * @param DeferredContext Context data passed to the DPC, used as the LBR filter options bitmask
 * @param SystemArgument1 First system argument used for DPC synchronization
 * @param SystemArgument2 Second system argument used for DPC synchronization
 * @return BOOLEAN
 */
BOOLEAN
DpcRoutineFilterLbrOptions(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);

    //
    // Flush LBR on all cores
    //
    LbrFilter((UINT64)DeferredContext);

    // ------------------------------------------------------------------------------
    // Synchronize the end of this routine with the caller
    //
    PlatformBroadcastSynchronizeEndOfRoutine(SystemArgument1, SystemArgument2);

    return TRUE;
}
