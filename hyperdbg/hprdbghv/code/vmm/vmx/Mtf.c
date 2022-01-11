/**
 * @file Mtf.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Routines relating to Monitor Trap Flag (MTF)
 * @details 
 * @version 0.1
 * @date 2021-01-27
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "..\hprdbghv\pch.h"

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
    BOOLEAN                          IsMtfForReApplySoftwareBreakpoint = FALSE;
    DEBUGGER_TRIGGERED_EVENT_DETAILS ContextAndTag                     = {0};
    BOOLEAN                          AvoidUnsetMtf;
    UINT16                           CsSel;

    //
    // Redo the instruction
    //
    g_GuestState[CurrentProcessorIndex].IncrementRip = FALSE;

    //
    // Explicitly say that we want to unset MTFs
    //
    g_GuestState[CurrentProcessorIndex].IgnoreMtfUnset = FALSE;

    //
    // Check if we need to re-apply a breakpoint or not
    // We check it separately because the guest might step
    // instructions on an MTF so we want to check for the step too
    //
    if (g_GuestState[CurrentProcessorIndex].DebuggingState.SoftwareBreakpointState != NULL)
    {
        BYTE BreakpointByte               = 0xcc;
        IsMtfForReApplySoftwareBreakpoint = TRUE;

        //
        // Restore previous breakpoint byte
        //

        MemoryMapperWriteMemorySafeByPhysicalAddress(
            g_GuestState[CurrentProcessorIndex].DebuggingState.SoftwareBreakpointState->PhysAddress,
            &BreakpointByte,
            sizeof(BYTE));

        //
        // Check if we should re-enabled IF bit of RFLAGS or not
        //
        if (g_GuestState[CurrentProcessorIndex].DebuggingState.SoftwareBreakpointState->SetRflagsIFBitOnMtf)
        {
            RFLAGS Rflags = {0};

            __vmx_vmread(GUEST_RFLAGS, &Rflags);
            Rflags.InterruptEnableFlag = TRUE;
            __vmx_vmwrite(GUEST_RFLAGS, Rflags.Value);

            g_GuestState[CurrentProcessorIndex].DebuggingState.SoftwareBreakpointState->SetRflagsIFBitOnMtf = FALSE;
        }

        g_GuestState[CurrentProcessorIndex].DebuggingState.SoftwareBreakpointState = NULL;
    }

    //
    // *** Regular Monitor Trap Flag functionalities ***
    //

    //
    // check the condition of passing the execution to NMIs
    //
    if (g_GuestState[CurrentProcessorIndex].DebuggingState.NmiCalledInVmxRootRelatedToHaltDebuggee)
    {
        //
        // Handle break of the core
        //
        KdHandleHaltsWhenNmiReceivedFromVmxRoot(CurrentProcessorIndex, GuestRegs);
    }
    else if (g_GuestState[CurrentProcessorIndex].MtfEptHookRestorePoint)
    {
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
        EptHandleMonitorTrapFlag(g_GuestState[CurrentProcessorIndex].MtfEptHookRestorePoint);

        //
        // Set it to NULL
        //
        g_GuestState[CurrentProcessorIndex].MtfEptHookRestorePoint = NULL;

        //
        // Check if we should enable interrupts in this core or not,
        // we have another same check in SWITCHING CORES too
        //
        if (g_GuestState[CurrentProcessorIndex].DebuggingState.EnableExternalInterruptsOnContinueMtf)
        {
            //
            // Enable normal interrupts
            //
            HvSetExternalInterruptExiting(FALSE);

            //
            // Check if there is at least an interrupt that needs to be delivered
            //
            if (g_GuestState[CurrentProcessorIndex].PendingExternalInterrupts[0] != NULL)
            {
                //
                // Enable Interrupt-window exiting.
                //
                HvSetInterruptWindowExiting(TRUE);
            }

            g_GuestState[CurrentProcessorIndex].DebuggingState.EnableExternalInterruptsOnContinueMtf = FALSE;
        }
    }
    else if (g_GuestState[CurrentProcessorIndex].DebuggingState.InstrumentationStepInTrace.WaitForInstrumentationStepInMtf)
    {
        //
        // Check if the cs selector changed or not, which indicates that the
        // execution changed from user-mode to kernel-mode or kernel-mode to
        // user-mode
        //
        __vmx_vmread(GUEST_CS_SELECTOR, &CsSel);

        KdCheckGuestOperatingModeChanges(g_GuestState[CurrentProcessorIndex].DebuggingState.InstrumentationStepInTrace.CsSel,
                                         CsSel);

        //
        //  Unset the MTF flag and previous cs selector
        //
        g_GuestState[CurrentProcessorIndex].DebuggingState.InstrumentationStepInTrace.WaitForInstrumentationStepInMtf = FALSE;
        g_GuestState[CurrentProcessorIndex].DebuggingState.InstrumentationStepInTrace.CsSel                           = 0;

        //
        // Check and handle if there is a software defined breakpoint
        //
        if (!BreakpointCheckAndHandleDebuggerDefinedBreakpoints(CurrentProcessorIndex,
                                                                g_GuestState[CurrentProcessorIndex].LastVmexitRip,
                                                                DEBUGGEE_PAUSING_REASON_DEBUGGEE_STEPPED,
                                                                GuestRegs,
                                                                &AvoidUnsetMtf))
        {
            //
            // Handle the step
            //
            ContextAndTag.Context = g_GuestState[CurrentProcessorIndex].LastVmexitRip;
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
            g_GuestState[CurrentProcessorIndex].IgnoreMtfUnset = AvoidUnsetMtf;
        }
    }
    else if (g_GuestState[CurrentProcessorIndex].DebuggingState.IgnoreOneMtf)
    {
        //
        // Nothing to do, just ignore
        //
        g_GuestState[CurrentProcessorIndex].DebuggingState.IgnoreOneMtf = FALSE;
    }
    else if (!IsMtfForReApplySoftwareBreakpoint)
    {
        LogError("Err, why MTF occurred?!");
    }

    //
    // Final check to unset mtf
    //
    if (!g_GuestState[CurrentProcessorIndex].IgnoreMtfUnset)
    {
        //
        // We don't need MTF anymore if it set to disable MTF
        //
        HvSetMonitorTrapFlag(FALSE);
        g_GuestState[CurrentProcessorIndex].IgnoreMtfUnset = FALSE;
    }
}
