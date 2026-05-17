/**
 * @file DpcRoutines.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief DPC routines
 *
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
        //
        // Perform VMX-root mode specific operations to enable load and save
        // VM-exit and VM-entry controls for IA32_DEBUGCTL for LBR
        //
        g_Callbacks.VmFuncSetSaveDebugControlsVmcallOnTargetCore(TRUE);
        g_Callbacks.VmFuncSetLoadDebugControlsVmcallOnTargetCore(TRUE);
    }

    //
    // Enable LBR on all cores from VMX-root mode by VMCALL
    // By default, all filter options are disabled, which means all branch types will be captured
    //
    LbrStart(LBR_SELECT);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);

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
        //
        // Perform VMX-root mode specific operations to disable load and save
        // VM-exit and VM-entry controls for IA32_DEBUGCTL for LBR
        //
        g_Callbacks.VmFuncSetSaveDebugControlsVmcallOnTargetCore(FALSE);
        g_Callbacks.VmFuncSetLoadDebugControlsVmcallOnTargetCore(FALSE);
    }

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);

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

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);

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

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);

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

    KeSignalCallDpcSynchronize(SystemArgument2);
    KeSignalCallDpcDone(SystemArgument1);
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

    KeSignalCallDpcSynchronize(SystemArgument2);
    KeSignalCallDpcDone(SystemArgument1);
    return TRUE;
}

/**
 * @brief Broadcast saving PT state
 */
BOOLEAN
DpcRoutineSavePt(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    PtSave();

    KeSignalCallDpcSynchronize(SystemArgument2);
    KeSignalCallDpcDone(SystemArgument1);
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

    KeSignalCallDpcSynchronize(SystemArgument2);
    KeSignalCallDpcDone(SystemArgument1);
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

    KeSignalCallDpcSynchronize(SystemArgument2);
    KeSignalCallDpcDone(SystemArgument1);
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

    KeSignalCallDpcSynchronize(SystemArgument2);
    KeSignalCallDpcDone(SystemArgument1);
    return TRUE;
}
