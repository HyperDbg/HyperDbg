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

#define LAPIC_LVT_FLAG_ENTRY_MASKED     (1UL << 16)
#define LAPIC_LVT_DELIVERY_MODE_EXT_INT (7UL << 8)
#define LAPIC_SVR_FLAG_SW_ENABLE        (1UL << 8)

/// @brief LAPIC structure size
#define LAPIC_SIZE 0x400

typedef struct _LAPIC_PAGE
{
    UINT8 __reserved_000[0x10];
    UINT8 __reserved_010[0x10];

    UINT32 Id; // offset 0x020
    UINT8  __reserved_024[0x0C];

    UINT32 Version; // offset 0x030
    UINT8  __reserved_034[0x0C];

    UINT8 __reserved_040[0x40];

    UINT32 TPR; // offset 0x080
    UINT8  __reserved_084[0x0C];

    UINT32 ArbitrationPriority; // offset 0x090
    UINT8  __reserved_094[0x0C];

    UINT32 ProcessorPriority; // offset 0x0A0
    UINT8  __reserved_0A4[0x0C];

    UINT32 EOI; // offset 0x0B0
    UINT8  __reserved_0B4[0x0C];

    UINT32 RemoteRead; // offset 0x0C0
    UINT8  __reserved_0C4[0x0C];

    UINT32 LogicalDestination; // offset 0x0D0
    UINT8  __reserved_0D4[0x0C];

    UINT32 DestinationFormat; // offset 0x0E0
    UINT8  __reserved_0E4[0x0C];

    UINT32 SpuriousInterruptVector; // offset 0x0F0
    UINT8  __reserved_0F4[0x0C];

    UINT32 ISR[32]; // offset 0x100

    UINT32 TMR[32]; // offset 0x180

    UINT32 IRR[32]; // offset 0x200

    UINT32 ErrorStatus; // offset 0x280
    UINT8  __reserved_284[0x0C];

    UINT8 __reserved_290[0x60];

    UINT32 LvtCmci; // offset 0x2F0
    UINT8  __reserved_2F4[0x0C];

    UINT32 IcrLow; // offset 0x300
    UINT8  __reserved_304[0x0C];

    UINT32 IcrHigh; // offset 0x310
    UINT8  __reserved_314[0x0C];

    UINT32 LvtTimer; // offset 0x320
    UINT8  __reserved_324[0x0C];

    UINT32 LvtThermalSensor; // offset 0x330
    UINT8  __reserved_334[0x0C];

    UINT32 LvtPerfMonCounters; // offset 0x340
    UINT8  __reserved_344[0x0C];

    UINT32 LvtLINT0; // offset 0x350
    UINT8  __reserved_354[0x0C];

    UINT32 LvtLINT1; // offset 0x360
    UINT8  __reserved_364[0x0C];

    UINT32 LvtError; // offset 0x370
    UINT8  __reserved_374[0x0C];

    UINT32 InitialCount; // offset 0x380
    UINT8  __reserved_384[0x0C];

    UINT32 CurrentCount; // offset 0x390
    UINT8  __reserved_394[0x0C];

    UINT8 __reserved_3A0[0x40]; // offset 0x3A0

    UINT32 DivideConfiguration; // offset 0x3E0
    UINT8  __reserved_3E4[0x0C];

    UINT32 SelfIpi;              // offset 0x3F0
    UINT8  __reserved_3F4[0x0C]; // valid only for X2APIC
} LAPIC_PAGE;

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
    LogInfo("Read offset at: %llx", Offset);
    return __readmsr(X2_MSR_BASE + TO_X2(Offset));
}

VOID
LapicPrintPlatformXApicMode(
    void)
{
    UINT8                   i = 0, j = 0;
    UINT32                  k   = 0;
    UINT32                  reg = 0;
    IA32_APIC_BASE_REGISTER ia32ApicBaseMsr;
    UINT64                  basePa = 0;

    volatile LAPIC_PAGE * lapicVa = g_ApicBase;

    if (!g_ApicBase)
    {
        return;
    }

    ia32ApicBaseMsr.AsUInt = __readmsr(IA32_APIC_BASE);
    basePa                 = ia32ApicBaseMsr.ApicBase;

    LogInfo("*** (xAPIC Mode) PHYSICAL LAPIC ID = %u, Ver = 0x%x, MaxLvtEntry = %d, DirectedEOI = P%d/E%d, (SW: '%s'). ApicBase=0x%x\n"
            "     -> TPR = 0x%08x,  APR = 0x%08x,  PPR = 0x%08x,  RRD = 0x%08x\n"
            "     -> LDR = 0x%08x,  DFR = 0x%08x,  SVR = 0x%08x,  Err = 0x%08x\n"
            "     -> ICR_LOW  = 0x%08x ,  ICR_HIGH = 0x%08x\n"
            "     -> LVT_INT0 = 0x%08x,  LVT_INT1 = 0x%08x\n"
            "     -> LVT_CMCI = 0x%08x,  LVT_PMCR = 0x%08x\n"
            "     -> LVT_TMR  = 0x%08x,  LVT_TSR  = 0x%08x\n"
            "     -> LVT_ERR  = 0x%08x\n"
            "     -> InitialCount = 0x%08x, CurrentCount = 0x%08x, DivideConfig = 0x%08x\n",
            lapicVa->Id >> 24,
            lapicVa->Version,
            (lapicVa->Version & 0xFF0000) >> 16,
            (lapicVa->Version >> 24) & 1,
            (lapicVa->SpuriousInterruptVector >> 12) & 1,
            (lapicVa->SpuriousInterruptVector & LAPIC_SVR_FLAG_SW_ENABLE) != 0 ? "Enabled" : "Disabled",
            ia32ApicBaseMsr,
            lapicVa->TPR,
            lapicVa->ArbitrationPriority,
            lapicVa->ProcessorPriority,
            lapicVa->RemoteRead,
            lapicVa->LogicalDestination,
            lapicVa->DestinationFormat,
            lapicVa->SpuriousInterruptVector,
            lapicVa->ErrorStatus,
            lapicVa->IcrLow,
            lapicVa->IcrHigh,
            lapicVa->LvtLINT0,
            lapicVa->LvtLINT1,
            lapicVa->LvtCmci,
            lapicVa->LvtPerfMonCounters,
            lapicVa->LvtTimer,
            lapicVa->LvtThermalSensor,
            lapicVa->LvtError,
            lapicVa->InitialCount,
            lapicVa->CurrentCount,
            lapicVa->DivideConfiguration);

    // print the ISR, TMR and IRR
    LogInfo("ISR : ");

    for (i = 0; i < 8; i++)
    {
        k   = 1;
        reg = (UINT32)lapicVa->ISR[i * 4];
        for (j = 0; j < 32; j++)
        {
            if (0 != (reg & k))
            {
                LogInfo("0x%02hhx ", (UINT8)(i * 32 + j));
            }
            k = k << 1;
        }
    }
    LogInfo("\n");

    LogInfo("TMR : ");

    for (i = 0; i < 8; i++)
    {
        k   = 1;
        reg = (UINT32)lapicVa->TMR[i * 4];
        for (j = 0; j < 32; j++)
        {
            if (reg & k)
            {
                LogInfo("0x%02hhx ", (UINT8)(i * 32 + j));
            }
            k = k << 1;
        }
    }

    LogInfo("\n");

    LogInfo("IRR : ");

    for (i = 0; i < 8; i++)
    {
        k   = 1;
        reg = (UINT32)lapicVa->IRR[i * 4];
        for (j = 0; j < 32; j++)
        {
            if (reg & k)
            {
                LogInfo("0x%02hhx ", (UINT8)(i * 32 + j));
            }
            k = k << 1;
        }
    }

    LogInfo("\n");

    return;
}

VOID
LapicPrintPlatformX2ApicMode(
    void)
{
    UINT8                   i = 0, j = 0;
    UINT32                  k   = 0;
    UINT32                  reg = 0;
    IA32_APIC_BASE_REGISTER ia32ApicBaseMsr;

    ia32ApicBaseMsr.AsUInt = __readmsr(IA32_APIC_BASE);

    LogInfo("*** (x2APIC Mode) PHYSICAL LAPIC ID = %u, Ver = 0x%x, MaxLvtEntry = %d, DirectedEOI = P%d/E%d, (SW: '%s'). ApicBase=0x%x\n"
            "     -> TPR = 0x%08x,  APR = 0x%08x,  PPR = 0x%08x,  RRD = 0x%08x\n"
            "     -> LDR = 0x%08x,  DFR = 0x%08x,  SVR = 0x%08x,  Err = 0x%08x\n"
            "     -> ICR_LOW  = 0x%08x ,  ICR_HIGH = 0x%08x\n"
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
            ia32ApicBaseMsr,
            X2ApicRead(APIC_TASK_PRIORITY),
            0, // X2ApicRead(APIC_ARBITRATION_PRIORITY),
            X2ApicRead(APIC_PROCESSOR_PRIORITY),
            0, // X2ApicRead(APIC_REMOTE_READ),
            X2ApicRead(APIC_LOGICAL_DESTINATION),
            0, //  X2ApicRead(APIC_DESTINATION_FORMAT),
            X2ApicRead(APIC_SPURIOUS_INTERRUPT_VECTOR),
            X2ApicRead(APIC_ERROR_STATUS),
            0, //  X2ApicRead(APIC_INTERRUPT_COMMAND_BITS_0_31),
            0, //  X2ApicRead(APIC_INTERRUPT_COMMAND_BITS_32_63),
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
    // print the ISR, TMR and IRR
    //

    LogInfo("ISR : ");

    for (i = 0; i < 8; i++)
    {
        k   = 1;
        reg = (UINT32)X2ApicRead(APIC_IN_SERVICE_BITS_31_0 + (0x10 * i));
        for (j = 0; j < 32; j++)
        {
            if (0 != (reg & k))
            {
                LogInfo("0x%02hhx ", (UINT8)(i * 32 + j));
            }
            k = k << 1;
        }
    }
    LogInfo("\n");

    LogInfo("TMR : ");

    for (i = 0; i < 8; i++)
    {
        k   = 1;
        reg = (UINT32)X2ApicRead(APIC_TRIGGER_MODE_BITS_31_0 + (0x10 * i));

        for (j = 0; j < 32; j++)
        {
            if (reg & k)
            {
                LogInfo("0x%02hhx ", (UINT8)(i * 32 + j));
            }
            k = k << 1;
        }
    }

    LogInfo("\n");

    LogInfo("IRR : ");

    for (i = 0; i < 8; i++)
    {
        k   = 1;
        reg = (UINT32)X2ApicRead(APIC_INTERRUPT_REQUEST_BITS_31_0 + (0x10 * i));

        for (j = 0; j < 32; j++)
        {
            if (reg & k)
            {
                LogInfo("0x%02hhx ", (UINT8)(i * 32 + j));
            }
            k = k << 1;
        }
    }

    LogInfo("\n");
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
        LapicPrintPlatformX2ApicMode();
    }
    else
    {
        LapicPrintPlatformXApicMode();
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
