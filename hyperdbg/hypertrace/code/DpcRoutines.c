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
    LBR_IOCTL_REQUEST * CurrentRequest;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Get the current request (for current core)
    //
    CurrentRequest = &g_LbrRequestState[KeGetCurrentProcessorNumberEx(NULL)];

    //
    // Enable LBR on all cores from VMX-root mode by VMCALL
    //

    // LbrStartLbr(CurrentRequest, TRUE, TRUE);
    HyperTraceExamplePerformLbrTrace(TRUE, TRUE);

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
    LBR_IOCTL_REQUEST * CurrentRequest;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);

    //
    // Get the current request (for current core)
    //
    CurrentRequest = &g_LbrRequestState[KeGetCurrentProcessorNumberEx(NULL)];

    //
    // Disable LBR on all cores from VMX-root mode by VMCALL
    //
    LbrStopLbr(CurrentRequest, TRUE, TRUE);

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
