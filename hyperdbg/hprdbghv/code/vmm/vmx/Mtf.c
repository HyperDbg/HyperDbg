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
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
MtfHandleVmexit(VIRTUAL_MACHINE_STATE * VCpu)
{
    BOOLEAN IsMtfHandled = FALSE;

    //
    // Redo the instruction
    //
    VmFuncSuppressRipIncrement(VCpu->CoreId);

    //
    // Explicitly say that we want to unset MTFs
    //
    VCpu->IgnoreMtfUnset = FALSE;

    //
    // Check if we need to re-apply a breakpoint or not
    // We check it separately because the guest might step
    // instructions on an MTF so we want to check for the step too
    //
    if (BreakpointCheckAndHandleReApplyingBreakpoint(VCpu->CoreId))
    {
        //
        // MTF is handled
        //
        IsMtfHandled = TRUE;
    }

    //
    // *** Regular Monitor Trap Flag functionalities ***
    //
    if (VCpu->MtfEptHookRestorePoint)
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
        EptHandleMonitorTrapFlag(VCpu->MtfEptHookRestorePoint);

        //
        // Check to trigger the post event (for events relating the !monitor command
        // and the emulation hardware debug registers)
        //
        if (VCpu->MtfEptHookRestorePoint->IsPostEventTriggerAllowed)
        {
            //
            // Check whether this is a "write" monitor hook or not
            //
            if (VCpu->MtfEptHookRestorePoint->IsMonitorToWriteOnPages)
            {
                //
                // This is a "write" hook
                //
                DispatchEventHiddenHookPageReadWriteWritePostEvent(VCpu,
                                                                   &VCpu->MtfEptHookRestorePoint->LastContextState);
            }
            else
            {
                //
                // This is a "read" hook
                //
                DispatchEventHiddenHookPageReadWriteReadPostEvent(VCpu,
                                                                  &VCpu->MtfEptHookRestorePoint->LastContextState);
            }
        }

        //
        // Set it to NULL
        //
        VCpu->MtfEptHookRestorePoint = NULL;

        //
        // Check if we should enable interrupts in this core or not,
        // we have another same check in SWITCHING CORES too
        //
        if (VCpu->EnableExternalInterruptsOnContinueMtf)
        {
            //
            // Enable normal interrupts
            //
            HvSetExternalInterruptExiting(VCpu, FALSE);

            //
            // Check if there is at least an interrupt that needs to be delivered
            //
            if (VCpu->PendingExternalInterrupts[0] != NULL)
            {
                //
                // Enable Interrupt-window exiting.
                //
                HvSetInterruptWindowExiting(TRUE);
            }

            VCpu->EnableExternalInterruptsOnContinueMtf = FALSE;
        }
    }

    //
    // Check for insturmentation step-in
    //
    if (VCpu->RegisterBreakOnMtf)
    {
        //
        // MTF is handled
        //
        IsMtfHandled = TRUE;

        //
        // Change the MTF registeration state (might be changed in the caller)
        //
        VCpu->RegisterBreakOnMtf = FALSE;

        //
        // Handle MTF in the debugger
        //
        g_Callbacks.KdHandleRegisteredMtfCallback(VCpu->CoreId);
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
    else if (KdCheckAndHandleNmiCallback(VCpu->CoreId))
    {
        //
        // MTF is handled
        //
        IsMtfHandled = TRUE;
    }

    //
    // Check for ignored MTFs
    //
    if (VCpu->IgnoreOneMtf)
    {
        //
        // MTF is handled
        //
        IsMtfHandled = TRUE;

        VCpu->IgnoreOneMtf = FALSE;
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
    if (!VCpu->IgnoreMtfUnset)
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
        VCpu->IgnoreMtfUnset = FALSE;
    }
}
