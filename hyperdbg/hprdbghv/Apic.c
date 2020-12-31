/**
 * @file Apic.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Routines for Advanced Programmable Interrupt Controller (APIC)
 * @details The code is derived from (https://www.cpl0.com/blog/?p=46)
 * @version 0.1
 * @date 2020-12-31
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Trigger NMI on XAPIC
 * 
 * @return VOID 
 */
VOID
ApicXTriggerNmi(UINT32 Low, UINT32 High)
{
    *(UINT32 *)((uintptr_t)g_ApicBase + ICROffset + 0x10) = High;
    *(UINT32 *)((uintptr_t)g_ApicBase + ICROffset)        = Low;
}

/**
 * @brief Trigger NMI on X2APIC
 * 
 * @return VOID 
 */
VOID
ApicX2TriggerNmi(UINT32 Low, UINT32 High)
{
    __writemsr(X2_MSR_BASE + TO_X2(ICROffset), ((UINT64)High << 32) | Low);
}

/**
 * @brief Trigger NMI on X2APIC or APIC based on Current system
 * 
 * @return VOID 
 */
VOID
ApicTriggerGenericNmi()
{
    if (g_IsX2Apic)
    {
        ApicX2TriggerNmi((4 << 8) | (1 << 14) | (3 << 18), 0);
    }
    else
    {
        ApicXTriggerNmi((4 << 8) | (1 << 14) | (3 << 18), 0);
    }
}

/**
 * @brief Initialize APIC
 * 
 * @return UINT32 
 */
UINT32
ApicInitialize()
{
    UINT64           ApicBaseMSR;
    PHYSICAL_ADDRESS PaApicBase;

    ApicBaseMSR = __readmsr(0x1B);
    if (!(ApicBaseMSR & (1 << 11)))
        return STATUS_FAILED_DRIVER_ENTRY;
    if (ApicBaseMSR & (1 << 10))
    {
        g_IsX2Apic = TRUE;
        return STATUS_FAILED_DRIVER_ENTRY;
    }
    else
    {
        PaApicBase.QuadPart = ApicBaseMSR & 0xFFFFFF000;
        g_ApicBase          = MmMapIoSpace(PaApicBase, 0x1000, MmNonCached);
        if (!g_ApicBase)
            return STATUS_FAILED_DRIVER_ENTRY;
        g_IsX2Apic = FALSE;
    }
    return STATUS_SUCCESS;
}
