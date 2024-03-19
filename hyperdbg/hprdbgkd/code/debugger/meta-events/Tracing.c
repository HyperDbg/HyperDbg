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
    DisassemblerShowOneInstructionInVmxRootMode((PVOID)VmFuncGetLastVmexitRip(DbgState->CoreId), FALSE);

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

/**
 * @brief Regular step-in | step one instruction to the debuggee
 * @param DbgState The state of the debugger on the current core
 *
 * @return VOID
 */
VOID
TracingPerformRegularStepInInstruction(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    UNREFERENCED_PARAMETER(DbgState);

    UINT64 Interruptibility;
    UINT64 InterruptibilityOld = NULL64_ZERO;

    //
    // Adjust RFLAG's trap-flag
    //
    VmFuncSetRflagTrapFlag(TRUE);

    //
    // During testing single-step, we realized that after single-stepping
    // on 'STI' instruction, after one instruction, the guest (target core)
    // starts Invalid Guest State (0x21) vm-exits, after some searches we
    // realized that KVM developer's encountered the same error; so, in order
    // to solve the problem of stepping on 'STI' and 'MOV SS', we check the
    // interruptibility state, here is a comment from KVM :
    //
    // When single stepping over STI and MOV SS, we must clear the
    // corresponding interruptibility bits in the guest state
    // Otherwise vmentry fails as it then expects bit 14 (BS)
    // in pending debug exceptions being set, but that's not
    // correct for the guest debugging case
    //
    InterruptibilityOld = VmFuncGetInterruptibilityState();

    Interruptibility = InterruptibilityOld;

    Interruptibility = VmFuncClearSteppingBits(Interruptibility);

    if ((Interruptibility != InterruptibilityOld))
    {
        VmFuncSetInterruptibilityState(Interruptibility);
    }
}
