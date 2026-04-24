/**
 * @file Lbr.c
 * @author Hari Mishal (harimishal6@gmail.com)
 * @brief Message logging and tracing implementation
 * @details Modified from LIBIHT project (Thomasaon Zhao et al) with Windows style updates.
 * @version 0.18
 * @date 2025-12-02
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#include "pch.h"

//////////////////////////////////////////////////
//             Global Definitions               //
//////////////////////////////////////////////////

ULONGLONG  LbrCapacity = 0;
LIST_ENTRY LbrStateHead;
KSPIN_LOCK LbrStateLock;

//
// Typical Intel LBR capacities based on CPU model
// This is a subset; you can expand this as needed
//
CPU_LBR_MAP CPU_LBR_MAPS[] = {
    {0x5c, 32},
    {0x5f, 32},
    {0x4e, 32},
    {0x5e, 32},
    {0x8e, 32},
    {0x9e, 32},
    {0x55, 32},
    {0x66, 32},
    {0x7a, 32},
    {0x67, 32},
    {0x6a, 32},
    {0x6c, 32},
    {0x7d, 32},
    {0x7e, 32},
    {0x8c, 32},
    {0x8d, 32},
    {0xa5, 32},
    {0xa6, 32},
    {0xa7, 32},
    {0xa8, 32},
    {0x86, 32},
    {0x8a, 32},
    {0x96, 32},
    {0x9c, 32},
    {0x3d, 16},
    {0x47, 16},
    {0x4f, 16},
    {0x56, 16},
    {0x3c, 16},
    {0x45, 16},
    {0x46, 16},
    {0x3f, 16},
    {0x2a, 16},
    {0x2d, 16},
    {0x3a, 16},
    {0x3e, 16},
    {0x1a, 16},
    {0x1e, 16},
    {0x1f, 16},
    {0x2e, 16},
    {0x25, 16},
    {0x2c, 16},
    {0x2f, 16},
    {0x17, 4},
    {0x1d, 4},
    {0x0f, 4},
    {0x37, 8},
    {0x4a, 8},
    {0x4c, 8},
    {0x4d, 8},
    {0x5a, 8},
    {0x5d, 8},
    {0x1c, 8},
    {0x26, 8},
    {0x27, 8},
    {0x35, 8},
    {0x36, 8}};

/**
 * @brief Initialize LBR state list and spinlock
 *
 * @param State
 * @return VOID
 */
VOID
LbrInitialize()
{
    InitializeListHead(&LbrStateHead);
    KeInitializeSpinLock(&LbrStateLock);
}

/**
 * @brief Read LBR MSRs and store the values in the provided LBR_STATE structure
 *
 * @param ApplyFromVmxRootMode
 * @param ApplyByVmcall
 *
 * @return VOID
 */
VOID
LbrGetLbr(BOOLEAN ApplyFromVmxRootMode, BOOLEAN ApplyByVmcall)
{
    ULONGLONG         DbgCtlMsr;
    UINT64            LbrTos;
    ULONG             CurrentIdx;
    LBR_STACK_ENTRY * State;
    UINT32            CurrentCore = 0;

    //
    // Get the current core id
    //
    CurrentCore = KeGetCurrentProcessorNumberEx(NULL);

    //
    // Get the current processor LBR stack
    //
    State = &g_LbrStateList[CurrentCore];

    if (ApplyFromVmxRootMode)
    {
        if (ApplyByVmcall)
        {
            DbgCtlMsr = g_Callbacks.VmFuncGetDebugctlVmcallOnTargetCore();
        }
        else
        {
            DbgCtlMsr = g_Callbacks.VmFuncGetDebugctl();
        }

        DbgCtlMsr &= ~DEBUGCTLMSR_LBR;

        if (ApplyByVmcall)
        {
            g_Callbacks.VmFuncSetDebugctlVmcallOnTargetCore(DbgCtlMsr);
        }
        else
        {
            g_Callbacks.VmFuncSetDebugctl(DbgCtlMsr);
        }
    }
    else
    {
        xrdmsr(MSR_IA32_DEBUGCTLMSR, &DbgCtlMsr);
        DbgCtlMsr &= ~DEBUGCTLMSR_LBR;
        xwrmsr(MSR_IA32_DEBUGCTLMSR, DbgCtlMsr);
    }

    //
    // Read the current TOS index to know where the most recent branch is stored
    //
    xrdmsr(MSR_LBR_TOS, &LbrTos);

    //
    // Dump LBR entries into the current core's state structure
    //
    for (ULONG i = 0; i < (ULONG)LbrCapacity; i++)
    {
        xrdmsr(MSR_LBR_NHM_FROM + i, &State->BranchEntry[i].From);
        xrdmsr(MSR_LBR_NHM_TO + i, &State->BranchEntry[i].To);
    }

    LogInfo("LBR Chronological Trace\n");

    for (ULONG i = 1; i <= LbrCapacity; i++)
    {
        CurrentIdx = (ULONG)(LbrTos + i) % (ULONG)LbrCapacity;

        if (State->BranchEntry[CurrentIdx].From == 0)
            continue;

        LogInfo("[%2u] FROM: 0x%llx  TO: 0x%llx\n",
                CurrentIdx,
                State->BranchEntry[CurrentIdx].From,
                State->BranchEntry[CurrentIdx].To);
    }
}

/**
 * @brief Write LBR MSRs from the provided LBR_STATE structure
 *
 * @param ApplyFromVmxRootMode
 * @param ApplyByVmcall
 *
 * @return VOID
 */
VOID
LbrPutLbr(BOOLEAN ApplyFromVmxRootMode, BOOLEAN ApplyByVmcall)
{
    ULONGLONG DbgCtlMsr;

    //
    // Force the selection mask
    //
    xwrmsr(MSR_LBR_SELECT, LBR_SELECT);

    //
    // Clear hardware state
    //
    xwrmsr(MSR_LBR_TOS, 0);
    for (ULONG i = 0; i < (ULONG)LbrCapacity; i++)
    {
        xwrmsr(MSR_LBR_NHM_FROM + i, 0);
        xwrmsr(MSR_LBR_NHM_TO + i, 0);
    }

    if (ApplyFromVmxRootMode)
    {
        if (ApplyByVmcall)
        {
            DbgCtlMsr = g_Callbacks.VmFuncGetDebugctlVmcallOnTargetCore();
        }
        else
        {
            DbgCtlMsr = g_Callbacks.VmFuncGetDebugctl();
        }

        DbgCtlMsr |= DEBUGCTLMSR_LBR; // Bit 0 = 1
        DbgCtlMsr &= ~(1ULL << 11);   // Bit 11 = 0

        if (ApplyByVmcall)
        {
            g_Callbacks.VmFuncSetDebugctlVmcallOnTargetCore(DbgCtlMsr);
        }
        else
        {
            g_Callbacks.VmFuncSetDebugctl(DbgCtlMsr);
        }
    }
    else
    {
        //
        // Enable LBR and CLEAR 'Freeze LBRs on PMI' (Bit 11)
        // If Bit 11 is set, the LBR stops as soon as a single interrupt happens
        //
        xrdmsr(MSR_IA32_DEBUGCTLMSR, &DbgCtlMsr);
        DbgCtlMsr |= DEBUGCTLMSR_LBR; // Bit 0 = 1
        DbgCtlMsr &= ~(1ULL << 11);   // Bit 11 = 0
        xwrmsr(MSR_IA32_DEBUGCTLMSR, DbgCtlMsr);
    }
}

/**
 * @brief Flush LBR MSRs by disabling LBR and clearing all LBR entries
 *
 * @return VOID
 */
VOID
LbrFlushLbr()
{
    ULONG     i;
    ULONGLONG DbgCtlMsr;
    KIRQL     OldIrql;

    xlock_core(&OldIrql);

    //
    // Disable LBR
    //
    LogInfo("Flush LBR on cpu core: %d\n", xcoreid());

    xrdmsr(MSR_IA32_DEBUGCTLMSR, &DbgCtlMsr);
    DbgCtlMsr &= ~DEBUGCTLMSR_LBR;
    xwrmsr(MSR_IA32_DEBUGCTLMSR, DbgCtlMsr);

    //
    // Flush LBR registers
    //
    xwrmsr(MSR_LBR_SELECT, 0);
    xwrmsr(MSR_LBR_TOS, 0);

    for (i = 0; i < LbrCapacity; i++)
    {
        xwrmsr(MSR_LBR_NHM_FROM + i, 0);
        xwrmsr(MSR_LBR_NHM_TO + i, 0);
    }

    xrelease_core(&OldIrql);
}

/**
 * @brief Start collecting LBR branches for a specific process and store the configuration in the global LBR state list
 *
 * @param ApplyFromVmxRootMode
 * @param ApplyByVmcall
 *
 * @return BOOLEAN
 */
BOOLEAN
LbrStartLbr(BOOLEAN ApplyFromVmxRootMode, BOOLEAN ApplyByVmcall)
{
    if (LbrCapacity == 0)
    {
        LogInfo("LBR: Aborting, CPU model not supported.\n");
        return FALSE;
    }

    LbrPutLbr(ApplyFromVmxRootMode, ApplyByVmcall);

    return TRUE;
}

/**
 * @brief Stop collecting LBR branches for a specific process and remove the corresponding LBR state from the global list
 *
 * @param ApplyFromVmxRootMode
 * @param ApplyByVmcall
 *
 * @return BOOLEAN
 */
BOOLEAN
LbrStopLbr(BOOLEAN ApplyFromVmxRootMode, BOOLEAN ApplyByVmcall)
{
    LbrGetLbr(ApplyFromVmxRootMode, ApplyByVmcall);

    return TRUE;
}

/**
 * @brief Check if the current CPU supports LBR by examining the CPU family and model and looking up the corresponding LBR capacity
 *
 * @return BOOLEAN
 */
BOOLEAN
LbrCheck()
{
    ULONG     a, b, c, d;
    ULONG     Family, Model;
    ULONGLONG i;

    xcpuid(1, &a, &b, &c, &d);

    Family = ((a >> 8) & 0xF) + ((a >> 20) & 0xFF);
    Model  = ((a >> 4) & 0xF) | ((a >> 12) & 0xF0);

    for (i = 0; i < sizeof(CPU_LBR_MAPS) / sizeof(CPU_LBR_MAPS[0]); ++i)
    {
        if (Model == CPU_LBR_MAPS[i].Model)
        {
            LbrCapacity = CPU_LBR_MAPS[i].LbrCapacity;
            break;
        }
    }

    if (LbrCapacity == 0)
    {
        return FALSE;
    }

    return TRUE;
}
