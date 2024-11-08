/**
 * @file Apic.h
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
 * @brief Print local APIC in XAPIC mode
 *
 * @return VOID
 */
VOID
ApicPrintLocalApicInXApicMode()
{
    UINT8                   i = 0, j = 0;
    UINT32                  k   = 0;
    UINT32                  Reg = 0;
    IA32_APIC_BASE_REGISTER Ia32ApicBase;
    UINT64                  BasePa = 0;

    volatile LAPIC_PAGE * LocalApicVa = g_ApicBase;

    //
    // Check if the base virtual address of local APIC is valid or not
    //
    if (!g_ApicBase)
    {
        return;
    }

    Ia32ApicBase.AsUInt = __readmsr(IA32_APIC_BASE);
    BasePa              = Ia32ApicBase.ApicBase;

    Log("*** (x2APIC Mode) PHYSICAL LAPIC ID = %u, Ver = 0x%x, MaxLvtEntry = %d, DirectedEOI = P%d/E%d, (SW: '%s'). ApicBase=0x%x\n"
        "     -> TPR = 0x%08x,  PPR = 0x%08x\n"
        "     -> LDR = 0x%08x,  SVR = 0x%08x,  Err = 0x%08x\n"
        "     -> LVT_INT0 = 0x%08x,  LVT_INT1 = 0x%08x\n"
        "     -> LVT_CMCI = 0x%08x,  LVT_PMCR = 0x%08x\n"
        "     -> LVT_TMR  = 0x%08x,  LVT_TSR  = 0x%08x\n"
        "     -> LVT_ERR  = 0x%08x\n"
        "     -> InitialCount = 0x%08x, CurrentCount = 0x%08x, DivideConfig = 0x%08x\n",
        LocalApicVa->Id >> 24,
        LocalApicVa->Version,
        (LocalApicVa->Version & 0xFF0000) >> 16,
        (LocalApicVa->Version >> 24) & 1,
        (LocalApicVa->SpuriousInterruptVector >> 12) & 1,
        (LocalApicVa->SpuriousInterruptVector & LAPIC_SVR_FLAG_SW_ENABLE) != 0 ? "Enabled" : "Disabled",
        Ia32ApicBase,
        LocalApicVa->TPR,
        LocalApicVa->ProcessorPriority,
        LocalApicVa->LogicalDestination,
        LocalApicVa->SpuriousInterruptVector,
        LocalApicVa->ErrorStatus,
        LocalApicVa->LvtLINT0,
        LocalApicVa->LvtLINT1,
        LocalApicVa->LvtCmci,
        LocalApicVa->LvtPerfMonCounters,
        LocalApicVa->LvtTimer,
        LocalApicVa->LvtThermalSensor,
        LocalApicVa->LvtError,
        LocalApicVa->InitialCount,
        LocalApicVa->CurrentCount,
        LocalApicVa->DivideConfiguration);

    //
    // Print the ISR, TMR and IRR
    //
    Log("ISR : ");

    for (i = 0; i < 8; i++)
    {
        k   = 1;
        Reg = (UINT32)LocalApicVa->ISR[i * 4];
        for (j = 0; j < 32; j++)
        {
            if (0 != (Reg & k))
            {
                Log("0x%02hhx ", (UINT8)(i * 32 + j));
            }
            k = k << 1;
        }
    }
    Log("\n");

    Log("TMR : ");

    for (i = 0; i < 8; i++)
    {
        k   = 1;
        Reg = (UINT32)LocalApicVa->TMR[i * 4];
        for (j = 0; j < 32; j++)
        {
            if (Reg & k)
            {
                Log("0x%02hhx ", (UINT8)(i * 32 + j));
            }
            k = k << 1;
        }
    }

    Log("\n");

    Log("IRR : ");

    for (i = 0; i < 8; i++)
    {
        k   = 1;
        Reg = (UINT32)LocalApicVa->IRR[i * 4];
        for (j = 0; j < 32; j++)
        {
            if (Reg & k)
            {
                Log("0x%02hhx ", (UINT8)(i * 32 + j));
            }
            k = k << 1;
        }
    }

    Log("\n");

    return;
}

/**
 * @brief Print local APIC in X2APIC mode
 *
 * @return VOID
 */
VOID
ApicPrintLocalApicInX2ApicMode()
{
    UINT8                   i = 0, j = 0;
    UINT32                  k   = 0;
    UINT32                  Reg = 0;
    IA32_APIC_BASE_REGISTER Ia32ApicBase;

    Ia32ApicBase.AsUInt = __readmsr(IA32_APIC_BASE);

    Log("*** (x2APIC Mode) PHYSICAL LAPIC ID = %u, Ver = 0x%x, MaxLvtEntry = %d, DirectedEOI = P%d/E%d, (SW: '%s'). ApicBase=0x%x\n"
        "     -> TPR = 0x%08x,  PPR = 0x%08x\n"
        "     -> LDR = 0x%08x,  SVR = 0x%08x,  Err = 0x%08x\n"
        "     -> LVT_INT0 = 0x%08x,  LVT_INT1 = 0x%08x\n"
        "     -> LVT_CMCI = 0x%08x,  LVT_PMCR = 0x%08x\n"
        "     -> LVT_TMR  = 0x%08x,  LVT_TSR  = 0x%08x\n"
        "     -> LVT_ERR  = 0x%08x\n"
        "     -> InitialCount = 0x%08x, CurrentCount = 0x%08x, DivideConfig = 0x%08x\n",
        X2ApicRead(APIC_ID) >> 24,
        X2ApicRead(APIC_VERSION),
        (X2ApicRead(APIC_VERSION) & 0xFF0000) >> 16,
        (X2ApicRead(APIC_VERSION) >> 24) & 1,
        (X2ApicRead(APIC_SPURIOUS_INTERRUPT_VECTOR) >> 12) & 1,
        (X2ApicRead(APIC_SPURIOUS_INTERRUPT_VECTOR) & LAPIC_SVR_FLAG_SW_ENABLE) != 0 ? "Enabled" : "Disabled",
        Ia32ApicBase,
        X2ApicRead(APIC_TASK_PRIORITY),
        X2ApicRead(APIC_PROCESSOR_PRIORITY),
        X2ApicRead(APIC_LOGICAL_DESTINATION),
        X2ApicRead(APIC_SPURIOUS_INTERRUPT_VECTOR),
        X2ApicRead(APIC_ERROR_STATUS),
        X2ApicRead(APIC_LVT_LINT0),
        X2ApicRead(APIC_LVT_LINT1),
        X2ApicRead(APIC_LVT_CORRECTED_MACHINE_CHECK_INTERRUPT),
        X2ApicRead(APIC_LVT_PERFORMANCE_MONITORING_COUNTERS),
        X2ApicRead(APIC_LVT_TIMER),
        X2ApicRead(APIC_LVT_THERMAL_SENSOR),
        X2ApicRead(APIC_LVT_ERROR),
        X2ApicRead(APIC_INITIAL_COUNT),
        X2ApicRead(APIC_CURRENT_COUNT),
        X2ApicRead(APIC_DIVIDE_CONFIGURATION));

    //
    // Print the ISR, TMR and IRR
    //
    Log("ISR : ");

    for (i = 0; i < 8; i++)
    {
        k   = 1;
        Reg = (UINT32)X2ApicRead(APIC_IN_SERVICE_BITS_31_0 + (0x10 * i));
        for (j = 0; j < 32; j++)
        {
            if (0 != (Reg & k))
            {
                Log("0x%02hhx ", (UINT8)(i * 32 + j));
            }
            k = k << 1;
        }
    }
    Log("\n");

    Log("TMR : ");

    for (i = 0; i < 8; i++)
    {
        k   = 1;
        Reg = (UINT32)X2ApicRead(APIC_TRIGGER_MODE_BITS_31_0 + (0x10 * i));

        for (j = 0; j < 32; j++)
        {
            if (Reg & k)
            {
                Log("0x%02hhx ", (UINT8)(i * 32 + j));
            }
            k = k << 1;
        }
    }

    Log("\n");

    Log("IRR : ");

    for (i = 0; i < 8; i++)
    {
        k   = 1;
        Reg = (UINT32)X2ApicRead(APIC_INTERRUPT_REQUEST_BITS_31_0 + (0x10 * i));

        for (j = 0; j < 32; j++)
        {
            if (Reg & k)
            {
                Log("0x%02hhx ", (UINT8)(i * 32 + j));
            }
            k = k << 1;
        }
    }

    Log("\n");
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
 * @brief Show the details of APIC in xAPIC and x2APIC mode
 *
 * @return VOID
 */
VOID
ApicShowDetails()
{
    if (g_CompatibilityCheck.IsX2Apic)
    {
        ApicPrintLocalApicInX2ApicMode();
    }
    else
    {
        ApicPrintLocalApicInXApicMode();
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
