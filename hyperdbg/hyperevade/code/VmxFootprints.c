/**
 * @file VmxFootprints.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @author jtaw5649
 * @brief Try to hide VMX methods from anti-debugging and anti-hypervisor
 * @details
 * @version 0.14
 * @date 2025-06-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#define HYPERV_VENDOR_MICROSOFT_EBX  0x7263694d // "Micr"
#define HYPERV_VENDOR_MICROSOFT_ECX  0x666f736f // "osof"
#define HYPERV_VENDOR_MICROSOFT_EDX  0x76482074 // "t Hv"
#define HYPERV_INTERFACE_HV1_EAX     0x31237648 // "Hv#1"
#define TRANSPARENT_HYPERV_CPUID_MAX HYPERV_CPUID_IMPLEMENT_LIMITS

static VOID
TransparentZeroCpuidLeaf(INT32 CpuInfo[])
{
    CpuInfo[0] = CpuInfo[1] = CpuInfo[2] = CpuInfo[3] = 0;
}

static BOOLEAN
TransparentCpuidLeafIsHypervisorRange(UINT64 Leaf)
{
    return Leaf >= HYPERV_CPUID_VENDOR_AND_MAX_FUNCTIONS && Leaf <= HYPERV_CPUID_MAX;
}

static BOOLEAN
TransparentGuestExecutionIsUserMode()
{
    return g_Callbacks.IsGuestExecutionUserMode();
}

static BOOLEAN
TransparentReservedMsrAccessShouldFault(UINT32 TargetMsr)
{
    // The Hyper-V synthetic MSR range is used by the Windows kernel for timers.
    // #GP injection for kernel traffic can crash nested guests on Meteor Lake.
    // Ref: https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/timers
    return TargetMsr >= RESERVED_MSR_RANGE_LOW &&
           TargetMsr <= RESERVED_MSR_RANGE_HI &&
           TransparentGuestExecutionIsUserMode();
}

static VOID
TransparentApplyBareMetalCpuidPolicy(PGUEST_REGS Regs, INT32 CpuInfo[])
{
    if (Regs->rax == CPUID_PROCESSOR_AND_PROCESSOR_FEATURE_IDENTIFIERS)
    {
        // Intel and AMD reserve ECX[31] as the hypervisor-present bit.
        CpuInfo[2] &= ~HYPERV_HYPERVISOR_PRESENT_BIT;
    }
    else if (TransparentCpuidLeafIsHypervisorRange(Regs->rax))
    {
        // Bare-metal policy hides the synthetic hypervisor CPUID range.
        TransparentZeroCpuidLeaf(CpuInfo);
    }
}

static VOID
TransparentApplyHyperVCpuidPolicy(PGUEST_REGS Regs, INT32 CpuInfo[])
{
    if (Regs->rax == CPUID_PROCESSOR_AND_PROCESSOR_FEATURE_IDENTIFIERS)
    {
        CpuInfo[2] |= HYPERV_HYPERVISOR_PRESENT_BIT;
    }
    else if (Regs->rax == HYPERV_CPUID_VENDOR_AND_MAX_FUNCTIONS)
    {
        // Expose a coherent Microsoft Hyper-V identity instead of HyperDbg's Hv#0 footprint.
        CpuInfo[0] = TRANSPARENT_HYPERV_CPUID_MAX;
        CpuInfo[1] = HYPERV_VENDOR_MICROSOFT_EBX;
        CpuInfo[2] = HYPERV_VENDOR_MICROSOFT_ECX;
        CpuInfo[3] = HYPERV_VENDOR_MICROSOFT_EDX;
    }
    else if (Regs->rax == HYPERV_CPUID_INTERFACE)
    {
        CpuInfo[0] = HYPERV_INTERFACE_HV1_EAX;
        CpuInfo[1] = CpuInfo[2] = CpuInfo[3] = 0;
    }
    else if (Regs->rax >= HYPERV_CPUID_VERSION && Regs->rax <= HYPERV_CPUID_IMPLEMENT_LIMITS)
    {
        // Keep the Hv#1 range TLFS-coherent without advertising unbacked features.
        TransparentZeroCpuidLeaf(CpuInfo);
    }
    else if (TransparentCpuidLeafIsHypervisorRange(Regs->rax) && Regs->rax > TRANSPARENT_HYPERV_CPUID_MAX)
    {
        TransparentZeroCpuidLeaf(CpuInfo);
    }
}

/**
 * @brief Handle Cpuid Vmexits when the Transparent mode is enabled
 *
 * @param Regs The virtual processor's state of registers
 * @param CpuInfo The temporary logical processor registers
 *
 * @return BOOLEAN Whether transparent CPUID handling is active
 */
BOOLEAN
TransparentCheckAndModifyCpuid(PGUEST_REGS Regs, INT32 CpuInfo[])
{
    if ((g_TransparentEvadeMask & TRANSPARENT_EVADE_MASK_CPUID) == 0)
    {
        return FALSE;
    }

    if ((g_TransparentEvadeMask & TRANSPARENT_EVADE_MASK_CPUID_BARE_METAL) != 0)
    {
        TransparentApplyBareMetalCpuidPolicy(Regs, CpuInfo);
    }
    else
    {
        TransparentApplyHyperVCpuidPolicy(Regs, CpuInfo);
    }

    return TRUE;
}

/**
 * @brief Handle RDMSR VM exits when the Transparent mode is enabled
 *
 * @param Regs The virtual processor's state of registers
 * @param TargetMsr Target MSR in ECX register
 *
 * @return BOOLEAN Whether the emulation should be further continued or not
 */
BOOLEAN
TransparentCheckAndModifyMsrRead(PGUEST_REGS Regs, UINT32 TargetMsr)
{
    if ((g_TransparentEvadeMask & TRANSPARENT_EVADE_MASK_MSR) == 0)
    {
        UNREFERENCED_PARAMETER(Regs);
        UNREFERENCED_PARAMETER(TargetMsr);

        return FALSE;
    }

    UNREFERENCED_PARAMETER(Regs);

    if (TransparentReservedMsrAccessShouldFault(TargetMsr))
    {
        g_Callbacks.EventInjectGeneralProtection();
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief Handle WRMSR VM exits when the Transparent mode is enabled
 *
 * @param Regs The virtual processor's state of registers
 * @param TargetMsr Target MSR in ECX register
 *
 * @return BOOLEAN Whether the emulation should be further continued or not
 */
BOOLEAN
TransparentCheckAndModifyMsrWrite(PGUEST_REGS Regs, UINT32 TargetMsr)
{
    if ((g_TransparentEvadeMask & TRANSPARENT_EVADE_MASK_MSR) == 0)
    {
        UNREFERENCED_PARAMETER(Regs);
        UNREFERENCED_PARAMETER(TargetMsr);

        return FALSE;
    }

    UNREFERENCED_PARAMETER(Regs);

    if (TransparentReservedMsrAccessShouldFault(TargetMsr))
    {
        g_Callbacks.EventInjectGeneralProtection();
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief Handle anti-debugging method of a trap flag after a VM exit
 *
 * @return VOID
 */
VOID
TransparentCheckAndTrapFlagAfterVmexit()
{
    if ((g_TransparentEvadeMask & TRANSPARENT_EVADE_MASK_TRAP_FLAG) == 0)
    {
        return;
    }

    //
    // If RIP is incremented, then we emulate an instruction, and then
    // we need to handle the trap flag if it is set in a guest
    //
    g_Callbacks.HvHandleTrapFlag();
}
