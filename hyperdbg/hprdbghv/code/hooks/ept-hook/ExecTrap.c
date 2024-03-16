/**
 * @file ExecTrap.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The reversing machine's routines
 * @details
 *
 * @version 0.4
 * @date 2023-07-05
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief This function gets virtual address and returns its PTE of the virtual address
 * based on the specific cr3 but without switching to the target address
 * @details the TargetCr3 should be kernel cr3 as we will use it to translate kernel
 * addresses so the kernel functions to translate addresses should be mapped; thus,
 * don't pass a KPTI meltdown user cr3 to this function
 *
 * @param Va Virtual Address
 * @param Level PMLx
 * @param TargetCr3 user/kernel cr3 of target process
 * @param KernelCr3 kernel cr3 of target process
 * @return PVOID virtual address of PTE based on cr3
 */
BOOLEAN
ExecTrapTraverseThroughOsPageTables(PVMM_EPT_PAGE_TABLE EptTable, CR3_TYPE TargetCr3, CR3_TYPE KernelCr3)
{
    CR3_TYPE Cr3;
    UINT64   TempCr3;
    PUINT64  Cr3Va;
    PUINT64  PdptVa;
    PUINT64  PdVa;
    PUINT64  PtVa;
    BOOLEAN  IsLargePage       = FALSE;
    CR3_TYPE CurrentProcessCr3 = {0};

    //
    // Move to guest process as we're currently in system cr3
    //
    CurrentProcessCr3 = SwitchToProcessMemoryLayoutByCr3(KernelCr3);

    Cr3.Flags = TargetCr3.Flags;

    //
    // Cr3 should be shifted 12 to the left because it's PFN
    //
    TempCr3 = Cr3.Fields.PageFrameNumber << 12;

    PVOID EptPmlEntry4 = EptGetPml1OrPml2Entry(EptTable, Cr3.Fields.PageFrameNumber << 12, &IsLargePage);

    if (EptPmlEntry4 != NULL)
    {
        if (IsLargePage)
        {
            ((PEPT_PML2_ENTRY)EptPmlEntry4)->ReadAccess  = TRUE;
            ((PEPT_PML2_ENTRY)EptPmlEntry4)->WriteAccess = TRUE;
        }
        else
        {
            ((PEPT_PML1_ENTRY)EptPmlEntry4)->ReadAccess  = TRUE;
            ((PEPT_PML1_ENTRY)EptPmlEntry4)->WriteAccess = TRUE;
        }
    }
    else
    {
        LogInfo("null address");
    }

    //
    // we need VA of Cr3, not PA
    //
    Cr3Va = (UINT64 *)PhysicalAddressToVirtualAddress(TempCr3);

    //
    // Check for invalid address
    //
    if (Cr3Va == NULL)
    {
        //
        // Restore the original process
        //
        SwitchToPreviousProcess(CurrentProcessCr3);

        return FALSE;
    }

    for (size_t i = 0; i < 512; i++)
    {
        // LogInfo("Address of Cr3Va: %llx", Cr3Va);

        PPAGE_ENTRY Pml4e = (PAGE_ENTRY *)&Cr3Va[i];

        if (Pml4e->Fields.Present)
        {
            // LogInfo("PML4[%d] = %llx", i, Pml4e->Fields.PageFrameNumber);

            IsLargePage  = FALSE;
            EptPmlEntry4 = EptGetPml1OrPml2Entry(EptTable, Pml4e->Fields.PageFrameNumber << 12, &IsLargePage);

            if (EptPmlEntry4 != NULL)
            {
                if (IsLargePage)
                {
                    ((PEPT_PML2_ENTRY)EptPmlEntry4)->ReadAccess  = TRUE;
                    ((PEPT_PML2_ENTRY)EptPmlEntry4)->WriteAccess = TRUE;
                }
                else
                {
                    ((PEPT_PML1_ENTRY)EptPmlEntry4)->ReadAccess  = TRUE;
                    ((PEPT_PML1_ENTRY)EptPmlEntry4)->WriteAccess = TRUE;
                }
            }
            else
            {
                LogInfo("null address");
            }

            PdptVa = (UINT64 *)PhysicalAddressToVirtualAddress(Pml4e->Fields.PageFrameNumber << 12);

            //
            // Check for invalid address
            //
            if (PdptVa != NULL)
            {
                for (size_t j = 0; j < 512; j++)
                {
                    // LogInfo("Address of PdptVa: %llx", PdptVa);

                    PPAGE_ENTRY Pdpte = (PAGE_ENTRY *)&PdptVa[j];

                    if (Pdpte->Fields.Present)
                    {
                        // LogInfo("PML3[%d] = %llx", j, Pdpte->Fields.PageFrameNumber);

                        IsLargePage        = FALSE;
                        PVOID EptPmlEntry3 = EptGetPml1OrPml2Entry(EptTable, Pdpte->Fields.PageFrameNumber << 12, &IsLargePage);

                        if (EptPmlEntry3 != NULL)
                        {
                            if (IsLargePage)
                            {
                                ((PEPT_PML2_ENTRY)EptPmlEntry3)->ReadAccess  = TRUE;
                                ((PEPT_PML2_ENTRY)EptPmlEntry3)->WriteAccess = TRUE;
                            }
                            else
                            {
                                ((PEPT_PML1_ENTRY)EptPmlEntry3)->ReadAccess  = TRUE;
                                ((PEPT_PML1_ENTRY)EptPmlEntry3)->WriteAccess = TRUE;
                            }
                        }
                        else
                        {
                            LogInfo("null address");
                        }

                        if (Pdpte->Fields.LargePage)
                        {
                            continue;
                        }

                        PdVa = (UINT64 *)PhysicalAddressToVirtualAddress(Pdpte->Fields.PageFrameNumber << 12);

                        //
                        // Check for invalid address
                        //
                        if (PdVa != NULL)
                        {
                            for (size_t k = 0; k < 512; k++)
                            {
                                // LogInfo("Address of PdVa: %llx", PdVa);

                                if (PdVa == (PUINT64)0xfffffffffffffe00)
                                {
                                    continue;
                                }

                                PPAGE_ENTRY Pde = (PAGE_ENTRY *)&PdVa[k];

                                if (Pde->Fields.Present)
                                {
                                    // LogInfo("PML2[%d] = %llx", k, Pde->Fields.PageFrameNumber);

                                    IsLargePage        = FALSE;
                                    PVOID EptPmlEntry2 = EptGetPml1OrPml2Entry(EptTable, Pde->Fields.PageFrameNumber << 12, &IsLargePage);

                                    if (EptPmlEntry2 != NULL)
                                    {
                                        if (IsLargePage)
                                        {
                                            ((PEPT_PML2_ENTRY)EptPmlEntry2)->ReadAccess  = TRUE;
                                            ((PEPT_PML2_ENTRY)EptPmlEntry2)->WriteAccess = TRUE;
                                        }
                                        else
                                        {
                                            ((PEPT_PML1_ENTRY)EptPmlEntry2)->ReadAccess  = TRUE;
                                            ((PEPT_PML1_ENTRY)EptPmlEntry2)->WriteAccess = TRUE;
                                        }
                                    }
                                    else
                                    {
                                        LogInfo("null address");
                                    }

                                    if (Pde->Fields.LargePage)
                                    {
                                        continue;
                                    }

                                    PtVa = (UINT64 *)PhysicalAddressToVirtualAddress(Pde->Fields.PageFrameNumber << 12);

                                    //
                                    // Check for invalid address
                                    //
                                    if (PtVa != NULL)
                                    {
                                        for (size_t l = 0; l < 512; l++)
                                        {
                                            // LogInfo("Address of PtVa: %llx", PtVa);

                                            // PPAGE_ENTRY Pt = &PtVa[l];

                                            /* if (Pt->Fields.Present)
                                            {
                                                // LogInfo("PML1[%d] = %llx", l, Pt->Fields.PageFrameNumber);
                                            }*/
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //
    // Restore the original process
    //
    SwitchToPreviousProcess(CurrentProcessCr3);

    return TRUE;
}

/**
 * @brief Initialize the needed structure for hooking user-mode execution
 * @details should be called from vmx non-root mode
 *
 * @return BOOLEAN
 */
BOOLEAN
ExecTrapAllocateUserDisabledMbecEptPageTable()
{
    PVMM_EPT_PAGE_TABLE ModeBasedEptTable;
    EPT_POINTER         EPTP = {0};

    //
    // Allocate another EPT page table
    //
    ModeBasedEptTable = EptAllocateAndCreateIdentityPageTable();

    if (ModeBasedEptTable == NULL)
    {
        //
        // There was an error allocating MBEC page tables
        //
        return FALSE;
    }

    //
    // Disable EPT user-mode execution bit for the target EPTP
    //
    ModeBasedExecHookDisableUserModeExecution(ModeBasedEptTable);

    //
    // Set the global address for MBEC EPT page table
    //
    g_EptState->ModeBasedUserDisabledEptPageTable = ModeBasedEptTable;

    //
    // For performance, we let the processor know it can cache the EPT
    //
    EPTP.MemoryType = MEMORY_TYPE_WRITE_BACK;

    //
    // We might utilize the 'access' and 'dirty' flag features in the dirty logging mechanism
    //
    EPTP.EnableAccessAndDirtyFlags = TRUE;

    //
    // Bits 5:3 (1 less than the EPT page-walk length) must be 3, indicating an EPT page-walk length of 4;
    // see Section 28.2.2
    //
    EPTP.PageWalkLength = 3;

    //
    // The physical page number of the page table we will be using
    //
    EPTP.PageFrameNumber = (SIZE_T)VirtualAddressToPhysicalAddress(&ModeBasedEptTable->PML4) / PAGE_SIZE;

    //
    // We will write the EPTP to the VMCS later
    //
    g_EptState->ModeBasedUserDisabledEptPointer.AsUInt = EPTP.AsUInt;

    return TRUE;
}

/**
 * @brief Initialize the needed structure for hooking kernel-mode execution
 * @details should be called from vmx non-root mode
 *
 * @return BOOLEAN
 */
BOOLEAN
ExecTrapAllocateKernelDisabledMbecEptPageTable()
{
    PVMM_EPT_PAGE_TABLE ModeBasedEptTable;
    EPT_POINTER         EPTP = {0};

    //
    // Allocate another EPT page table
    //
    ModeBasedEptTable = EptAllocateAndCreateIdentityPageTable();

    if (ModeBasedEptTable == NULL)
    {
        //
        // There was an error allocating MBEC page tables
        //
        return FALSE;
    }

    //
    // Disable EPT user-mode execution bit for the target EPTP
    //
    ModeBasedExecHookDisableKernelModeExecution(ModeBasedEptTable);

    //
    // Set the global address for MBEC EPT page table
    //
    g_EptState->ModeBasedKernelDisabledEptPageTable = ModeBasedEptTable;

    //
    // For performance, we let the processor know it can cache the EPT
    //
    EPTP.MemoryType = MEMORY_TYPE_WRITE_BACK;

    //
    // We might utilize the 'access' and 'dirty' flag features in the dirty logging mechanism
    //
    EPTP.EnableAccessAndDirtyFlags = TRUE;

    //
    // Bits 5:3 (1 less than the EPT page-walk length) must be 3, indicating an EPT page-walk length of 4;
    // see Section 28.2.2
    //
    EPTP.PageWalkLength = 3;

    //
    // The physical page number of the page table we will be using
    //
    EPTP.PageFrameNumber = (SIZE_T)VirtualAddressToPhysicalAddress(&ModeBasedEptTable->PML4) / PAGE_SIZE;

    //
    // We will write the EPTP to the VMCS later
    //
    g_EptState->ModeBasedKernelDisabledEptPointer.AsUInt = EPTP.AsUInt;

    return TRUE;
}

/**
 * @brief Adjust execute-only bits bit of target page-table
 * @details should be called from vmx non-root mode
 * @param EptTable
 *
 * @return BOOLEAN
 */
BOOLEAN
ExecTrapEnableExecuteOnlyPages(PVMM_EPT_PAGE_TABLE EptTable)
{
    INT64  RemainingSize  = 0;
    UINT64 CurrentAddress = 0;

    //
    // *** allow execution of user-mode pages in execute only EPTP ***
    //

    //
    // Set execute access for PML4s
    //
    for (size_t i = 0; i < VMM_EPT_PML4E_COUNT; i++)
    {
        //
        // We only set the top-level PML4 for intercepting user-mode execution
        //
        EptTable->PML4[i].UserModeExecute = TRUE;
    }

    //
    // Set execute access for PML3s
    //
    for (size_t i = 0; i < VMM_EPT_PML3E_COUNT; i++)
    {
        EptTable->PML3[i].UserModeExecute = TRUE;
    }

    //
    // Set execute access for PML2s
    //
    for (size_t i = 0; i < VMM_EPT_PML3E_COUNT; i++)
    {
        for (size_t j = 0; j < VMM_EPT_PML2E_COUNT; j++)
        {
            EptTable->PML2[i][j].UserModeExecute = TRUE;
        }
    }

    //
    // *** disallow read or write for certain memory only (not MMIO) EPTP pages ***
    //
    for (size_t i = 0; i < MAX_PHYSICAL_RAM_RANGE_COUNT; i++)
    {
        if (PhysicalRamRegions[i].RamPhysicalAddress != NULL64_ZERO)
        {
            RemainingSize  = (INT64)PhysicalRamRegions[i].RamSize;
            CurrentAddress = PhysicalRamRegions[i].RamPhysicalAddress;

            while (RemainingSize > 0)
            {
                //
                // Get the target entry in EPT table (every entry is 2-MB granularity)
                //
                PEPT_PML2_ENTRY EptEntry = EptGetPml2Entry(EptTable, CurrentAddress);
                EptEntry->WriteAccess    = FALSE;

                //
                // Move to the new address
                //
                CurrentAddress += SIZE_2_MB;
                RemainingSize -= SIZE_2_MB;
            }
        }
    }

    return TRUE;
}

/**
 * @brief Read the RAM regions (physical address)
 *
 * @return VOID
 */
VOID
ExecTrapReadRamPhysicalRegions()
{
    PHYSICAL_ADDRESS       Address;
    LONGLONG               Size;
    UINT32                 Count                = 0;
    PPHYSICAL_MEMORY_RANGE PhysicalMemoryRanges = NULL;

    //
    // Read the RAM regions (BIOS) gives these details to Windows
    //
    PhysicalMemoryRanges = MmGetPhysicalMemoryRanges();

    do
    {
        Address.QuadPart = PhysicalMemoryRanges[Count].BaseAddress.QuadPart;
        Size             = PhysicalMemoryRanges[Count].NumberOfBytes.QuadPart;

        if (!Address.QuadPart && !Size)
        {
            break;
        }

        // LogInfo("RAM Range, from: %llx to %llx", Address.QuadPart, Address.QuadPart + Size);

        PhysicalRamRegions[Count].RamPhysicalAddress = Address.QuadPart;
        PhysicalRamRegions[Count].RamSize            = Size;

    } while (++Count < MAX_PHYSICAL_RAM_RANGE_COUNT);

    ExFreePool(PhysicalMemoryRanges);
}

/**
 * @brief Initialize the reversing machine based on service request
 *
 * @return BOOLEAN
 */
BOOLEAN
ExecTrapInitialize()
{
    //
    // Check if the reversing machine is already initialized
    //
    if (g_ExecTrapInitialized)
    {
        //
        // Already initialized
        //
        return FALSE;
    }

    //
    // Check if MBEC supported by this processors
    //
    if (!g_CompatibilityCheck.ModeBasedExecutionSupport)
    {
        LogInfo("Your processor doesn't support Mode-Based Execution Controls (MBEC), which is a needed feature for this functionality :(\n"
                "MBEC is available on processors starting from the 7th generation (Kaby Lake) and onwards");
        return FALSE;
    }

    //
    // Read the RAM regions
    //
    ExecTrapReadRamPhysicalRegions();

    //
    // Allocate MBEC EPT page-table (user-disabled)
    //
    if (!ExecTrapAllocateUserDisabledMbecEptPageTable())
    {
        //
        // There was an error allocating MBEC page table for EPT tables
        //
        return FALSE;
    }

    //
    // Allocate MBEC EPT page-table (kernel-disabled)
    //
    if (!ExecTrapAllocateKernelDisabledMbecEptPageTable())
    {
        //
        // Free the user-disabled page-table buffer
        //
        MmFreeContiguousMemory(g_EptState->ModeBasedUserDisabledEptPageTable);
        g_EptState->ModeBasedUserDisabledEptPageTable = NULL;

        //
        // There was an error allocating MBEC page table for EPT tables
        //
        return FALSE;
    }

    //
    // Call the function responsible for initializing Mode-based hooks
    //
    if (ModeBasedExecHookInitialize() == FALSE)
    {
        //
        // Free the user-disabled page-table buffer
        //
        MmFreeContiguousMemory(g_EptState->ModeBasedUserDisabledEptPageTable);
        g_EptState->ModeBasedUserDisabledEptPageTable = NULL;

        //
        // Free the kernel-disabled page-table buffer
        //
        MmFreeContiguousMemory(g_EptState->ModeBasedKernelDisabledEptPageTable);
        g_EptState->ModeBasedKernelDisabledEptPageTable = NULL;

        //
        // The initialization was not successful
        //
        return FALSE;
    }

    //
    // Change EPT on all core's to a MBEC supported EPTP
    // (No longer needed as the starting phase of the process uses EPT hooks)
    //
    BroadcastChangeToMbecSupportedEptpOnAllProcessors();

    //
    // Indicate that the reversing machine is initialized
    // It should be initialized here BEFORE broadcasting mov 2 cr3 exiting
    // because an EPT violation might be thrown before we enabled it from
    // here
    //
    g_ExecTrapInitialized = TRUE;

    //
    // Enable Mode-based execution control by broadcasting MOV to CR3 exiting
    //
    BroadcastEnableMovToCr3ExitingOnAllProcessors();

    return TRUE;
}

/**
 * @brief Uinitialize the needed structure for the reversing machine
 * @details should be called from vmx non-root mode
 *
 * @return VOID
 */
VOID
ExecTrapUninitialize()
{
    //
    // Check if it's already initialized
    //
    if (!g_ExecTrapInitialized)
    {
        return;
    }

    //
    // Indicate that the uninitialization phase started
    //
    g_ExecTrapUnInitializationStarted = TRUE;

    //
    // Disable MOV to CR3 exiting
    //
    BroadcastDisableMovToCr3ExitingOnAllProcessors();

    //
    // Restore to normal EPTP
    //
    BroadcastRestoreToNormalEptpOnAllProcessors();

    //
    // Uninitialize the mode-based execution controls
    //
    ModeBasedExecHookUninitialize();

    //
    // Indicate that the execution traps are disabled
    //
    g_ExecTrapInitialized = FALSE;

    //
    // Indicate that the uninitialization phase finished
    //
    g_ExecTrapUnInitializationStarted = FALSE;

    //
    // Free Identity Page Table for MBEC hooks (user-disabled)
    //
    if (g_EptState->ModeBasedUserDisabledEptPageTable != NULL)
    {
        MmFreeContiguousMemory(g_EptState->ModeBasedUserDisabledEptPageTable);
        g_EptState->ModeBasedUserDisabledEptPageTable = NULL;
    }

    //
    // Free Identity Page Table for MBEC hooks (kernel-disabled)
    //
    if (g_EptState->ModeBasedKernelDisabledEptPageTable != NULL)
    {
        MmFreeContiguousMemory(g_EptState->ModeBasedKernelDisabledEptPageTable);
        g_EptState->ModeBasedKernelDisabledEptPageTable = NULL;
    }
}

/**
 * @brief restore to normal EPTP
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
ExecTrapRestoreToNormalEptp(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Change EPTP
    //
    __vmx_vmwrite(VMCS_CTRL_EPT_POINTER, VCpu->EptPointer.AsUInt);

    //
    // It's on normal EPTP
    //
    VCpu->NotNormalEptp = FALSE;
}

/**
 * @brief change to execute-only EPTP
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
ExecTrapChangeToExecuteOnlyEptp(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Change EPTP
    //
    __vmx_vmwrite(VMCS_CTRL_EPT_POINTER, g_EptState->ExecuteOnlyEptPointer.AsUInt);

    //
    // It's not on normal EPTP
    //
    VCpu->NotNormalEptp = TRUE;
}

/**
 * @brief change to user-disabled MBEC EPTP
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
ExecTrapChangeToUserDisabledMbecEptp(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Change EPTP
    //
    __vmx_vmwrite(VMCS_CTRL_EPT_POINTER, g_EptState->ModeBasedUserDisabledEptPointer.AsUInt);

    //
    // It's not on normal EPTP
    //
    VCpu->NotNormalEptp = TRUE;
}

/**
 * @brief change to kernel-disabled MBEC EPTP
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
ExecTrapChangeToKernelDisabledMbecEptp(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Change EPTP
    //
    __vmx_vmwrite(VMCS_CTRL_EPT_POINTER, g_EptState->ModeBasedKernelDisabledEptPointer.AsUInt);

    //
    // It's not on normal EPTP
    //
    VCpu->NotNormalEptp = TRUE;
}

/**
 * @brief Restore the execution of the trap to adjusted trap state
 * @param VCpu The virtual processor's state
 * @param IsUserMode Whether the execution event caused by a switch from kernel-to-user
 * or otherwise user-to-kernel
 *
 * @return VOID
 */
VOID
ExecTrapHandleMoveToAdjustedTrapState(VIRTUAL_MACHINE_STATE * VCpu, DEBUGGER_EVENT_MODE_TYPE TargetMode)
{
    if (TargetMode == DEBUGGER_EVENT_MODE_TYPE_USER_MODE)
    {
        //
        // Change EPT to kernel disabled
        //
        ExecTrapChangeToKernelDisabledMbecEptp(VCpu);
    }
    else if (TargetMode == DEBUGGER_EVENT_MODE_TYPE_KERNEL_MODE)
    {
        //
        // Change EPT to user disabled
        //
        ExecTrapChangeToUserDisabledMbecEptp(VCpu);
    }
}

/**
 * @brief Handle EPT Violations related to the MBEC hooks
 * @param VCpu The virtual processor's state
 * @param ViolationQualification
 *
 * @return BOOLEAN
 */
BOOLEAN
ExecTrapHandleEptViolationVmexit(VIRTUAL_MACHINE_STATE *                VCpu,
                                 VMX_EXIT_QUALIFICATION_EPT_VIOLATION * ViolationQualification)
{
    //
    // Check if this mechanism is use or not
    //
    if (!g_ExecTrapInitialized)
    {
        return FALSE;
    }

    if (!ViolationQualification->EptExecutableForUserMode && ViolationQualification->ExecuteAccess)
    {
        //
        // For test purposes
        //
        // LogInfo("Reached to the user-mode of the process (0x%x) is executed address: %llx", PsGetCurrentProcessId(), VCpu->LastVmexitRip);

        //
        // Suppress the RIP increment
        //
        HvSuppressRipIncrement(VCpu);

        //
        // Trigger the event
        //
        DispatchEventMode(VCpu, DEBUGGER_EVENT_MODE_TYPE_USER_MODE, TRUE);

        return TRUE;
    }
    else if (!ViolationQualification->EptExecutable && ViolationQualification->ExecuteAccess)
    {
        //
        // For test purposes
        //
        // LogInfo("Reached to the kernel-mode of the process (0x%x) is executed address: %llx", PsGetCurrentProcessId(), VCpu->LastVmexitRip);

        //
        // Suppress the RIP increment
        //
        HvSuppressRipIncrement(VCpu);

        //
        // Trigger the event
        //
        DispatchEventMode(VCpu, DEBUGGER_EVENT_MODE_TYPE_KERNEL_MODE, TRUE);
    }
    else
    {
        //
        // Unexpected violation
        //
        return FALSE;
    }

    //
    // It successfully handled by MBEC hooks
    //
    return TRUE;
}

/**
 * @brief Handle MOV to CR3 vm-exits for hooking mode execution
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
ExecTrapHandleCr3Vmexit(VIRTUAL_MACHINE_STATE * VCpu)
{
    BOOLEAN Result;
    UINT32  Index;

    //
    // Search the list of processes for the current process's user-execution
    //  trap state
    //
    Result = BinarySearchPerformSearchItem(&g_ExecTrapState.InterceptionProcessIds[0],
                                           g_ExecTrapState.NumberOfItems,
                                           &Index,
                                           (UINT64)PsGetCurrentProcessId());

    //
    // Check whether the procerss is in the list of interceptions or not
    //
    if (Result)
    {
        //
        // Enable MBEC to detect execution in user-mode
        //
        HvSetModeBasedExecutionEnableFlag(TRUE);
        VCpu->MbecEnabled = TRUE;

        //
        // Trigger the event
        //
        DispatchEventMode(VCpu, DEBUGGER_EVENT_MODE_TYPE_KERNEL_MODE, TRUE);
    }
    else if (VCpu->MbecEnabled)
    {
        //
        // In case, the process is changed, we've disable the MBEC
        //
        HvSetModeBasedExecutionEnableFlag(FALSE);
        VCpu->MbecEnabled = FALSE;
    }
}

/**
 * @brief Add the target process to the watching list
 * @param ProcessId
 *
 * @return BOOLEAN
 */
BOOLEAN
ExecTrapAddProcessToWatchingList(UINT32 ProcessId)
{
    return InsertionSortInsertItem(&g_ExecTrapState.InterceptionProcessIds[0],
                                   &g_ExecTrapState.NumberOfItems,
                                   MAXIMUM_NUMBER_OF_PROCESSES_FOR_USER_KERNEL_EXEC_THREAD,
                                   (UINT64)ProcessId);
}

/**
 * @brief Remove the target process from the watching list
 * @param ProcessId
 *
 * @return BOOLEAN
 */
BOOLEAN
ExecTrapRemoveProcessFromWatchingList(UINT32 ProcessId)
{
    return InsertionSortDeleteItem(&g_ExecTrapState.InterceptionProcessIds[0],
                                   &g_ExecTrapState.NumberOfItems,
                                   (UINT64)ProcessId);
}
