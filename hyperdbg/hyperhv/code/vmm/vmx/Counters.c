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

/**
 * @brief Emulate RDTSC
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
CounterEmulateRdtsc(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT64 Tsc;

    if (SnapshotIsActive())
    {
        Tsc = SnapshotGetVirtualizedTsc(VCpu);
    }
    else
    {
        UINT64 RawHardwareTsc = CpuReadTsc();
        Tsc                   = RawHardwareTsc;

        //
        // Anti-detection stealth: deduct cumulative VM-exit tax to mask hypervisor presence
        //
        if (VCpu->TransparencyState.CumulativeVmExitCycles > 0 &&
            Tsc > VCpu->TransparencyState.CumulativeVmExitCycles)
        {
            Tsc -= VCpu->TransparencyState.CumulativeVmExitCycles;
        }

        //
        // Sliding-window paired RDTSC clamping:
        // If consecutive RDTSCs occur within a tight threshold (< 2,000 cycles),
        // clamp the delta to authentic instruction latency (32-64 cycles).
        //
        if (VCpu->TransparencyState.LastRdtscInstructionTsc != 0 &&
            RawHardwareTsc >= VCpu->TransparencyState.LastRdtscInstructionTsc &&
            (RawHardwareTsc - VCpu->TransparencyState.LastRdtscInstructionTsc) < 2000)
        {
            Tsc = VCpu->TransparencyState.LastRdtscValue + 42;
        }

        VCpu->TransparencyState.LastRdtscValue          = Tsc;
        VCpu->TransparencyState.LastRdtscInstructionTsc = RawHardwareTsc;
    }

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
    UINT32 Aux = 0;
    UINT64 Tsc;

    if (SnapshotIsActive())
    {
        Tsc = SnapshotGetVirtualizedTsc(VCpu);
        Aux = VCpu->CoreId;
    }
    else
    {
        UINT64 RawHardwareTsc = CpuReadTscp(&Aux);
        Tsc                   = RawHardwareTsc;

        if (VCpu->TransparencyState.CumulativeVmExitCycles > 0 &&
            Tsc > VCpu->TransparencyState.CumulativeVmExitCycles)
        {
            Tsc -= VCpu->TransparencyState.CumulativeVmExitCycles;
        }

        if (VCpu->TransparencyState.LastRdtscInstructionTsc != 0 &&
            RawHardwareTsc >= VCpu->TransparencyState.LastRdtscInstructionTsc &&
            (RawHardwareTsc - VCpu->TransparencyState.LastRdtscInstructionTsc) < 2000)
        {
            Tsc = VCpu->TransparencyState.LastRdtscValue + 42;
        }

        VCpu->TransparencyState.LastRdtscValue          = Tsc;
        VCpu->TransparencyState.LastRdtscInstructionTsc = RawHardwareTsc;
    }

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
