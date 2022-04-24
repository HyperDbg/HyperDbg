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
#include "..\hprdbghv\pch.h"

/**
 * @brief Emulate RDTSC
 *
 * @param GuestRegs guest registers
 * @return VOID
 */
VOID
CounterEmulateRdtsc(PGUEST_REGS GuestRegs)
{
    //
    // I realized that if you log anything here (LogInfo) then
    // the system-halts, currently don't have any idea of how
    // to solve it, in the future we solve it using tsc offsectiong
    // or tsc scalling (The reason is because of that fucking patchguard :( )
    //
    UINT64 Tsc     = __rdtsc();
    GuestRegs->rax = 0x00000000ffffffff & Tsc;
    GuestRegs->rdx = 0x00000000ffffffff & (Tsc >> 32);
}

/**
 * @brief Emulate RDTSCP
 *
 * @param GuestRegs Guest Registers
 * @return VOID
 */
VOID
CounterEmulateRdtscp(PGUEST_REGS GuestRegs)
{
    int    Aux     = 0;
    UINT64 Tsc     = __rdtscp(&Aux);
    GuestRegs->rax = 0x00000000ffffffff & Tsc;
    GuestRegs->rdx = 0x00000000ffffffff & (Tsc >> 32);

    GuestRegs->rcx = 0x00000000ffffffff & Aux;
}

/**
 * @brief Emulate RDPMC
 *
 * @param GuestRegs Guest Register
 * @return VOID
 */
VOID
CounterEmulateRdpmc(PGUEST_REGS GuestRegs)
{
    ULONG EcxReg = 0;

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
    __vmx_vmwrite(VMCS_GUEST_VMX_PREEMPTION_TIMER_VALUE, TimerValue);
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
    __vmx_vmwrite(VMCS_GUEST_VMX_PREEMPTION_TIMER_VALUE, NULL);
}
