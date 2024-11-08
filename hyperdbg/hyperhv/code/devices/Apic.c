/**
 * @file Apic.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Routines for Advanced Programmable Interrupt Controller (APIC)
 * @details The code is derived from (https://www.cpl0.com/blog/?p=46) and the code
 * for showing the APIC details in the XAPIC is derived from bitdefender/napoca project
 * which is enhanced to support X2APIC mode in HyperDbg
 *
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
 * @brief Read x2APIC mode
 * @param Offset
 *
 * @return UINT64
 */
UINT64
X2ApicRead(UINT32 Offset)
{
    return __readmsr(X2_MSR_BASE + TO_X2(Offset));
}

/**
 * @brief Store the local APIC in XAPIC mode
 *
 * @return BOOLEAN
 */
BOOLEAN
ApicStoretLocalApicInXApicMode(PLAPIC_PAGE LApicBuffer)
{
    volatile LAPIC_PAGE * LocalApicVa = g_ApicBase;

    //
    // Check if the base virtual address of local APIC is valid or not
    //
    if (!g_ApicBase)
    {
        return FALSE;
    }

    //
    // Store fields
    //
    LApicBuffer->Id                      = LocalApicVa->Id;
    LApicBuffer->Version                 = LocalApicVa->Version;
    LApicBuffer->SpuriousInterruptVector = LocalApicVa->SpuriousInterruptVector;
    LApicBuffer->TPR                     = LocalApicVa->TPR;
    LApicBuffer->ProcessorPriority       = LocalApicVa->ProcessorPriority;
    LApicBuffer->LogicalDestination      = LocalApicVa->LogicalDestination;
    LApicBuffer->ErrorStatus             = LocalApicVa->ErrorStatus;
    LApicBuffer->LvtLINT0                = LocalApicVa->LvtLINT0;
    LApicBuffer->LvtLINT1                = LocalApicVa->LvtLINT1;
    LApicBuffer->LvtCmci                 = LocalApicVa->LvtCmci;
    LApicBuffer->LvtPerfMonCounters      = LocalApicVa->LvtPerfMonCounters;
    LApicBuffer->LvtTimer                = LocalApicVa->LvtTimer;
    LApicBuffer->LvtThermalSensor        = LocalApicVa->LvtThermalSensor;
    LApicBuffer->LvtError                = LocalApicVa->LvtError;
    LApicBuffer->InitialCount            = LocalApicVa->InitialCount;
    LApicBuffer->CurrentCount            = LocalApicVa->CurrentCount;
    LApicBuffer->DivideConfiguration     = LocalApicVa->DivideConfiguration;

    //
    // Save the ISR, TMR and IRR
    //
    for (UINT32 i = 0; i < 8; i++)
    {
        LApicBuffer->ISR[i * 4] = LocalApicVa->ISR[i * 4];
    }

    for (UINT32 i = 0; i < 8; i++)
    {
        LApicBuffer->TMR[i * 4] = LocalApicVa->TMR[i * 4];
    }

    for (UINT32 i = 0; i < 8; i++)
    {
        LApicBuffer->IRR[i * 4] = LocalApicVa->IRR[i * 4];
    }

    return TRUE;
}

/**
 * @brief Store the local APIC in X2APIC mode
 *
 * @return BOOLEAN
 */
BOOLEAN
ApicStoreLocalApicInX2ApicMode(PLAPIC_PAGE LApicBuffer)
{
    //
    // Store fields
    //
    LApicBuffer->Id                      = (UINT32)X2ApicRead(APIC_ID);
    LApicBuffer->Version                 = (UINT32)X2ApicRead(APIC_VERSION);
    LApicBuffer->SpuriousInterruptVector = (UINT32)X2ApicRead(APIC_SPURIOUS_INTERRUPT_VECTOR);
    LApicBuffer->TPR                     = (UINT32)X2ApicRead(APIC_TASK_PRIORITY);
    LApicBuffer->ProcessorPriority       = (UINT32)X2ApicRead(APIC_PROCESSOR_PRIORITY);
    LApicBuffer->LogicalDestination      = (UINT32)X2ApicRead(APIC_LOGICAL_DESTINATION);
    LApicBuffer->ErrorStatus             = (UINT32)X2ApicRead(APIC_ERROR_STATUS);
    LApicBuffer->LvtLINT0                = (UINT32)X2ApicRead(APIC_LVT_LINT0);
    LApicBuffer->LvtLINT1                = (UINT32)X2ApicRead(APIC_LVT_LINT1);
    LApicBuffer->LvtCmci                 = (UINT32)X2ApicRead(APIC_LVT_CORRECTED_MACHINE_CHECK_INTERRUPT);
    LApicBuffer->LvtPerfMonCounters      = (UINT32)X2ApicRead(APIC_LVT_PERFORMANCE_MONITORING_COUNTERS);
    LApicBuffer->LvtTimer                = (UINT32)X2ApicRead(APIC_LVT_TIMER);
    LApicBuffer->LvtThermalSensor        = (UINT32)X2ApicRead(APIC_LVT_THERMAL_SENSOR);
    LApicBuffer->LvtError                = (UINT32)X2ApicRead(APIC_LVT_ERROR);
    LApicBuffer->InitialCount            = (UINT32)X2ApicRead(APIC_INITIAL_COUNT);
    LApicBuffer->CurrentCount            = (UINT32)X2ApicRead(APIC_CURRENT_COUNT);
    LApicBuffer->DivideConfiguration     = (UINT32)X2ApicRead(APIC_DIVIDE_CONFIGURATION);

    //
    // Save the ISR, TMR and IRR
    //
    for (UINT32 i = 0; i < 8; i++)
    {
        LApicBuffer->ISR[i * 4] = (UINT32)X2ApicRead(APIC_IN_SERVICE_BITS_31_0 + (0x10 * i));
    }

    for (UINT32 i = 0; i < 8; i++)
    {
        LApicBuffer->TMR[i * 4] = (UINT32)X2ApicRead(APIC_TRIGGER_MODE_BITS_31_0 + (0x10 * i));
    }

    for (UINT32 i = 0; i < 8; i++)
    {
        LApicBuffer->IRR[i * 4] = (UINT32)X2ApicRead(APIC_INTERRUPT_REQUEST_BITS_31_0 + (0x10 * i));
    }

    return TRUE;
}

/**
 * @brief Trigger NMI on X2APIC or APIC based on Current system
 *
 * @return VOID
 */
VOID
ApicTriggerGenericNmi()
{
    if (g_CompatibilityCheck.IsX2Apic)
    {
        X2ApicIcrWrite((4 << 8) | (1 << 14) | (3 << 18), 0);
    }
    else
    {
        XApicIcrWrite((4 << 8) | (1 << 14) | (3 << 18), 0);
    }
}

/**
 * @brief Stire the details of APIC in xAPIC and x2APIC mode
 * @param LApicBuffer
 *
 * @return BOOLEAN
 */
BOOLEAN
ApicStoreLocalApicFields(PLAPIC_PAGE LApicBuffer)
{
    if (g_CompatibilityCheck.IsX2Apic)
    {
        return ApicStoreLocalApicInX2ApicMode(LApicBuffer);
    }
    else
    {
        return ApicStoretLocalApicInXApicMode(LApicBuffer);
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

    ApicBaseMSR = __readmsr(IA32_APIC_BASE);
    if (!(ApicBaseMSR & (1 << 11)))
        return FALSE;

    if (ApicBaseMSR & (1 << 10))
    {
        g_CompatibilityCheck.IsX2Apic = TRUE;

        return FALSE;
    }
    else
    {
        PaApicBase.QuadPart = ApicBaseMSR & 0xFFFFFF000;
        g_ApicBase          = MmMapIoSpace(PaApicBase, 0x1000, MmNonCached);

        if (!g_ApicBase)
            return FALSE;

        g_CompatibilityCheck.IsX2Apic = FALSE;
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
    if (g_CompatibilityCheck.IsX2Apic)
    {
        X2ApicIcrWrite(APIC_DEST_SELF | APIC_DEST_PHYSICAL | APIC_DM_FIXED | Vector, 0);
    }
    else
    {
        XApicIcrWrite(APIC_DEST_SELF | APIC_DEST_PHYSICAL | APIC_DM_FIXED | Vector, 0);
    }
}
