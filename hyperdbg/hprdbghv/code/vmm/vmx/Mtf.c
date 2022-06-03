/**
 * @file Mtf.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Routines relating to Monitor Trap Flag (MTF)
 * @details 
 * @version 0.1
 * @date 2021-01-27
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief Handle Monitor Trap Flag vm-exits
 *
 * @param CurrentProcessorIndex
 * @param GuestRegs
 * @return VOID
 */
VOID
MtfHandleVmexit(ULONG CurrentProcessorIndex, PGUEST_REGS GuestRegs)
{
    DEBUGGER_TRIGGERED_EVENT_DETAILS ContextAndTag = {0};
    BOOLEAN                          AvoidUnsetMtf;
    //
    // Only 16 bit is needed howerver, vmwrite might write on other bits
    // and corrupt other variables, that's why we get 64bit
    //
    UINT64                      CsSel                 = NULL;
    BOOLEAN                     IsMtfHandled          = FALSE;
    VIRTUAL_MACHINE_STATE *     CurrentVmState        = &g_GuestState[CurrentProcessorIndex];
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggingState = &CurrentVmState->DebuggingState;

    //
    // Redo the instruction
    //
    CurrentVmState->IncrementRip = FALSE;

    //
    // Explicitly say that we want to unset MTFs
    //
    CurrentVmState->IgnoreMtfUnset = FALSE;

    //
    // Check if we need to re-apply a breakpoint or not
    // We check it separately because the guest might step
    // instructions on an MTF so we want to check for the step too
    //
    if (CurrentDebuggingState->SoftwareBreakpointState != NULL)
    {
        BYTE BreakpointByte = 0xcc;

        //
        // MTF is handled
        //
        IsMtfHandled = TRUE;

        //
        // Restore previous breakpoint byte
        //
        MemoryMapperWriteMemorySafeByPhysicalAddress(
            CurrentDebuggingState->SoftwareBreakpointState->PhysAddress,
            &BreakpointByte,
            sizeof(BYTE));

        //
        // Check if we should re-enabled IF bit of RFLAGS or not
        //
        if (CurrentDebuggingState->SoftwareBreakpointState->SetRflagsIFBitOnMtf)
        {
            RFLAGS Rflags = {0};

            __vmx_vmread(VMCS_GUEST_RFLAGS, &Rflags);
            Rflags.InterruptEnableFlag = TRUE;
            __vmx_vmwrite(VMCS_GUEST_RFLAGS, Rflags.AsUInt);

            CurrentDebuggingState->SoftwareBreakpointState->SetRflagsIFBitOnMtf = FALSE;
        }

        CurrentDebuggingState->SoftwareBreakpointState = NULL;
    }

    //
    // *** Regular Monitor Trap Flag functionalities ***
    //
    if (CurrentVmState->MtfEptHookRestorePoint)
    {
        //
        // MTF is handled
        //
        IsMtfHandled = TRUE;

        //
        // Check for user-mode attaching mechanisms
        //
        if (g_IsWaitingForUserModeModuleEntrypointToBeCalled)
        {
            UserAccessCheckForLoadedModuleDetails();
        }

        //
        // Restore the previous state
        //
        EptHandleMonitorTrapFlag(CurrentVmState->MtfEptHookRestorePoint);

        //
        // Set it to NULL
        //
        CurrentVmState->MtfEptHookRestorePoint = NULL;

        //
        // Check if we should enable interrupts in this core or not,
        // we have another same check in SWITCHING CORES too
        //
        if (CurrentDebuggingState->EnableExternalInterruptsOnContinueMtf)
        {
            //
            // Enable normal interrupts
            //
            HvSetExternalInterruptExiting(FALSE);

            //
            // Check if there is at least an interrupt that needs to be delivered
            //
            if (CurrentVmState->PendingExternalInterrupts[0] != NULL)
            {
                //
                // Enable Interrupt-window exiting.
                //
                HvSetInterruptWindowExiting(TRUE);
            }

            CurrentDebuggingState->EnableExternalInterruptsOnContinueMtf = FALSE;
        }
    }

    //
    // Check for insturmentation step-in
    //
    if (CurrentDebuggingState->InstrumentationStepInTrace.WaitForInstrumentationStepInMtf)
    {
        //
        // MTF is handled
        //
        IsMtfHandled = TRUE;

        //
        // Check if the cs selector changed or not, which indicates that the
        // execution changed from user-mode to kernel-mode or kernel-mode to
        // user-mode
        //
        __vmx_vmread(VMCS_GUEST_CS_SELECTOR, &CsSel);

        KdCheckGuestOperatingModeChanges(CurrentDebuggingState->InstrumentationStepInTrace.CsSel,
                                         (UINT16)CsSel);

        //
        //  Unset the MTF flag and previous cs selector
        //
        CurrentDebuggingState->InstrumentationStepInTrace.WaitForInstrumentationStepInMtf = FALSE;
        CurrentDebuggingState->InstrumentationStepInTrace.CsSel                           = 0;

        //
        // Check and handle if there is a software defined breakpoint
        //
        if (!BreakpointCheckAndHandleDebuggerDefinedBreakpoints(CurrentProcessorIndex,
                                                                CurrentVmState->LastVmexitRip,
                                                                DEBUGGEE_PAUSING_REASON_DEBUGGEE_STEPPED,
                                                                GuestRegs,
                                                                &AvoidUnsetMtf))
        {
            //
            // Handle the step
            //
            ContextAndTag.Context = CurrentVmState->LastVmexitRip;
            KdHandleBreakpointAndDebugBreakpoints(CurrentProcessorIndex,
                                                  GuestRegs,
                                                  DEBUGGEE_PAUSING_REASON_DEBUGGEE_STEPPED,
                                                  &ContextAndTag);
        }
        else
        {
            //
            // Not unset again (it needs to restore the breakpoint byte)
            //
            CurrentVmState->IgnoreMtfUnset = AvoidUnsetMtf;
        }
    }

    //
    // check the condition of passing the execution to NMIs
    //
    // This one wastes one week of my life!
    // During the testing we realized the !epthook command in Debugger Mode
    // is not working. After some tests, it's because if in the middle of a
    // command in vmx-root and NMI is sent and the debugger waits for another
    // MTF, we'll ignore that MTF and a new MTF is not set again.
    // That's why we moved this check here so every command that needs a task
    // from MTF is doing its tasks and when we reached here, the check for halting
    // the debuggee in MTF is performed
    //
    else if (CurrentDebuggingState->WaitingToBeLocked)
    {
        //
        // MTF is handled
        //
        IsMtfHandled = TRUE;

        //
        // Handle break of the core
        //
        if (CurrentDebuggingState->NmiCalledInVmxRootRelatedToHaltDebuggee)
        {
            //
            // Handle it like an NMI is received from VMX root
            //
            KdHandleHaltsWhenNmiReceivedFromVmxRoot(CurrentProcessorIndex, GuestRegs);
        }
        else
        {
            //
            // Handle halt of the current core as an NMI
            //
            KdHandleNmi(CurrentProcessorIndex, GuestRegs);
        }
    }

    //
    // Check for ignored MTFs
    //
    if (CurrentDebuggingState->IgnoreOneMtf)
    {
        //
        // MTF is handled
        //
        IsMtfHandled = TRUE;

        CurrentDebuggingState->IgnoreOneMtf = FALSE;
    }

    //
    // Check for possible unhandled MTFs to avoid setting unusable MTFs
    //
    if (!IsMtfHandled)
    {
        LogError("Err, why MTF occurred?!");
    }

    //
    // Final check to unset MTF
    //
    if (!CurrentVmState->IgnoreMtfUnset)
    {
        //
        // We don't need MTF anymore if it set to disable MTF
        //
        HvSetMonitorTrapFlag(FALSE);
    }
    else
    {
        //
        // Set it to false to avoid future errors
        //
        CurrentVmState->IgnoreMtfUnset = FALSE;
    }
}
