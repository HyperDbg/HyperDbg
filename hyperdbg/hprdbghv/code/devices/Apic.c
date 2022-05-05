/**
 * @file Apic.h
 * @author Sina Karvandi (sina@hyperdbg.org)
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
 * @param Low
 * @param High
 * 
 * @return VOID 
 */
VOID
XApicIcrWrite(UINT32 Low, UINT32 High)
{
    *(UINT32 *)((uintptr_t)g_ApicBase + ICROffset + 0x10) = High;
    *(UINT32 *)((uintptr_t)g_ApicBase + ICROffset)        = Low;
}

/**
 * @brief Trigger NMI on X2APIC
 * @param Low
 * @param High
 * 
 * @return VOID 
 */
VOID
X2ApicIcrWrite(UINT32 Low, UINT32 High)
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
        X2ApicIcrWrite((4 << 8) | (1 << 14) | (3 << 18), 0);
    }
    else
    {
        XApicIcrWrite((4 << 8) | (1 << 14) | (3 << 18), 0);
    }
}

/**
 * @brief Initialize APIC
 * 
 * @return BOOLEAN 
 */
BOOLEAN
ApicInitialize()
{
    UINT64           ApicBaseMSR;
    PHYSICAL_ADDRESS PaApicBase;

    ApicBaseMSR = __readmsr(0x1B);
    if (!(ApicBaseMSR & (1 << 11)))
        return FALSE;

    if (ApicBaseMSR & (1 << 10))
    {
        g_IsX2Apic = TRUE;
        return FALSE;
    }
    else
    {
        PaApicBase.QuadPart = ApicBaseMSR & 0xFFFFFF000;
        g_ApicBase          = MmMapIoSpace(PaApicBase, 0x1000, MmNonCached);

        if (!g_ApicBase)
            return FALSE;

        g_IsX2Apic = FALSE;
    }
    return TRUE;
}

/**
 * @brief Uninitialize APIC
 * 
 * @return VOID 
 */
VOID
ApicUninitialize()
{
    //
    // Unmap I/O Base
    //
    if (g_ApicBase)
        MmUnmapIoSpace(g_ApicBase, 0x1000);
}

/**
 * @brief Self IPI the current core
 * 
 * @param Vector
 * @return VOID 
 */
VOID
ApicSelfIpi(UINT32 Vector)
{
    //
    // Check and apply self-IPI to x2APIC and xAPIC
    //
    if (g_IsX2Apic)
    {
        X2ApicIcrWrite(APIC_DEST_SELF | APIC_DEST_PHYSICAL | APIC_DM_FIXED | Vector, 0);
    }
    else
    {
        XApicIcrWrite(APIC_DEST_SELF | APIC_DEST_PHYSICAL | APIC_DM_FIXED | Vector, 0);
    }
}
