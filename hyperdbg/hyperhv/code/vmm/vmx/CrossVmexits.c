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
 * @param VCpu
 *
 * @return VOID
 */
VOID
VmxHandleXsetbv(VIRTUAL_MACHINE_STATE * VCpu)
{
    CPUID_EAX_01         CpuidInfo;
    VMX_SEGMENT_SELECTOR GuestCs  = GetGuestCs();
    UINT32               XCrIndex = VCpu->Regs->rcx & 0xffffffff;
    XCR0                 XCr0     = {.AsUInt = VCpu->Regs->rdx << 32 | VCpu->Regs->rax};
    CR4                  GuestCr4 = {.AsUInt = GetGuestCr4()};

    if (XCrIndex != 0 || GuestCs.Attributes.DescriptorPrivilegeLevel != 0 || !CommonIsXCr0Valid(XCr0))
    {
        EventInjectGeneralProtection();
        return;
    }

    CommonCpuidInstruction(CPUID_VERSION_INFORMATION, 0, (INT32 *)&CpuidInfo);

    if (CpuidInfo.CpuidFeatureInformationEcx.XsaveXrstorInstruction == 0 || GuestCr4.OsXsave == 0)
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    _xsetbv(XCrIndex, XCr0.AsUInt);
}

/**
 * @brief Handling VMX Preemption Timer vm-exits
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
VmxHandleVmxPreemptionTimerVmexit(VIRTUAL_MACHINE_STATE * VCpu)
{
    LogError("Why vm-exit for VMX preemption timer happened?");

    //
    // Not increase the RIP by default
    //
    HvSuppressRipIncrement(VCpu);
}

/**
 * @brief Handling triple fault VM-exits
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
VmxHandleTripleFaults(VIRTUAL_MACHINE_STATE * VCpu)
{
    LogError("Err, triple fault error occurred!");

    //
    // This error cannot be recovered, produce debug results
    //
    CommonWriteDebugInformation(VCpu);

    //
    // We won't further continue after this error
    //
    DbgBreakPoint();
}
