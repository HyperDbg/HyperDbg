/**
 * @file VmxFootprints.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Try to hide VMX methods from anti-debugging and anti-hypervisor
 * @details
 * @version 0.14
 * @date 2025-06-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Handle Cpuid Vmexits when the Transparent mode is enabled
 *
 * @param CpuInfo The temporary logical processor registers
 * @param Regs The virtual processor's state of registers
 *
 * @return VOID
 */
VOID
TransparentCheckAndModifyCpuid(INT32 CpuInfo[], PGUEST_REGS Regs)
{
    if (Regs->rax == CPUID_PROCESSOR_AND_PROCESSOR_FEATURE_IDENTIFIERS)
    {
        //
        // Unset the Hypervisor Present-bit in RCX, which Intel and AMD have both
        // reserved for this indication
        //
        CpuInfo[2] &= ~HYPERV_HYPERVISOR_PRESENT_BIT;
    }
    else if (Regs->rax == CPUID_HV_VENDOR_AND_MAX_FUNCTIONS || Regs->rax == HYPERV_CPUID_INTERFACE)
    {
        //
        // When transparent, all CPUID leaves in the 0x40000000+ range should contain no usable data
        //
        CpuInfo[0] = CpuInfo[1] = CpuInfo[2] = CpuInfo[3] = 0x40000000;
    }
}
