/**
 * @file VmxMechanisms.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief VMX based mechanisms
 * @details
 * @version 0.1
 * @date 2021-12-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Create an immediate vm-exit after vm-entry by using
 * VMX Preemption Timer
 * @return VOID
 */
VOID
VmxMechanismCreateImmediateVmexitByVmxPreemptionTimer()
{
    //
    // Activate VMX preemption timer on pin-based controls
    //
    HvSetVmxPreemptionTimerExiting(TRUE);

    //
    // Setting VMX preemption timer to 0, cause and immediate
    // vm-exit and architecturally guarantees that no instruction
    // get a chance to be executed
    //
    CounterSetPreemptionTimer(0);
}

/**
 * @brief Disable the immediate vm-exit after vm-entry by using
 * VMX Preemption Timer
 *
 * @return VOID
 */
VOID
VmxMechanismDisableImmediateVmexitByVmxPreemptionTimer()
{
    CounterClearPreemptionTimer();

    //
    // Disable the VMX preemption timer on pin-based controls
    //
    HvSetVmxPreemptionTimerExiting(FALSE);
}

/**
 * @brief Create an immediate vm-exit after vm-entry by using
 * self-ipi
 *
 * @return VOID
 */
VOID
VmxMechanismCreateImmediateVmexitBySelfIpi()
{
    //
    // Send self-ipi on the target vector using xAPIC or x2APIC
    //
    ApicSelfIpi(IMMEDIATE_VMEXIT_MECHANISM_VECTOR_FOR_SELF_IPI);
}

/**
 * @brief Create an immediate vm-exit after vm-entry
 *
 * @param CurrentCoreIndex
 * @return VOID
 */
VOID
VmxMechanismCreateImmediateVmexit(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // I didn't test vm-exit by preemption timer as my machine
    // or maybe VMware workstation's nested virtualization didn't
    // support VMX Preemption Timer, that's why we use self-ipi
    // method by default
    //

    //
    // Indicate wait for an immediate vm-exit
    //
    VCpu->WaitForImmediateVmexit = TRUE;

    //
    // Self-ipi current core
    //
    VmxMechanismCreateImmediateVmexitBySelfIpi();

    //
    // Set vm-exit on external interrupts
    //
    HvSetExternalInterruptExiting(VCpu, TRUE);
}

/**
 * @brief Handle immediate vm-exit after vm-entry
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
VmxMechanismHandleImmediateVmexit(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Not waiting for immediate vm-exit anymore
    //
    VCpu->WaitForImmediateVmexit = FALSE;

    //
    // Set vm-exit on external interrupts
    //
    HvSetExternalInterruptExiting(VCpu, FALSE);
}
