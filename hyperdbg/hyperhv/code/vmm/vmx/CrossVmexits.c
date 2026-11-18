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

    CpuXsetbv(XCrIndex, XCr0.AsUInt);
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
    PlatformDbgBreakPoint();
}

/**
 * @brief Handle a vm-entry failure
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
VmxHandleVmentryFailures(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT32 ActivityState         = 0;
    UINT32 EntryInterruptionInfo = 0;

    VmxVmread32P(VMCS_GUEST_ACTIVITY_STATE, &ActivityState);
    VmxVmread32P(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, &EntryInterruptionInfo);

    LogError("Err, vm-entry failed, reason: 0x%x, qualification: 0x%x, rflags: 0x%llx, "
             "interruptibility: 0x%llx, activity: 0x%x, pending debug exceptions: 0x%llx, "
             "debugctl: 0x%llx, injected event: 0x%x",
             VCpu->ExitReason,
             VCpu->ExitQualification,
             HvGetRflags(),
             HvGetInterruptibilityState(),
             ActivityState,
             HvGetPendingDebugExceptions(),
             HvGetDebugctl(),
             EntryInterruptionInfo);

    //
    // Keep guest RIP unchanged because vm-entry failed
    //
    HvSuppressRipIncrement(VCpu);

    CommonWriteDebugInformation(VCpu);

    //
    // Break into the debugger instead of retrying
    //
    PlatformDbgBreakPoint();
}
