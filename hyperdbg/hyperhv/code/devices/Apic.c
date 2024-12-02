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
 * @brief Perfom read I/O APIC
 *
 * @param IoApicBaseVa
 * @param Reg
 *
 * @return UINT64
 */
UINT64
IoApicRead(volatile IO_APIC_ENT * IoApicBaseVa, UINT32 Reg)
{
    UINT32 High = 0, Low;

    IoApicBaseVa->Reg = Reg;
    Low               = IoApicBaseVa->Data;

    if (Reg >= IOAPIC_REDTBL(0) && Reg < IOAPIC_REDTBL(IOAPIC_REDTBL_MAX))
    {
        // ASSERT(Reg % 2 == 0);
        Reg++;
        IoApicBaseVa->Reg = Reg;
        High              = IoApicBaseVa->Data;
        return IOAPIC_APPEND_QWORD(High, Low);
    }
    else
    {
        return Low;
    }
}

/**
 * @brief Perfom write to I/O APIC
 *
 * @param IoApicBaseVa
 * @param Reg
 * @param Data
 *
 * @return VOID
 */
VOID
IoApicWrite(volatile IO_APIC_ENT * IoApicBaseVa, UINT32 Reg, UINT64 Data)
{
    IoApicBaseVa->Reg  = Reg;
    IoApicBaseVa->Data = IOAPIC_LOW_DWORD(Data);

    if (Reg >= IOAPIC_REDTBL(0) && Reg < IOAPIC_REDTBL(IOAPIC_REDTBL_MAX))
    {
        // ASSERT(Reg % 2 == 0);
        Reg++;
        IoApicBaseVa->Reg  = Reg;
        IoApicBaseVa->Data = IOAPIC_HIGH_DWORD(Data);
    }
}

/**
 * @brief Dump I/O APIC
 * @param IoApicPackets
 *
 * @return VOID
 */
VOID
ApicDumpIoApic(IO_APIC_ENTRY_PACKETS * IoApicPackets)
{
    UINT32 Index = 0;
    UINT32 Max;
    UINT64 ll, lh;

    UINT64 ApicBasePa = IO_APIC_DEFAULT_BASE_ADDR;

    ll = IoApicRead(g_IoApicBase, IO_VERS_REGISTER),

    Max = (ll >> 16) & 0xff;

    //   Log("IoApic @ %08x  ID:%x (%x)  Arb:%x\n",
    //       ApicBasePa,
    //       IoApicRead(g_IoApicBase, IO_ID_REGISTER) >> 24,
    //       ll & 0xFF,
    //       IoApicRead(g_IoApicBase, IO_ARB_ID_REGISTER));

    //
    // Fill the I/O APIC
    //
    IoApicPackets->ApicBasePa = (UINT32)ApicBasePa;
    IoApicPackets->ApicBaseVa = (UINT64)g_IoApicBase;
    IoApicPackets->IoIdReg    = (UINT32)IoApicRead(g_IoApicBase, IO_ID_REGISTER);
    IoApicPackets->IoLl       = (UINT32)ll;
    IoApicPackets->IoArbIdReg = (UINT32)IoApicRead(g_IoApicBase, IO_ARB_ID_REGISTER);

    //
    // Dump inti table
    //
    Max *= 2;

    for (Index = 0; Index <= Max; Index += 2)
    {
        if (Index >= MAX_NUMBER_OF_IO_APIC_ENTRIES)
        {
            //
            // To prevent overflow of the target buffer
            //
            return;
        }

        ll = IoApicRead(g_IoApicBase, IO_REDIR_BASE + Index + 0);
        lh = IoApicRead(g_IoApicBase, IO_REDIR_BASE + Index + 1);

        IoApicPackets->LlLhData[Index]     = ll;
        IoApicPackets->LlLhData[Index + 1] = lh;
    }
}

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
 * @brief Trigger NMI on X2APIC or APIC based on Current system
 *
 * @return VOID
 */
VOID
ApicTriggerGenericSmi()
{
    LogInfo("Generating SMIs");
    if (g_CompatibilityCheck.IsX2Apic)
    {
        // X2ApicIcrWrite((4 << 8) | (1 << 14) | (3 << 18), 0);
        X2ApicIcrWrite((2 << 8) | (1 << 14) | (3 << 18), 0);
    }
    else
    {
        //    XApicIcrWrite((4 << 8) | (1 << 14) | (3 << 18), 0);
        XApicIcrWrite((2 << 8) | (1 << 14) | (3 << 18), 0);
    }
}

/**
 * @brief Store the details of APIC in xAPIC and x2APIC mode
 * @param LApicBuffer
 * @param IsUsingX2APIC
 *
 * @return BOOLEAN
 */
BOOLEAN
ApicStoreLocalApicFields(PLAPIC_PAGE LApicBuffer, PBOOLEAN IsUsingX2APIC)
{
    if (g_CompatibilityCheck.IsX2Apic)
    {
        *IsUsingX2APIC = TRUE;
        return ApicStoreLocalApicInX2ApicMode(LApicBuffer);
    }
    else
    {
        *IsUsingX2APIC = FALSE;
        return ApicStoretLocalApicInXApicMode(LApicBuffer);
    }
}

/**
 * @brief Store the details of I/O APIC
 * @param IoApicPackets
 *
 * @return BOOLEAN
 */
BOOLEAN
ApicStoreIoApicFields(IO_APIC_ENTRY_PACKETS * IoApicPackets)
{
    //
    // Dump I/O APIC Entries
    // Note that I/O APIC is not accessed through MSRs (e.g., X2APIC)
    // So, it's all about the physical memory
    //
    ApicDumpIoApic(IoApicPackets);

    //
    // There is not error defined for it at the moment
    //
    return TRUE;
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
    PHYSICAL_ADDRESS PaIoApicBase;

    ApicBaseMSR = __readmsr(IA32_APIC_BASE);

    if (!(ApicBaseMSR & (1 << 11)))
    {
        return FALSE;
    }

    //
    // Map I/O APIC default base address
    //
    // The exact APIC base address should be read from MADT table (ACPI)
    // However, we don't have an ACPI parser right now, but the address
    // is proved to stay at this (default) physical address since Intel
    // recommends OS/BIOS to not relocate it, but it could be relocated
    // however, this address is valid for almost all of the systems
    //
    PaIoApicBase.QuadPart = IO_APIC_DEFAULT_BASE_ADDR & 0xFFFFFF000;
    g_IoApicBase          = MmMapIoSpace(PaIoApicBase, 0x1000, MmNonCached);

    if (!g_IoApicBase)
    {
        //
        // Not gonna fail the initialization since the IOAPIC might be relocated by
        // either OS/BIOS
        //

        // return FALSE;
    }

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
        {
            return FALSE;
        }

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
    // Unmap Local APIC base
    //
    if (g_ApicBase)
    {
        MmUnmapIoSpace(g_ApicBase, 0x1000);
    }

    //
    // Unmap I/O APIC base
    //
    if (g_IoApicBase)
    {
        MmUnmapIoSpace(g_IoApicBase, 0x1000);
    }
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
