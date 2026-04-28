/**
 * @file Lbr.c
 * @author Hari Mishal (harimishal6@gmail.com)
 * @brief Last Branch Record (LBR) tracing implementation for HyperTrace module
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

/**
 * @brief Flush LBR MSRs by disabling LBR and clearing all LBR entries
 *
 * @return VOID
 */
VOID
LbrFlush()
{
    ULONG     i;
    ULONGLONG DbgCtlMsr;
    BOOLEAN   IsOnVmxRootMode;

    //
    // Disable LBR
    //
    LogInfo("Flush LBR on cpu core: %d\n", KeGetCurrentProcessorNumberEx(NULL));

    if (g_RunningOnHypervisorEnvironment)
    {
        IsOnVmxRootMode = g_Callbacks.VmFuncVmxGetCurrentExecutionMode();

        if (IsOnVmxRootMode)
        {
            //
            // It is on VMX-root mode, run it directly to get the IA32_DEBUGCTL MSR value in VMCS
            //
            DbgCtlMsr = g_Callbacks.VmFuncGetDebugctl();
        }
        else
        {
            //
            // It is not on VMX-root mode, so we need to perform a VMCALL to set the IA32_DEBUGCTL MSR value on the target core
            //
            DbgCtlMsr = g_Callbacks.VmFuncGetDebugctlVmcallOnTargetCore();
        }

        DbgCtlMsr &= ~IA32_DEBUGCTL_LBR_FLAG;

        if (IsOnVmxRootMode)
        {
            //
            // It is on VMX-root mode, run it directly to set the IA32_DEBUGCTL MSR value in VMCS
            //
            g_Callbacks.VmFuncSetDebugctl(DbgCtlMsr);
        }
        else
        {
            //
            // It is not on VMX-root mode, so we need to perform a VMCALL to set the IA32_DEBUGCTL MSR value on the target core
            //
            g_Callbacks.VmFuncSetDebugctlVmcallOnTargetCore(DbgCtlMsr);
        }
    }
    else
    {
        xrdmsr(IA32_DEBUGCTL, &DbgCtlMsr);
        DbgCtlMsr &= ~IA32_DEBUGCTL_LBR_FLAG;
        xwrmsr(IA32_DEBUGCTL, DbgCtlMsr);
    }

    //
    // *** Flush LBR registers ***
    //

    //
    // Set LBR filter (MSR_LEGACY_LBR_SELECT) to 0 to capture all branch types (reset)
    //
    if (g_RunningOnHypervisorEnvironment)
    {
        IsOnVmxRootMode = g_Callbacks.VmFuncVmxGetCurrentExecutionMode();

        //
        // If we don't set it on VMX-root mode, the LBR MSRs won't work based on our tests
        // even though it is just MSR (and not a VMCS field)
        //
        if (IsOnVmxRootMode)
        {
            //
            // It is on VMX-root mode, run it directly to set the MSR_LEGACY_LBR_SELECT MSR value
            //
            g_Callbacks.VmFuncSetLbrSelect(0);
        }
        else
        {
            //
            // It is not on VMX-root mode, so we need to perform a VMCALL to set the MSR_LEGACY_LBR_SELECT MSR value on the target core
            //
            g_Callbacks.VmFuncSetLbrSelectVmcallOnTargetCore(0);
        }
    }
    else
    {
        xwrmsr(MSR_LEGACY_LBR_SELECT, 0);
    }

    //
    // Clear TOS and all LBR entries
    //
    xwrmsr(MSR_LBR_TOS, 0);

    for (i = 0; i < LbrCapacity; i++)
    {
        xwrmsr(MSR_LBR_NHM_FROM + i, 0);
        xwrmsr(MSR_LBR_NHM_TO + i, 0);
    }
}

/**
 * @brief Start collecting LBR branches
 *
 * @param FilterOptions A bitmask of filter options to apply to the LBR branches (e.g., filtering by branch type, privilege level, etc.)
 *
 * @return BOOLEAN
 */
BOOLEAN
LbrStart(UINT64 FilterOptions)
{
    BOOLEAN   IsOnVmxRootMode;
    ULONGLONG DbgCtlMsr;

    if (LbrCapacity == 0)
    {
        LogInfo("LBR: Aborting, CPU model not supported.\n");
        return FALSE;
    }

    //
    // Set LBR filter (MSR_LEGACY_LBR_SELECT) to selection mask
    //
    if (g_RunningOnHypervisorEnvironment)
    {
        IsOnVmxRootMode = g_Callbacks.VmFuncVmxGetCurrentExecutionMode();

        //
        // If we don't set it on VMX-root mode, the LBR MSRs won't work based on our tests
        // even though it is just MSR (and not a VMCS field)
        //
        if (IsOnVmxRootMode)
        {
            //
            // It is on VMX-root mode, run it directly to set the MSR_LEGACY_LBR_SELECT MSR value
            //
            g_Callbacks.VmFuncSetLbrSelect(FilterOptions);
        }
        else
        {
            //
            // It is not on VMX-root mode, so we need to perform a VMCALL to set the MSR_LEGACY_LBR_SELECT MSR value on the target core
            //
            g_Callbacks.VmFuncSetLbrSelectVmcallOnTargetCore(FilterOptions);
        }
    }
    else
    {
        xwrmsr(MSR_LEGACY_LBR_SELECT, FilterOptions);
    }

    //
    // Clear hardware state
    //
    xwrmsr(MSR_LBR_TOS, 0);

    for (ULONG i = 0; i < (ULONG)LbrCapacity; i++)
    {
        xwrmsr(MSR_LBR_NHM_FROM + i, 0);
        xwrmsr(MSR_LBR_NHM_TO + i, 0);
    }

    if (g_RunningOnHypervisorEnvironment)
    {
        IsOnVmxRootMode = g_Callbacks.VmFuncVmxGetCurrentExecutionMode();

        if (IsOnVmxRootMode)
        {
            //
            // It is on VMX-root mode, run it directly to get the IA32_DEBUGCTL MSR value in VMCS
            //
            DbgCtlMsr = g_Callbacks.VmFuncGetDebugctl();
        }
        else
        {
            //
            // It is not on VMX-root mode, so we need to perform a VMCALL to get the IA32_DEBUGCTL MSR value on the target core
            //
            DbgCtlMsr = g_Callbacks.VmFuncGetDebugctlVmcallOnTargetCore();
        }

        DbgCtlMsr |= IA32_DEBUGCTL_LBR_FLAG; // Bit 0 = 1
        DbgCtlMsr &= ~(1ULL << 11);          // Bit 11 = 0

        if (IsOnVmxRootMode)
        {
            //
            // It is on VMX-root mode, run it directly to set the IA32_DEBUGCTL MSR value in VMCS
            //
            g_Callbacks.VmFuncSetDebugctl(DbgCtlMsr);
        }
        else
        {
            //
            // It is not on VMX-root mode, so we need to perform a VMCALL to set the IA32_DEBUGCTL MSR value on the target core
            //
            g_Callbacks.VmFuncSetDebugctlVmcallOnTargetCore(DbgCtlMsr);
        }
    }
    else
    {
        //
        // Enable LBR and CLEAR 'Freeze LBRs on PMI' (Bit 11)
        // If Bit 11 is set, the LBR stops as soon as a single interrupt happens
        //
        xrdmsr(IA32_DEBUGCTL, &DbgCtlMsr);
        DbgCtlMsr |= IA32_DEBUGCTL_LBR_FLAG; // Bit 0 = 1
        DbgCtlMsr &= ~(1ULL << 11);          // Bit 11 = 0
        xwrmsr(IA32_DEBUGCTL, DbgCtlMsr);
    }

    return TRUE;
}

/**
 * @brief Save LBR branches
 *
 * @return VOID
 */
VOID
LbrSave()
{
    UINT64            LbrTos;
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

    //
    // Read and store the current TOS index to know where the most recent branch is stored
    //
    xrdmsr(MSR_LBR_TOS, &LbrTos);
    State->Tos = (UINT32)LbrTos;

    //
    // Dump LBR entries into the current core's state structure
    //
    for (ULONG i = 0; i < (ULONG)LbrCapacity; i++)
    {
        xrdmsr(MSR_LBR_NHM_FROM + i, &State->BranchEntry[i].From);
        xrdmsr(MSR_LBR_NHM_TO + i, &State->BranchEntry[i].To);
        xrdmsr(MSR_LASTBRANCH_INFO_0 + i, &State->LastBranchInfo[i].AsUInt);
    }
}

/**
 * @brief Stop collecting LBR branches
 *
 * @return VOID
 */
VOID
LbrStop()
{
    ULONGLONG DbgCtlMsr;
    BOOLEAN   IsOnVmxRootMode;

    if (g_RunningOnHypervisorEnvironment)
    {
        IsOnVmxRootMode = g_Callbacks.VmFuncVmxGetCurrentExecutionMode();

        if (IsOnVmxRootMode)
        {
            //
            // It is on VMX-root mode, run it directly to get the IA32_DEBUGCTL MSR value in VMCS
            //
            DbgCtlMsr = g_Callbacks.VmFuncGetDebugctl();
        }
        else
        {
            //
            // It is not on VMX-root mode, so we need to perform a VMCALL to get the IA32_DEBUGCTL MSR value on the target core
            //
            DbgCtlMsr = g_Callbacks.VmFuncGetDebugctlVmcallOnTargetCore();
        }

        DbgCtlMsr &= ~IA32_DEBUGCTL_LBR_FLAG;

        if (IsOnVmxRootMode)
        {
            //
            // It is on VMX-root mode, run it directly to set the IA32_DEBUGCTL MSR value in VMCS
            //
            g_Callbacks.VmFuncSetDebugctl(DbgCtlMsr);
        }
        else
        {
            //
            // It is not on VMX-root mode, so we need to perform a VMCALL to set the IA32_DEBUGCTL MSR value on the target core
            //
            g_Callbacks.VmFuncSetDebugctlVmcallOnTargetCore(DbgCtlMsr);
        }
    }
    else
    {
        xrdmsr(IA32_DEBUGCTL, &DbgCtlMsr);
        DbgCtlMsr &= ~IA32_DEBUGCTL_LBR_FLAG;
        xwrmsr(IA32_DEBUGCTL, DbgCtlMsr);
    }

    //
    // Save the LBR entries
    //
    LbrSave();
}

/**
 * @brief Filter LBR branches based on the provided options
 * @param FilterOptions A bitmask of filter options to apply to the LBR branches
 *
 * @return VOID
 */
VOID
LbrFilter(UINT64 FilterOptions)
{
    LogInfo("Updating LBR filter options: 0x%llx\n", FilterOptions);

    //
    // First, we flush the LBR to clear out any existing entries that may not meet the new filter criteria
    //
    LbrFlush();

    //
    // Then we apply the new filter options and re-enable LBR with the updated filter settings
    //
    LbrStart(FilterOptions);
}

/**
 * @brief Dump collected LBR branches
 *
 * @return VOID
 */
VOID
LbrDump()
{
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

    LogInfo("LBR Chronological Trace");

    for (ULONG i = 1; i <= LbrCapacity; i++)
    {
        CurrentIdx = (ULONG)(State->Tos + i) % (ULONG)LbrCapacity;

        if (State->BranchEntry[CurrentIdx].From == 0)
            continue;

        Log("[%2u] Branch Mispredicted: %s, Cycle Count (Decimal): %03d - From: %016llx  To: %016llx\n",
            CurrentIdx,
            State->LastBranchInfo[CurrentIdx].Mispred ? "true " : "false",
            State->LastBranchInfo[CurrentIdx].CycleCount,
            State->BranchEntry[CurrentIdx].From,
            State->BranchEntry[CurrentIdx].To);
    }
}
