/**
 * @file VmxMechanisms.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief VMX based mechanisms
 * @details
 * @version 0.1
 * @date 2021-12-16
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "..\hprdbghv\pch.h"

/**
 * @brief Create an immediate vm-exit after vm-entry
 * 
 * @return VOID 
 */
VOID
VmxMechanismCreateImmediateVmexit()
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
 * @brief Disable the immediate vm-exit after vm-entry
 * 
 * @return VOID 
 */
VOID
VmxMechanismDisableImmediateVmexit()
{
    CounterClearPreemptionTimer();

    //
    // Disable the VMX preemption timer on pin-based controls
    //
    HvSetVmxPreemptionTimerExiting(FALSE);
}
