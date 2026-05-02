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
 * @brief Check if the current CPU supports architectural LBR
 *
 * @return BOOLEAN
 */
BOOLEAN
LbrCheckAndReadArchitecturalLbrDetails()
{
    ULONG a, b, c, d;

    CPUID_EAX_07 Edx07 = {0};

    CPUID28_EAX Eax1c = {0};
    CPUID28_EBX Ebx1c = {0};
    CPUID28_ECX Ecx1c = {0};

    //
    // Check for Architectural LBR support
    //
    //
    xcpuidex(CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS, 0x00, &a, &b, &c, &d);

    Edx07.Edx.AsUInt = d;

    //
    // CPUID.07H.00H:EDX[19] == 1 means arch LBR is supported
    //
    if (Edx07.Edx.AsUInt & (1 << 19))
    {
        g_ArchBasedLastBranchRecord = TRUE;
    }
    else
    {
        //
        // Architectural LBR is not supported by the CPU
        //
        g_ArchBasedLastBranchRecord = FALSE;

        return FALSE;
    }

    //
    // Being here means the CPU supports architectural LBR, we can read the LBR capabilities from CPUID 0x1c leaf
    //
    xcpuidex(CPUID_ARCH_LAST_BRANCH_RECORD_INFORMATION, 0x00, &a, &b, &c, &d);

    //
    // Assign LBR leafs to sturcture for easier access
    //
    Eax1c.AsUInt = a;
    Ebx1c.AsUInt = b;
    Ecx1c.AsUInt = c;

    //
    // Read LBR capacity from CPUID.1CH.00H:EAX[7:0]
    // Based on Intel SDM: For each bit n set in this field, the IA32_LBR_DEPTH.DEPTH value 8 * (n + 1) is supported
    //
    if (Eax1c.LbrDepthMask)
    {
        //
        // Get the highest set bit in LbrDepthMask to determine the maximum supported LBR depth
        //
        ULONG HighestSetBit = 0;
        for (ULONG i = 0; i < 8; i++)
        {
            if (Eax1c.LbrDepthMask & (1 << i))
            {
                HighestSetBit = i;
            }
        }
        LbrCapacity = 8 * (HighestSetBit + 1);
    }
    else
    {
        //
        // If LbrDepthMask is 0, it means the CPU supports architectural LBR but does not specify the depth, we can assume a default value (e.g., 16 or 32) or treat it as unsupported
        //
        LbrCapacity = MAXIMUM_LBR_CAPACITY; // Assuming a default capacity of MAXIMUM_LBR_CAPACITY if not specified
    }

    return TRUE;
}

/**
 * @brief Check if the current CPU supports LBR by examining the CPU family and model and looking up the corresponding LBR capacity
 *
 * @return BOOLEAN
 */
BOOLEAN
LbrCheckAndReadLegacyLbrDetails()
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
 * @brief Convert a filter options bitmask into IA32_LBR_CTL format for architectural LBR
 *
 * @param FilterOptions A bitmask of filter options (e.g., LBR_KERNEL, LBR_USER, LBR_JCC, etc.)
 * @param Ia32LbrCtl Pointer to the IA32_LBR_CTL_REGISTER to populate with the converted filter bits
 * @return VOID
 */
VOID
LbrBuildArchBasedFilterOptions(UINT64 FilterOptions, IA32_LBR_CTL_REGISTER * Ia32LbrCtl)
{
    if (FilterOptions & LBR_KERNEL)
    {
        Ia32LbrCtl->Bits.OS = 1;
    }
    if (FilterOptions & LBR_USER)
    {
        Ia32LbrCtl->Bits.USR = 1;
    }
    if (FilterOptions & LBR_JCC)
    {
        Ia32LbrCtl->Bits.JCC = 1;
    }
    if (FilterOptions & LBR_REL_CALL)
    {
        Ia32LbrCtl->Bits.NearRelCall = 1;
    }
    if (FilterOptions & LBR_IND_CALL)
    {
        Ia32LbrCtl->Bits.NearIndCall = 1;
    }
    if (FilterOptions & LBR_RETURN)
    {
        Ia32LbrCtl->Bits.NearRet = 1;
    }
    if (FilterOptions & LBR_IND_JMP)
    {
        Ia32LbrCtl->Bits.NearIndJmp = 1;
    }
    if (FilterOptions & LBR_REL_JMP)
    {
        Ia32LbrCtl->Bits.NearRelJmp = 1;
    }
    if (FilterOptions & LBR_FAR)
    {
        Ia32LbrCtl->Bits.OtherBranch = 1;
    }
    if (FilterOptions & LBR_CALL_STACK)
    {
        Ia32LbrCtl->Bits.CallStack = 1;
    }
}

/**
 * @brief Set the LBR select filter MSR (MSR_LEGACY_LBR_SELECT) for legacy LBR,
 *        dispatching via VMCALL when in VMX non-root mode
 *
 * @details This function is only relevant for legacy LBR. Architectural LBR encodes
 *          filter options directly in IA32_LBR_CTL and does not use MSR_LEGACY_LBR_SELECT.
 *          Note: Even though MSR_LEGACY_LBR_SELECT is a plain MSR (not a VMCS field), it
 *          must be set from VMX-root mode for the LBR MSRs to work correctly.
 *
 * @param FilterOptions The raw filter options bitmask to write into MSR_LEGACY_LBR_SELECT
 * @return VOID
 */
VOID
LbrSetLbrSelectFilter(UINT64 FilterOptions)
{
    BOOLEAN IsOnVmxRootMode;

    if (g_RunningOnHypervisorEnvironment)
    {
        IsOnVmxRootMode = g_Callbacks.VmFuncVmxGetCurrentExecutionMode();

        //
        // If we don't set it on VMX-root mode, the LBR MSRs won't work based on our tests
        // even though it is just a plain MSR (and not a VMCS field)
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
}

/**
 * @brief Zero out the LBR hardware state (TOS and all from/to MSR pairs)
 *
 * @details For architectural LBR the TOS index is not maintained by software — the CPU
 *          shifts entries automatically — so MSR_LBR_TOS is skipped in that case.
 *
 * @return VOID
 */
VOID
LbrClearHardwareState()
{
    //
    // For architectural LBR, the TOS is not used; the CPU shifts LBR entries automatically
    // and does not update the TOS index
    //
    if (!g_ArchBasedLastBranchRecord)
    {
        xwrmsr(MSR_LBR_TOS, 0);
    }

    //
    // Clearing LBR TO and FROM MSRs
    //
    for (ULONG i = 0; i < (ULONG)LbrCapacity; i++)
    {
        xwrmsr(MSR_LBR_NHM_FROM + i, 0);
        xwrmsr(MSR_LBR_NHM_TO + i, 0);
    }
}

/**
 * @brief Enable LBR collection on architectural (ARCH) based LBR
 *
 * @param Ia32LbrCtl Pointer to the IA32_LBR_CTL_REGISTER whose LBREn bit will be set
 * @return VOID
 */
VOID
LbrEnableArchBased(IA32_LBR_CTL_REGISTER * Ia32LbrCtl)
{
    Ia32LbrCtl->Bits.LBREn = 1;
}

/**
 * @brief Enable LBR collection on legacy based LBR
 *
 * @param DbgCtlMsr Pointer to the IA32_DEBUGCTL MSR value to be modified
 * @return VOID
 */
VOID
LbrEnableLegacyBased(ULONGLONG * DbgCtlMsr)
{
    //
    // Enable LBR and CLEAR 'Freeze LBRs on PMI' (Bit 11)
    // If Bit 11 is set, the LBR stops recording as soon as a single PMI interrupt fires
    //
    *DbgCtlMsr |= IA32_DEBUGCTL_LBR_FLAG; // Bit 0 = 1
    *DbgCtlMsr &= ~(1ULL << 11);          // Bit 11 = 0
}

/**
 * @brief Start LBR collection when running natively (outside any hypervisor environment)
 *
 * @param Ia32LbrCtl The pre-built IA32_LBR_CTL_REGISTER value carrying the filter bits (arch LBR)
 * @return VOID
 */
VOID
LbrStartOnNativeMode(IA32_LBR_CTL_REGISTER Ia32LbrCtl)
{
    ULONGLONG DbgCtlMsr = 0;

    if (g_ArchBasedLastBranchRecord)
    {
        //
        // No need to read the existing control since we are replacing it entirely for ARCH LBR
        //
        LbrEnableArchBased(&Ia32LbrCtl);
        xwrmsr(IA32_LBR_CTL, Ia32LbrCtl.AsUInt);
    }
    else
    {
        xrdmsr(IA32_DEBUGCTL, &DbgCtlMsr);
        LbrEnableLegacyBased(&DbgCtlMsr);
        xwrmsr(IA32_DEBUGCTL, DbgCtlMsr);
    }
}

/**
 * @brief Read current LBR control register values while in VMX root-mode
 *
 * @param DbgCtlMsr Pointer to receive the IA32_DEBUGCTL MSR value (legacy LBR)
 * @param Ia32LbrCtl Pointer to receive the IA32_LBR_CTL register value (arch LBR)
 * @return VOID
 */
VOID
LbrGetValuesOnVmxRootMode(ULONGLONG * DbgCtlMsr, IA32_LBR_CTL_REGISTER * Ia32LbrCtl)
{
    if (g_ArchBasedLastBranchRecord)
    {
        //
        // It is on VMX-root mode, run it directly to get the IA32_LBR_CTL value in VMCS
        //
        Ia32LbrCtl->AsUInt = g_Callbacks.VmFuncGetGuestIa32LbrCtl();
    }
    else
    {
        //
        // It is on VMX-root mode, run it directly to get the IA32_DEBUGCTL MSR value in VMCS
        //
        *DbgCtlMsr = g_Callbacks.VmFuncGetDebugctl();
    }
}

/**
 * @brief Read current LBR control register values while in VMX non-root mode (via VMCALL)
 *
 * @param DbgCtlMsr Pointer to receive the IA32_DEBUGCTL MSR value (legacy LBR)
 * @param Ia32LbrCtl Pointer to receive the IA32_LBR_CTL register value (arch LBR)
 * @return VOID
 */
VOID
LbrGetValuesOnVmxNonRootMode(ULONGLONG * DbgCtlMsr, IA32_LBR_CTL_REGISTER * Ia32LbrCtl)
{
    if (g_ArchBasedLastBranchRecord)
    {
        //
        // It is not on VMX-root mode, so we need to perform a VMCALL to get the IA32_LBR_CTL value on the target core
        //
        Ia32LbrCtl->AsUInt = g_Callbacks.VmFuncGetGuestIa32LbrCtlVmcallOnTargetCore();
    }
    else
    {
        //
        // It is not on VMX-root mode, so we need to perform a VMCALL to get the IA32_DEBUGCTL MSR value on the target core
        //
        *DbgCtlMsr = g_Callbacks.VmFuncGetDebugctlVmcallOnTargetCore();
    }
}

/**
 * @brief Disable LBR collection on architectural (ARCH) based LBR
 *
 * @param Ia32LbrCtl Pointer to the IA32_LBR_CTL register whose LBREn bit will be cleared
 * @return VOID
 */
VOID
LbrDisableArchBased(IA32_LBR_CTL_REGISTER * Ia32LbrCtl)
{
    Ia32LbrCtl->Bits.LBREn = 0;
}

/**
 * @brief Disable LBR collection on legacy based LBR
 *
 * @param DbgCtlMsr Pointer to the IA32_DEBUGCTL MSR value whose LBR flag will be cleared
 * @return VOID
 */
VOID
LbrDisableLegacyBased(ULONGLONG * DbgCtlMsr)
{
    *DbgCtlMsr &= ~IA32_DEBUGCTL_LBR_FLAG;
}

/**
 * @brief Write back modified LBR control register values while in VMX root-mode
 *
 * @param DbgCtlMsr The updated IA32_DEBUGCTL MSR value to apply (legacy LBR)
 * @param Ia32LbrCtl The updated IA32_LBR_CTL register value to apply (arch LBR)
 * @return VOID
 */
VOID
LbrSetValuesOnVmxRootMode(ULONGLONG DbgCtlMsr, IA32_LBR_CTL_REGISTER Ia32LbrCtl)
{
    if (g_ArchBasedLastBranchRecord)
    {
        //
        // It is on VMX-root mode, run it directly to set the IA32_LBR_CTL value in VMCS
        //
        g_Callbacks.VmFuncSetGuestIa32LbrCtl(Ia32LbrCtl.AsUInt);
    }
    else
    {
        //
        // It is on VMX-root mode, run it directly to set the IA32_DEBUGCTL MSR value in VMCS
        //
        g_Callbacks.VmFuncSetDebugctl(DbgCtlMsr);
    }
}

/**
 * @brief Write back modified LBR control register values while in VMX non-root mode (via VMCALL)
 *
 * @param DbgCtlMsr The updated IA32_DEBUGCTL MSR value to apply (legacy LBR)
 * @param Ia32LbrCtl The updated IA32_LBR_CTL register value to apply (arch LBR)
 * @return VOID
 */
VOID
LbrSetValuesOnVmxNonRootMode(ULONGLONG DbgCtlMsr, IA32_LBR_CTL_REGISTER Ia32LbrCtl)
{
    if (g_ArchBasedLastBranchRecord)
    {
        //
        // It is not on VMX-root mode, so we need to perform a VMCALL to set the IA32_LBR_CTL value on the target core
        //
        g_Callbacks.VmFuncSetGuestIa32LbrCtlVmcallOnTargetCore(Ia32LbrCtl.AsUInt);
    }
    else
    {
        //
        // It is not on VMX-root mode, so we need to perform a VMCALL to set the IA32_DEBUGCTL MSR value on the target core
        //
        g_Callbacks.VmFuncSetDebugctlVmcallOnTargetCore(DbgCtlMsr);
    }
}

/**
 * @brief Stop LBR collection when running natively (outside any hypervisor environment)
 *
 * @return VOID
 */
VOID
LbrStopOnNativeMode()
{
    ULONGLONG             DbgCtlMsr  = NULL64_ZERO;
    IA32_LBR_CTL_REGISTER Ia32LbrCtl = {0};

    if (g_ArchBasedLastBranchRecord)
    {
        xrdmsr(IA32_LBR_CTL, &Ia32LbrCtl.AsUInt);
        LbrDisableArchBased(&Ia32LbrCtl);
        xwrmsr(IA32_LBR_CTL, Ia32LbrCtl);
    }
    else
    {
        xrdmsr(IA32_DEBUGCTL, &DbgCtlMsr);
        LbrDisableLegacyBased(&DbgCtlMsr);
        xwrmsr(IA32_DEBUGCTL, DbgCtlMsr);
    }
}

/**
 * @brief Zero the ARCH LBR control register while in VMX root-mode
 *
 * @details Setting IA32_LBR_CTL to 0 simultaneously clears all filter bits and
 *          disables collection (LBREn = 0), which is the correct flush state.
 *
 * @return VOID
 */
VOID
LbrResetArchControlOnVmxRootMode()
{
    //
    // It is on VMX-root mode, set the entire IA32_LBR_CTL to 0 directly in VMCS
    // (this also disables LBR since LBREn is cleared)
    //
    g_Callbacks.VmFuncSetGuestIa32LbrCtl(0);
}

/**
 * @brief Zero the ARCH LBR control register while in VMX non-root mode (via VMCALL)
 *
 * @details Setting IA32_LBR_CTL to 0 simultaneously clears all filter bits and
 *          disables collection (LBREn = 0), which is the correct flush state.
 *
 * @return VOID
 */
VOID
LbrResetArchControlOnVmxNonRootMode()
{
    //
    // It is not on VMX-root mode, perform a VMCALL to set the entire IA32_LBR_CTL to 0 on the target core
    // (this also disables LBR since LBREn is cleared)
    //
    g_Callbacks.VmFuncSetGuestIa32LbrCtlVmcallOnTargetCore(0);
}

/**
 * @brief Reset the LBR control registers to zero, covering both ARCH and legacy LBR
 *        across all execution environments (native, VMX root-mode, VMX non-root mode)
 *
 * @details For architectural LBR, IA32_LBR_CTL is zeroed entirely (clearing filter bits
 *          and disabling collection in one write). For legacy LBR, MSR_LEGACY_LBR_SELECT
 *          is zeroed to reset the branch filter to its default capture-all state.
 *          Note: Even though MSR_LEGACY_LBR_SELECT is a plain MSR (not a VMCS field),
 *          it must be written from VMX-root mode for the change to take effect.
 *
 * @return VOID
 */
VOID
LbrResetControlRegisters()
{
    BOOLEAN IsOnVmxRootMode;

    if (g_RunningOnHypervisorEnvironment)
    {
        IsOnVmxRootMode = g_Callbacks.VmFuncVmxGetCurrentExecutionMode();

        //
        // If we don't set it on VMX-root mode, the LBR MSRs won't work based on our tests
        // even though it is just a plain MSR (and not a VMCS field)
        //
        if (IsOnVmxRootMode)
        {
            if (g_ArchBasedLastBranchRecord)
            {
                LbrResetArchControlOnVmxRootMode();
            }
            else
            {
                //
                // Reuse LbrSetLbrSelectFilter: writing 0 resets MSR_LEGACY_LBR_SELECT to
                // its default state (capture all branch types) from VMX-root mode
                //
                g_Callbacks.VmFuncSetLbrSelect(0);
            }
        }
        else
        {
            if (g_ArchBasedLastBranchRecord)
            {
                LbrResetArchControlOnVmxNonRootMode();
            }
            else
            {
                //
                // Reuse LbrSetLbrSelectFilter: writing 0 resets MSR_LEGACY_LBR_SELECT to
                // its default state (capture all branch types) via VMCALL on the target core
                //
                g_Callbacks.VmFuncSetLbrSelectVmcallOnTargetCore(0);
            }
        }
    }
    else
    {
        if (g_ArchBasedLastBranchRecord)
        {
            xwrmsr(IA32_LBR_CTL, 0);
        }
        else
        {
            xwrmsr(MSR_LEGACY_LBR_SELECT, 0);
        }
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
    BOOLEAN               IsOnVmxRootMode;
    IA32_LBR_CTL_REGISTER Ia32LbrCtl = {0};
    ULONGLONG             DbgCtlMsr  = 0;

    if (LbrCapacity == 0)
    {
        LogInfo("LBR: Aborting, CPU model not supported.\n");
        return FALSE;
    }

    //
    // For architectural LBR, convert filter options into IA32_LBR_CTL bit fields
    //
    if (g_ArchBasedLastBranchRecord)
    {
        LbrBuildArchBasedFilterOptions(FilterOptions, &Ia32LbrCtl);
    }

    //
    // For legacy LBR, write the raw filter options to MSR_LEGACY_LBR_SELECT
    // Architectural LBR encodes its filters in IA32_LBR_CTL and does not use this MSR
    //
    if (!g_ArchBasedLastBranchRecord)
    {
        LbrSetLbrSelectFilter(FilterOptions);
    }

    //
    // Clear hardware state before enabling LBR
    //
    LbrClearHardwareState();

    if (g_RunningOnHypervisorEnvironment)
    {
        IsOnVmxRootMode = g_Callbacks.VmFuncVmxGetCurrentExecutionMode();

        //
        // DEBUGCTL is not involved in ARCH LBR - it has its own dedicated control register
        // So for ARCH LBR we skip reading the previous value and build the register from scratch (replacing it)
        //
        if (!g_ArchBasedLastBranchRecord)
        {
            if (IsOnVmxRootMode)
            {
                LbrGetValuesOnVmxRootMode(&DbgCtlMsr, &Ia32LbrCtl);
            }
            else
            {
                LbrGetValuesOnVmxNonRootMode(&DbgCtlMsr, &Ia32LbrCtl);
            }
        }

        //
        // Apply the appropriate bit to enable LBR based on whether it is architectural LBR or legacy LBR
        //
        if (g_ArchBasedLastBranchRecord)
        {
            LbrEnableArchBased(&Ia32LbrCtl);
        }
        else
        {
            LbrEnableLegacyBased(&DbgCtlMsr);
        }

        //
        // Write the updated LBR control register values back based on the current VMX execution mode
        //
        if (IsOnVmxRootMode)
        {
            LbrSetValuesOnVmxRootMode(DbgCtlMsr, Ia32LbrCtl);
        }
        else
        {
            LbrSetValuesOnVmxNonRootMode(DbgCtlMsr, Ia32LbrCtl);
        }
    }
    else
    {
        LbrStartOnNativeMode(Ia32LbrCtl);
    }

    return TRUE;
}

/**
 * @brief Stop collecting LBR branches
 *
 * @return VOID
 */
VOID
LbrStop()
{
    BOOLEAN               IsOnVmxRootMode;
    ULONGLONG             DbgCtlMsr  = NULL64_ZERO;
    IA32_LBR_CTL_REGISTER Ia32LbrCtl = {0};

    if (g_RunningOnHypervisorEnvironment)
    {
        IsOnVmxRootMode = g_Callbacks.VmFuncVmxGetCurrentExecutionMode();

        //
        // Read the current LBR control register values based on the current VMX execution mode
        //
        if (IsOnVmxRootMode)
        {
            LbrGetValuesOnVmxRootMode(&DbgCtlMsr, &Ia32LbrCtl);
        }
        else
        {
            LbrGetValuesOnVmxNonRootMode(&DbgCtlMsr, &Ia32LbrCtl);
        }

        //
        // Apply the appropriate bit to disable LBR based on whether it is architectural LBR or legacy LBR
        //
        if (g_ArchBasedLastBranchRecord)
        {
            LbrDisableArchBased(&Ia32LbrCtl);
        }
        else
        {
            LbrDisableLegacyBased(&DbgCtlMsr);
        }

        //
        // Write the updated LBR control register values back based on the current VMX execution mode
        //
        if (IsOnVmxRootMode)
        {
            LbrSetValuesOnVmxRootMode(DbgCtlMsr, Ia32LbrCtl);
        }
        else
        {
            LbrSetValuesOnVmxNonRootMode(DbgCtlMsr, Ia32LbrCtl);
        }
    }
    else
    {
        LbrStopOnNativeMode();
    }
}

/**
 * @brief Flush LBR MSRs by disabling LBR and clearing all LBR entries
 *
 * @return VOID
 */
VOID
LbrFlush()
{
    LogInfo("Flush LBR on cpu core: %d\n", KeGetCurrentProcessorNumberEx(NULL));

    //
    // Stop LBR collection and save any remaining entries
    //
    LbrStop();

    //
    // Reset the LBR control registers to zero:
    // - ARCH LBR: zeroes IA32_LBR_CTL entirely (clears filters and disables collection)
    // - Legacy LBR: zeroes MSR_LEGACY_LBR_SELECT (resets branch filter to capture-all)
    //
    LbrResetControlRegisters();

    //
    // Clear all remaining hardware state (TOS for legacy LBR, and all from/to MSR pairs)
    //
    LbrClearHardwareState();
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
    // Note that there is no TOS index in ARCH LBR since everything is in order
    //
    if (!g_ArchBasedLastBranchRecord)
    {
        xrdmsr(MSR_LBR_TOS, &LbrTos);
        State->Tos = (UINT8)LbrTos;
    }

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
 * @brief Print all collected LBR branches
 *
 * @return VOID
 */
VOID
LbrPrintAll()
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
        if (g_ArchBasedLastBranchRecord)
        {
            //
            // In ARCH LBR, there is not TOS index and everything is in order
            //
            CurrentIdx = i - 1;
        }
        else
        {
            CurrentIdx = (ULONG)(State->Tos + i) % (ULONG)LbrCapacity;
        }

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
