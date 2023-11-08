/**
 * @file Tracing.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of tracing functions
 * @details
 *
 * @version 0.7
 * @date 2023-11-03
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Perform tracing of instructions (instrumentation step-in)
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
TracingPerformInstrumentationStepIn(VIRTUAL_MACHINE_STATE * VCpu)
{
    DisassemblerShowOneInstructionInVmxRootMode(VCpu->LastVmexitRip, FALSE);

    VCpu->TracingMode = TRUE;
    HvEnableMtfAndChangeExternalInterruptState(VCpu);
}

/**
 * @brief Callback for handling VM-exits for MTF in the case of tracing instructions
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
TracingHandleMtfVmexit(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Dispatch and trigger the related events
    //
    DispatchEventInstrumentationTrace(VCpu);
}

/**
 * @brief Restore the system state in the case of tracing instructions
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
TracingRestoreSystemState(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Indicate that we're no longer looking for the tracing
    //
    VCpu->TracingMode = FALSE;

    //
    // Check for reenabling external interrupts
    //
    HvEnableAndCheckForPreviousExternalInterrupts(VCpu);
}

/**
 * @brief Change for continuing the stepping state in the case of tracing
 * instructions
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
TracingCheckForContinuingSteps(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // As it's called from a step itself, we need to double
    // check whether the MTF should be ignored or not
    //
    if (VCpu->TracingMode)
    {
        VCpu->IgnoreMtfUnset = TRUE;
    }
}
