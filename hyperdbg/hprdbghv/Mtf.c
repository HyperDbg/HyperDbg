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

    if (g_GuestState[CurrentProcessorIndex].MtfEptHookRestorePoint)
    {
        //
        // Restore the previous state
        //
        EptHandleMonitorTrapFlag(g_GuestState[CurrentProcessorIndex].MtfEptHookRestorePoint);

        //
        // Set it to NULL
        //
        g_GuestState[CurrentProcessorIndex].MtfEptHookRestorePoint = NULL;
    }
    else if (g_GuestState[CurrentProcessorIndex].DebuggingState.InstrumentInTrace.WaitForStepOnMtf)
    {
        //
        // Check if the cs selector changed or not, which indicates that the
        // execution changed from user-mode to kernel-mode or kernel-mode to
        // user-mode
        //
        __vmx_vmread(GUEST_CS_SELECTOR, &CsSel);

        KdCheckGuestOperatingModeChanges(g_GuestState[CurrentProcessorIndex].DebuggingState.InstrumentInTrace.CsSel,
                                         CsSel);

        //
        //  Unset the MTF flag and previous cs selector
        //
        g_GuestState[CurrentProcessorIndex].DebuggingState.InstrumentInTrace.WaitForStepOnMtf = FALSE;
        g_GuestState[CurrentProcessorIndex].DebuggingState.InstrumentInTrace.CsSel            = 0;

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
    else if (g_GuestState[CurrentProcessorIndex].MtfTest)
    {
        SteppingsHandleThreadChanges(GuestRegs, CurrentProcessorIndex);
        g_GuestState[CurrentProcessorIndex].MtfTest = FALSE;
    }
    else if (!IsMtfForReApplySoftwareBreakpoint)
    {
        LogError("Why MTF occured ?!");
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
