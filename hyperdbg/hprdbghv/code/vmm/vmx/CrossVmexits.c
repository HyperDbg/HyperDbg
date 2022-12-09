/**
 * @file CrossVmexits.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The functions for passing vm-exits in vmx root
 * @details
 * @version 0.1
 * @date 2020-06-14
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Handling XSETBV Instruction vm-exits
 *
 * @param VCpu
 * @return VOID
 */
VOID
VmxHandleXsetbv(VIRTUAL_MACHINE_STATE * VCpu)
{
    _xsetbv(VCpu->Regs->rcx & 0xffffffff, VCpu->Regs->rdx << 32 | VCpu->Regs->rax);
}

/**
 * @brief Handling VMX Preemption Timer vm-exits
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
VmxHandleVmxPreemptionTimerVmexit(VIRTUAL_MACHINE_STATE * VCpu)
{
    LogError("Why vm-exit for VMX preemption timer happened?");

    //
    // Not increase the RIP by default
    //
    VmFuncSuppressRipIncrement(VCpu->CoreId);
}
