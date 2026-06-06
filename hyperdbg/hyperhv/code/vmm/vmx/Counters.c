/**
 * @file Counters.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @author jtaw5649
 * @brief The functions for emulating counters
 * @details
 * @version 0.1
 * @date 2020-06-14
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#define MAX_TRANSPARENT_CPUID_TSC_COMPENSATION      0x100000
#define TRANSPARENT_CPUID_TSC_RIP_WINDOW            0x100
#define TRANSPARENT_CPUID_SYNTHETIC_TSC_BASE_COST   0x100
#define TRANSPARENT_CPUID_SYNTHETIC_TSC_JITTER_MASK 0xff
#define TRANSPARENT_TSC_SMOOTHING_DELTA_MAX         0x100000
#define TRANSPARENT_TSC_SYNTHETIC_BASE_COST         0x40
#define TRANSPARENT_TSC_SYNTHETIC_JITTER_MASK       0x1f

static VOID
CounterSetGuestTsc(VIRTUAL_MACHINE_STATE * VCpu, UINT64 Tsc)
{
    PGUEST_REGS GuestRegs = VCpu->Regs;

    GuestRegs->rax = 0x00000000ffffffff & Tsc;
    GuestRegs->rdx = 0x00000000ffffffff & (Tsc >> 32);
}

static VOID
CounterRecordGuestTsc(VIRTUAL_MACHINE_STATE * VCpu, UINT64 Tsc)
{
    if (g_CheckForFootprints && g_TransparentCpuidTscCompensationEnabled)
    {
        if (VCpu->TransparencyState.CpuidTscCompensationArmed)
        {
            return;
        }

        VCpu->TransparencyState.LastGuestTimeStampCounter      = Tsc;
        VCpu->TransparencyState.LastGuestTimeStampCounterValid = TRUE;
    }
}

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

static BOOLEAN
CounterTransparentTimingActive(VIRTUAL_MACHINE_STATE * VCpu)
{
    return g_CheckForFootprints &&
           g_TransparentCpuidTscCompensationEnabled &&
           VCpu->TransparencyState.TransparentCpuidTscTimingEnabled;
}

static UINT64
CounterSyntheticCpuidTscCost(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT64 Seed = VCpu->TransparencyState.LastGuestTimeStampCounterValid ? VCpu->TransparencyState.LastGuestTimeStampCounter : VCpu->TransparencyState.CpuidTscEntryTsc;

    return TRANSPARENT_CPUID_SYNTHETIC_TSC_BASE_COST +
           ((Seed ^ (Seed >> 11) ^ VCpu->CoreId) & TRANSPARENT_CPUID_SYNTHETIC_TSC_JITTER_MASK);
}

static UINT64
CounterSyntheticTransparentTscCost(VIRTUAL_MACHINE_STATE * VCpu, UINT64 HostTsc)
{
    return TRANSPARENT_TSC_SYNTHETIC_BASE_COST +
           ((HostTsc ^ (HostTsc >> 9) ^ VCpu->CoreId) & TRANSPARENT_TSC_SYNTHETIC_JITTER_MASK);
}

static BOOLEAN
CounterIsMatchingCpuidTscConsumer(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT64 EntryRip = VCpu->TransparencyState.CpuidTscEntryRip;

    if (VCpu->LastVmexitRip <= EntryRip)
    {
        return FALSE;
    }

    return VCpu->LastVmexitRip - EntryRip <= TRANSPARENT_CPUID_TSC_RIP_WINDOW;
}

static UINT64
CounterApplyCpuidTscCompensation(VIRTUAL_MACHINE_STATE * VCpu, UINT64 Tsc)
{
    UINT64 BaseTsc = VCpu->TransparencyState.LastRevealedTimeStampCounter;
    UINT64 TargetTsc;

    if (VCpu->TransparencyState.LastGuestTimeStampCounterValid)
    {
        BaseTsc   = VCpu->TransparencyState.LastGuestTimeStampCounter;
        TargetTsc = BaseTsc + CounterSyntheticCpuidTscCost(VCpu);

        if (TargetTsc > Tsc)
        {
            TargetTsc = Tsc;
        }
    }
    else
    {
        if (BaseTsc == 0)
        {
            BaseTsc = VCpu->TransparencyState.CpuidTscEntryTsc;
        }

        TargetTsc = BaseTsc + CounterSyntheticCpuidTscCost(VCpu);

        if (Tsc > TargetTsc && Tsc - TargetTsc > MAX_TRANSPARENT_CPUID_TSC_COMPENSATION)
        {
            TargetTsc = Tsc - MAX_TRANSPARENT_CPUID_TSC_COMPENSATION;
        }

        if (TargetTsc < VCpu->TransparencyState.LastRevealedTimeStampCounter)
        {
            TargetTsc = VCpu->TransparencyState.LastRevealedTimeStampCounter;
        }

        if (TargetTsc > Tsc)
        {
            TargetTsc = Tsc;
        }
    }

    VCpu->TransparencyState.LastRevealedTimeStampCounter   = TargetTsc;
    VCpu->TransparencyState.LastGuestTimeStampCounter      = TargetTsc;
    VCpu->TransparencyState.LastGuestTimeStampCounterValid = TRUE;
    VCpu->TransparencyState.LastTransparentTscHostCounter  = Tsc;
    VCpu->TransparencyState.LastTransparentTscGuestCounter = TargetTsc;
    VCpu->TransparencyState.LastTransparentTscValid        = TRUE;

    return TargetTsc;
}

static UINT64
CounterApplyTransparentTscSmoothing(VIRTUAL_MACHINE_STATE * VCpu, UINT64 HostTsc)
{
    UINT64 HostDelta;
    UINT64 TargetTsc;

    if (!CounterTransparentTimingActive(VCpu))
    {
        return HostTsc;
    }

    if (!VCpu->TransparencyState.LastTransparentTscValid ||
        HostTsc < VCpu->TransparencyState.LastTransparentTscHostCounter)
    {
        VCpu->TransparencyState.LastTransparentTscHostCounter  = HostTsc;
        VCpu->TransparencyState.LastTransparentTscGuestCounter = HostTsc;
        VCpu->TransparencyState.LastTransparentTscValid        = TRUE;
        return HostTsc;
    }

    HostDelta = HostTsc - VCpu->TransparencyState.LastTransparentTscHostCounter;
    if (HostDelta > TRANSPARENT_TSC_SMOOTHING_DELTA_MAX)
    {
        TargetTsc = HostTsc;
    }
    else
    {
        TargetTsc = VCpu->TransparencyState.LastTransparentTscGuestCounter +
                    CounterSyntheticTransparentTscCost(VCpu, HostTsc);
    }

    if (TargetTsc < VCpu->TransparencyState.LastRevealedTimeStampCounter)
    {
        TargetTsc = VCpu->TransparencyState.LastRevealedTimeStampCounter;
    }

    if (TargetTsc > HostTsc)
    {
        TargetTsc = HostTsc;
    }

    VCpu->TransparencyState.LastTransparentTscHostCounter  = HostTsc;
    VCpu->TransparencyState.LastTransparentTscGuestCounter = TargetTsc;
    VCpu->TransparencyState.LastTransparentTscValid        = TRUE;

    return TargetTsc;
}

VOID
CounterArmCpuidTscCompensation(VIRTUAL_MACHINE_STATE * VCpu, UINT32 Leaf, UINT32 Subleaf)
{
    CounterClearCpuidTscCompensation(VCpu);

    VCpu->TransparencyState.CpuidTscEntryTsc          = VCpu->TransparencyState.LastVmexitTimeStampCounter;
    VCpu->TransparencyState.CpuidTscEntryRip          = VCpu->LastVmexitRip;
    VCpu->TransparencyState.CpuidTscEntryRsp          = VCpu->Regs->rsp;
    VCpu->TransparencyState.CpuidTscEntryLeaf         = Leaf;
    VCpu->TransparencyState.CpuidTscEntrySubleaf      = Subleaf;
    VCpu->TransparencyState.CpuidTscCompensationArmed = TRUE;
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

BOOLEAN
CounterEmulateCpuidTscCompensation(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN IsRdtscp)
{
    UINT32 Aux = 0;
    UINT64 Tsc;

    if (!VCpu->TransparencyState.CpuidTscCompensationArmed)
    {
        return FALSE;
    }

    if (!CounterIsMatchingCpuidTscConsumer(VCpu))
    {
        CounterClearCpuidTscCompensation(VCpu);
        return FALSE;
    }

    if (IsRdtscp)
    {
        Tsc = __rdtscp(&Aux);
    }
    else
    {
        Tsc = __rdtsc();
    }

    CounterSetGuestTsc(VCpu, CounterApplyCpuidTscCompensation(VCpu, Tsc));

    if (IsRdtscp)
    {
        VCpu->Regs->rcx = 0x00000000ffffffff & Aux;
    }

    CounterClearCpuidTscCompensation(VCpu);
    return TRUE;
}

BOOLEAN
CounterHandleTransparentRdpmcGeneralProtection(VIRTUAL_MACHINE_STATE * VCpu)
{
    BYTE GuestInstruction[2] = {0};

    if (!CounterTransparentTimingActive(VCpu))
    {
        return FALSE;
    }

    if (!MemoryMapperReadMemorySafeOnTargetProcess(VCpu->LastVmexitRip,
                                                   GuestInstruction,
                                                   sizeof(GuestInstruction)))
    {
        return FALSE;
    }

    if (GuestInstruction[0] != 0x0f || GuestInstruction[1] != 0x33)
    {
        return FALSE;
    }

    EventInjectGeneralProtection();
    HvSuppressRipIncrement(VCpu);

    return TRUE;
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
    UINT64 Tsc = CounterApplyTransparentTscSmoothing(VCpu, CpuReadTsc());

    CounterSetGuestTsc(VCpu, Tsc);
    VCpu->TransparencyState.LastRevealedTimeStampCounter = Tsc;
    CounterRecordGuestTsc(VCpu, Tsc);
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
    UINT32 Aux = 0;
    UINT64 Tsc = CounterApplyTransparentTscSmoothing(VCpu, CpuReadTscp(&Aux));

    CounterSetGuestTsc(VCpu, Tsc);
    VCpu->TransparencyState.LastRevealedTimeStampCounter = Tsc;
    CounterRecordGuestTsc(VCpu, Tsc);

    VCpu->Regs->rcx = 0x00000000ffffffff & Aux;
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
    UINT32               EcxReg   = 0;
    UINT64               GuestCr4 = 0;
    UINT64               Pmc      = 0;
    VMX_SEGMENT_SELECTOR Cs;
    PGUEST_REGS          GuestRegs = VCpu->Regs;

    Cs = GetGuestCs();
    VmxVmread64P(VMCS_GUEST_CR4, &GuestCr4);
    if (Cs.Attributes.DescriptorPrivilegeLevel != DPL_SYSTEM && (GuestCr4 & REG_CR4_PCE) == 0)
    {
        EventInjectGeneralProtection();
        HvSuppressRipIncrement(VCpu);
        return;
    }

    EcxReg = GuestRegs->rcx & 0xffffffff;
    __try
    {
        Pmc = __readpmc(EcxReg);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        EventInjectGeneralProtection();
        HvSuppressRipIncrement(VCpu);
        return;
    }

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
