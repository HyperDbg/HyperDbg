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
 * @param DbgState The state of the debugger on the current core
 *
 * @return VOID
 */
VOID
TracingPerformInstrumentationStepIn(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    DisassemblerShowOneInstructionInVmxRootMode(VmFuncGetLastVmexitRip(DbgState->CoreId), FALSE);

    DbgState->TracingMode = TRUE;

    //
    // Register break on MTF
    //
    VmFuncRegisterMtfBreak(DbgState->CoreId);

    VmFuncEnableMtfAndChangeExternalInterruptState(DbgState->CoreId);
}

/**
 * @brief Callback for handling VM-exits for MTF in the case of tracing instructions
 * @param DbgState The state of the debugger on the current core
 *
 * @return VOID
 */
VOID
TracingHandleMtf(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    //
    // Handle system-state after handling single instrumentation step-in
    //
    TracingRestoreSystemState(DbgState);

    //
    // Dispatch and trigger the related events
    //
    MetaDispatchEventInstrumentationTrace(DbgState);

    //
    // Check whether the steps needs to be continued or not
    //
    TracingCheckForContinuingSteps(DbgState);
}

/**
 * @brief Restore the system state in the case of tracing instructions
 * @param DbgState The state of the debugger on the current core
 *
 * @return VOID
 */
VOID
TracingRestoreSystemState(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    //
    // Indicate that we're no longer looking for the tracing
    //
    DbgState->TracingMode = FALSE;

    //
    // Unregister break on MTF
    //
    VmFuncUnRegisterMtfBreak(DbgState->CoreId);

    //
    // Check for reenabling external interrupts
    //
    VmFuncEnableAndCheckForPreviousExternalInterrupts(DbgState->CoreId);
}

/**
 * @brief Change for continuing the stepping state in the case of tracing
 * instructions
 * @param DbgState The state of the debugger on the current core
 *
 * @return VOID
 */
VOID
TracingCheckForContinuingSteps(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    //
    // As it's called from a step itself, we need to double
    // check whether the MTF should be ignored or not
    //
    if (DbgState->TracingMode)
    {
        VmFuncChangeMtfUnsettingState(DbgState->CoreId, TRUE);
    }
}
