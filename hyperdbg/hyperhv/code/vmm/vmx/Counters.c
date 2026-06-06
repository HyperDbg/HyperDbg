/**
 * @file Counters.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The functions for emulating counters
 * @details
 * @version 0.1
 * @date 2020-06-14
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

static BOOLEAN
CounterIsRdtscExitingEnabled()
{
    UINT32 CpuBasedVmExecControls = 0;

    VmxVmread32P(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuBasedVmExecControls);

    return (CpuBasedVmExecControls & IA32_VMX_PROCBASED_CTLS_RDTSC_EXITING_FLAG) != 0;
}

static BOOLEAN
CounterIsRdpmcExitingEnabled()
{
    UINT32 CpuBasedVmExecControls = 0;

    VmxVmread32P(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &CpuBasedVmExecControls);

    return (CpuBasedVmExecControls & IA32_VMX_PROCBASED_CTLS_RDPMC_EXITING_FLAG) != 0;
}

VOID
CounterClearCpuidTscCompensation(VIRTUAL_MACHINE_STATE * VCpu)
{
    if (!VCpu->TransparencyState.CpuidTscCompensationArmed)
    {
        return;
    }

    VCpu->TransparencyState.CpuidTscCompensationArmed = FALSE;
    VCpu->TransparencyState.CpuidTscEntryTsc          = 0;
    VCpu->TransparencyState.CpuidTscEntryRip          = 0;
    VCpu->TransparencyState.CpuidTscEntryRsp          = 0;
    VCpu->TransparencyState.CpuidTscEntryLeaf         = 0;
    VCpu->TransparencyState.CpuidTscEntrySubleaf      = 0;
}

VOID
CounterEnableTransparentCpuidTscTiming(VIRTUAL_MACHINE_STATE * VCpu)
{
    VCpu->TransparencyState.TransparentCpuidTscTimingEnabled = TRUE;

    if (!CounterIsRdtscExitingEnabled())
    {
        ProtectedHvSetRdtscExiting(VCpu, TRUE);
        VCpu->TransparencyState.TransparentCpuidTscHadForcedRdtscExiting = TRUE;
    }

    if (!CounterIsRdpmcExitingEnabled())
    {
        ProtectedHvSetPmcVmexit(VCpu, TRUE);
        VCpu->TransparencyState.TransparentCpuidTscHadForcedRdpmcExiting = TRUE;
    }
}

VOID
CounterDisableTransparentCpuidTscTiming(VIRTUAL_MACHINE_STATE * VCpu)
{
    BOOLEAN DisableRdtscExiting = VCpu->TransparencyState.TransparentCpuidTscHadForcedRdtscExiting;
    BOOLEAN DisableRdpmcExiting = VCpu->TransparencyState.TransparentCpuidTscHadForcedRdpmcExiting;

    CounterClearCpuidTscCompensation(VCpu);

    VCpu->TransparencyState.LastGuestTimeStampCounter                = 0;
    VCpu->TransparencyState.LastGuestTimeStampCounterValid           = FALSE;
    VCpu->TransparencyState.LastRevealedTimeStampCounter             = 0;
    VCpu->TransparencyState.LastTransparentTscHostCounter            = 0;
    VCpu->TransparencyState.LastTransparentTscGuestCounter           = 0;
    VCpu->TransparencyState.LastTransparentTscValid                  = FALSE;
    VCpu->TransparencyState.TransparentCpuidTscTimingEnabled         = FALSE;
    VCpu->TransparencyState.TransparentCpuidTscHadForcedRdtscExiting = FALSE;
    VCpu->TransparencyState.TransparentCpuidTscHadForcedRdpmcExiting = FALSE;

    if (DisableRdtscExiting)
    {
        ProtectedHvSetRdtscExiting(VCpu, FALSE);
    }

    if (DisableRdpmcExiting)
    {
        ProtectedHvSetPmcVmexit(VCpu, FALSE);
    }
}

/**
 * @brief Emulate RDTSC
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
CounterEmulateRdtsc(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // I realized that if you log anything here (LogInfo) then
    // the system-halts, currently don't have any idea of how
    // to solve it, in the future we solve it using tsc offsetting
    // or tsc scalling (The reason is because of that fucking patchguard :( )
    //
    UINT64      Tsc       = CpuReadTsc();
    PGUEST_REGS GuestRegs = VCpu->Regs;

    GuestRegs->rax = 0x00000000ffffffff & Tsc;
    GuestRegs->rdx = 0x00000000ffffffff & (Tsc >> 32);
}

/**
 * @brief Emulate RDTSCP
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
CounterEmulateRdtscp(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT32      Aux       = 0;
    UINT64      Tsc       = CpuReadTscp(&Aux);
    PGUEST_REGS GuestRegs = VCpu->Regs;

    GuestRegs->rax = 0x00000000ffffffff & Tsc;
    GuestRegs->rdx = 0x00000000ffffffff & (Tsc >> 32);

    GuestRegs->rcx = 0x00000000ffffffff & Aux;
}

/**
 * @brief Emulate RDPMC
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
CounterEmulateRdpmc(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT32      EcxReg    = 0;
    PGUEST_REGS GuestRegs = VCpu->Regs;

    EcxReg         = GuestRegs->rcx & 0xffffffff;
    UINT64 Pmc     = __readpmc(EcxReg);
    GuestRegs->rax = 0x00000000ffffffff & Pmc;
    GuestRegs->rdx = 0x00000000ffffffff & (Pmc >> 32);
}

/**
 * @brief Set the timer value for preemption timer
 *
 * @param TimerValue Value of the timer
 * @return VOID
 */
VOID
CounterSetPreemptionTimer(UINT32 TimerValue)
{
    //
    // Set the time value
    //
    VmxVmwrite64(VMCS_GUEST_VMX_PREEMPTION_TIMER_VALUE, TimerValue);
}

/**
 * @brief Clears the preemption timer
 *
 * @return VOID
 */
VOID
CounterClearPreemptionTimer()
{
    //
    // Set the time value to NULL
    //
    VmxVmwrite64(VMCS_GUEST_VMX_PREEMPTION_TIMER_VALUE, NULL64_ZERO);
}
