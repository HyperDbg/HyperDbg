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
 * @param Reg 
 * @param Value 
 * @return VOID 
 */
VOID
VmxHandleXsetbv(UINT32 Reg, UINT64 Value)
{
    _xsetbv(Reg, Value);
}

/**
 * @brief Handling VMX Preemption Timer vm-exits
 * 
 * @param CurrentCoreIndex 
 * @param GuestRegs 
 * @return VOID 
 */
VOID
VmxHandleVmxPreemptionTimerVmexit(UINT32 CurrentCoreIndex, PGUEST_REGS GuestRegs)
{
    UNREFERENCED_PARAMETER(GuestRegs);
    LogError("Why vm-exit for VMX preemption timer happened?");

    //
    // Not increase the RIP by default
    //
    g_GuestState[CurrentCoreIndex].IncrementRip = FALSE;
}
