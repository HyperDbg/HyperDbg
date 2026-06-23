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
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
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
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
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
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
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
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
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

/**
 * @brief Broadcast enabling PT
 *
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * @return BOOLEAN
 */
BOOLEAN
DpcRoutineEnablePt(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Enable PT on the current core. PT in the current implementation is
    // controlled via direct MSR writes from kernel context; if PT is later
    // wired into VMCS save/load controls, the corresponding hypervisor
    // helpers should be invoked here similar to DpcRoutineEnableLbr.
    //
    PtStart();

    // ------------------------------------------------------------------------------
    // Synchronize the end of this routine with the caller
    //
    PlatformBroadcastSynchronizeEndOfRoutine(SystemArgument1, SystemArgument2);

    return TRUE;
}

/**
 * @brief Broadcast disabling PT
 */
BOOLEAN
DpcRoutineDisablePt(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    PtStop();

    // ------------------------------------------------------------------------------
    // Synchronize the end of this routine with the caller
    //
    PlatformBroadcastSynchronizeEndOfRoutine(SystemArgument1, SystemArgument2);

    return TRUE;
}

/**
 * @brief Broadcast pausing PT
 */
BOOLEAN
DpcRoutinePausePt(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    PtPause();

    // ------------------------------------------------------------------------------
    // Synchronize the end of this routine with the caller
    //
    PlatformBroadcastSynchronizeEndOfRoutine(SystemArgument1, SystemArgument2);

    return TRUE;
}

/**
 * @brief Broadcast resuming PT
 */
BOOLEAN
DpcRoutineResumePt(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    PtResume();

    // ------------------------------------------------------------------------------
    // Synchronize the end of this routine with the caller
    //
    PlatformBroadcastSynchronizeEndOfRoutine(SystemArgument1, SystemArgument2);

    return TRUE;
}

/**
 * @brief Broadcast snapshotting per-CPU PT output position.
 *
 *        DeferredContext is a UINT64 array (one slot per active CPU);
 *        each per-core DPC writes its own core's byte count and never
 *        touches another slot, so no synchronisation is required.
 */
BOOLEAN
DpcRoutineSizePt(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UINT64 * Sizes = (UINT64 *)DeferredContext;
    UINT32   Core  = KeGetCurrentProcessorNumberEx(NULL);

    UNREFERENCED_PARAMETER(Dpc);

    if (Sizes != NULL && Core < PT_MAX_CPUS_FOR_MMAP)
        Sizes[Core] = PtSize();

    // ------------------------------------------------------------------------------
    // Synchronize the end of this routine with the caller
    //
    PlatformBroadcastSynchronizeEndOfRoutine(SystemArgument1, SystemArgument2);

    return TRUE;
}

/**
 * @brief Broadcast dumping PT state
 */
BOOLEAN
DpcRoutineDumpPt(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    PtDump();

    // ------------------------------------------------------------------------------
    // Synchronize the end of this routine with the caller
    //
    PlatformBroadcastSynchronizeEndOfRoutine(SystemArgument1, SystemArgument2);

    return TRUE;
}

/**
 * @brief Broadcast flushing PT state
 */
BOOLEAN
DpcRoutineFlushPt(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    PtFlush();

    // ------------------------------------------------------------------------------
    // Synchronize the end of this routine with the caller
    //
    PlatformBroadcastSynchronizeEndOfRoutine(SystemArgument1, SystemArgument2);

    return TRUE;
}

/**
 * @brief Broadcast applying a PT filter to all cores.
 *
 *        DeferredContext carries the PT_FILTER_OPTIONS * supplied by the
 *        broadcaster; PtFilter writes the user-tunable fields into the
 *        current CPU's per-CPU PT_TRACE_CONFIG and reprograms PT MSRs.
 */
BOOLEAN
DpcRoutineFilterPt(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);

    PtFilter((const PT_FILTER_OPTIONS *)DeferredContext);

    // ------------------------------------------------------------------------------
    // Synchronize the end of this routine with the caller
    //
    PlatformBroadcastSynchronizeEndOfRoutine(SystemArgument1, SystemArgument2);

    return TRUE;
}
