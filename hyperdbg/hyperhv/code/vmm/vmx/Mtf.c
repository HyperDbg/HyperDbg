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
    HvSuppressRipIncrement(VCpu);

    //
    // Explicitly say that we want to unset MTFs
    //
    VCpu->IgnoreMtfUnset = FALSE;

    //
    // Check for KD related MTFs
    //
    if (VmmCallbackHandleMtfCallback(VCpu->CoreId))
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
        // Restore the previous state
        //
        EptHookHandleMonitorTrapFlag(VCpu);

        //
        // Set it to NULL
        //
        VCpu->MtfEptHookRestorePoint = NULL;

        //
        // Check for re-enabling external interrupts
        //
        HvEnableAndCheckForPreviousExternalInterrupts(VCpu);
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
