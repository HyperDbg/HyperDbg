/**
 * @file VmxBroadcast.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Broadcast mechanism in vmx-root
 * @details
 * @version 0.1
 * @date 2021-12-31
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Handles NMIs in kernel-mode
 *
 * @param Context
 * @param Handled
 * @return BOOLEAN
 */
BOOLEAN
VmxBroadcastHandleNmiCallback(PVOID Context, BOOLEAN Handled)
{
    ULONG CurrentCoreIndex;
    CurrentCoreIndex             = KeGetCurrentProcessorNumber();
    VIRTUAL_MACHINE_STATE * VCpu = &g_GuestState[CurrentCoreIndex];

    //
    // This mechanism tries to solve the problem of receiving NMIs
    // when we're already in vmx-root mode, e.g., when we want to
    // inject NMIs to other cores and those cores are already operating
    // in vmx-root mode; however, this is not the approach to solve the
    // problem. In order to solve this problem, we should create our own
    // host IDT in vmx-root mode (Note that we should set VMCS_HOST_IDTR_BASE
    // and there is no need to LIMIT as it's fixed at 0xffff for VMX
    // operations).
    // Because we want to use the debugging mechanism of the Windows
    // we use the same IDT with the guest (guest and host IDT is the
    // same), but in the future versions we solve this problem by our
    // own ISR NMI handler in vmx-root mode
    //

    //
    // We should check whether the NMI is in vmx-root mode or not
    // if it's not in vmx-root mode then it's not related to us
    //
    if (!VmxBroadcastNmiHandler(VCpu, TRUE))
    {
        return Handled;
    }
    else
    {
        //
        // If we're here then it related to us
        // return true to show that it's handled
        //
        return TRUE;
    }
}

/**
 * @brief Broadcast NMI in vmx-root mode
 * @details caller to this function should take actions to the current core
 * the NMI won't be triggered for the current core
 *
 * @param VCpu The virtual processor's state
 * @param VmxBroadcastAction
 * @return BOOLEAN
 */
BOOLEAN
VmxBroadcastNmi(VIRTUAL_MACHINE_STATE * VCpu, NMI_BROADCAST_ACTION_TYPE VmxBroadcastAction)
{
    ULONG CoreCount;

    //
    // Check if NMI broadcasting is initialized
    //
    if (!g_NmiBroadcastingInitialized)
    {
        return FALSE;
    }

    CoreCount = KeQueryActiveProcessorCount(0);

    //
    // make sure, nobody is in the middle of sending anything
    //
    SpinlockLock(&DebuggerResponseLock);

    //
    // Indicate that we're waiting for NMI
    //
    for (size_t i = 0; i < CoreCount; i++)
    {
        if (i != VCpu->CoreId)
        {
            SpinlockInterlockedCompareExchange((volatile LONG *)&VCpu->NmiBroadcastingState.NmiBroadcastAction,
                                               VmxBroadcastAction,
                                               NMI_BROADCAST_ACTION_NONE);
        }
    }

    //
    // Broadcast NMI through APIC (xAPIC or x2APIC)
    //
    ApicTriggerGenericNmi();

    SpinlockUnlock(&DebuggerResponseLock);

    return TRUE;
}

/**
 * @brief Handle broadcast NMIs for halting cores in vmx-root mode
 *
 * @param CurrentCoreIndex
 * @param GuestRegs
 * @param IsOnVmxNmiHandler
 *
 * @return VOID
 */
VOID
VmxBroadcastHandleKdDebugBreaks(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN IsOnVmxNmiHandler)
{
    //
    // We use it as a global flag (for both vmx-root and vmx non-root), because
    // generally it doesn't have any use case in vmx-root (IsOnVmxNmiHandler == FALSE)
    // but in some cases, we might set the MTF but another vm-exit receives before
    // MTF and in that place if it tries to trigger and event, then the MTF is not
    // handled and the core is not locked properly, just waits to get the handle
    // of the "DebuggerHandleBreakpointLock", so we check this flag there
    //
    VCpu->NmiBroadcastingState.WaitingToBeLocked = TRUE;

    if (IsOnVmxNmiHandler)
    {
        //
        // Indicate that it's called from NMI handle, and it relates to
        // halting the debuggee
        //
        VCpu->NmiBroadcastingState.NmiCalledInVmxRootRelatedToHaltDebuggee = TRUE;

        //
        // If the core was in the middle of spinning on the spinlock
        // of getting the debug lock, this mechansim is not needed,
        // but if the core is not spinning there or the core is processing
        // a random vm-exit, then we inject an immediate vm-exit after vm-entry
        // or inject a DPC
        // this is used for two reasons.
        //
        //      1. first, we will get the registers (context) to halt the core
        //      2. second, it guarantees that if the NMI arrives within any
        //         instruction in vmx-root mode, then we injected an immediate
        //         vm-exit and we won't miss any cpu cycle in the guest
        //
        // KdFireDpc(KdHaltCoreInTheCaseOfHaltedFromNmiInVmxRoot, NULL);
        // VmxMechanismCreateImmediateVmexit(VCpu);
        HvSetMonitorTrapFlag(TRUE);
    }
    else
    {
        //
        // Handle core break
        //
        KdHandleNmi(VCpu);
    }
}

/**
 * @brief Handle broadcast NMIs in vmx-root mode
 *
 * @param VCpu The virtual processor's state
 * @param IsOnVmxNmiHandler
 *
 * @return BOOLEAN Shows whether it's handled by this routine or not
 */
BOOLEAN
VmxBroadcastNmiHandler(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN IsOnVmxNmiHandler)
{
    NMI_BROADCAST_ACTION_TYPE BroadcastAction;
    BOOLEAN                   IsHandled = FALSE;

    //
    // Check if NMI relates to us or not
    // Set NMI broadcasting action to none (clear the action)
    //
    BroadcastAction = InterlockedExchange((volatile LONG *)&VCpu->NmiBroadcastingState.NmiBroadcastAction, NMI_BROADCAST_ACTION_NONE);

    if (BroadcastAction == NMI_BROADCAST_ACTION_NONE)
    {
        IsHandled = FALSE;
        goto ReturnIsHandled;
    }

    //
    // *** It's relating to us ***
    //

    switch (BroadcastAction)
    {
    case NMI_BROADCAST_ACTION_TEST:

        //
        // Use for stress testing of the broadcasting mechanism
        //
        IsHandled = TRUE;

        break;
    case NMI_BROADCAST_ACTION_KD_HALT_CORE:

        //
        // Handle NMI of halt the other cores
        //
        IsHandled = TRUE;
        VmxBroadcastHandleKdDebugBreaks(VCpu, IsOnVmxNmiHandler);

        break;

    default:

        IsHandled = FALSE;
        LogError("Err, invalid NMI reason received");

        break;
    }

ReturnIsHandled:
    return IsHandled;
}
