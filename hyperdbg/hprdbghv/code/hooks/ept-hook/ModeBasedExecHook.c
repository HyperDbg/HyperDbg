/**
 * @file ModeBasedExecHook.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of hooks based on Mode-based execution
 * @details
 *
 * @version 0.2
 * @date 2023-03-16
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
_Use_decl_annotations_
BOOLEAN
ModeBasedExecTest(PVMM_EPT_PAGE_TABLE EptTable, CR3_TYPE TargetCr3, CR3_TYPE KernelCr3)
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
    Cr3Va = PhysicalAddressToVirtualAddress(TempCr3);

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

        PPAGE_ENTRY Pml4e = &Cr3Va[i];

        if (Pml4e->Fields.Present)
        {
            // LogInfo("PML4[%d] = %llx", i, Pml4e->Fields.PageFrameNumber);

            IsLargePage        = FALSE;
            PVOID EptPmlEntry4 = EptGetPml1OrPml2Entry(EptTable, Pml4e->Fields.PageFrameNumber << 12, &IsLargePage);

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

            PdptVa = PhysicalAddressToVirtualAddress(Pml4e->Fields.PageFrameNumber << 12);

            //
            // Check for invalid address
            //
            if (PdptVa != NULL)
            {
                for (size_t j = 0; j < 512; j++)
                {
                    // LogInfo("Address of PdptVa: %llx", PdptVa);

                    PPAGE_ENTRY Pdpte = &PdptVa[j];

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

                        PdVa = PhysicalAddressToVirtualAddress(Pdpte->Fields.PageFrameNumber << 12);

                        //
                        // Check for invalid address
                        //
                        if (PdVa != NULL)
                        {
                            for (size_t k = 0; k < 512; k++)
                            {
                                // LogInfo("Address of PdVa: %llx", PdVa);

                                if (PdVa == 0xfffffffffffffe00)
                                {
                                    continue;
                                }

                                PPAGE_ENTRY Pde = &PdVa[k];

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

                                    PtVa = PhysicalAddressToVirtualAddress(Pde->Fields.PageFrameNumber << 12);

                                    //
                                    // Check for invalid address
                                    //
                                    if (PtVa != NULL)
                                    {
                                        for (size_t l = 0; l < 512; l++)
                                        {
                                            // LogInfo("Address of PtVa: %llx", PtVa);

                                            PPAGE_ENTRY Pt = &PtVa[l];

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Adjust (unset) user-mode execution bit of target page-table
 * @details should be called from vmx non-root mode
 * @param EptTable
 *
 * @return BOOLEAN
 */
BOOLEAN
ModeBasedExecHookEnableUsermodeExecution(PVMM_EPT_PAGE_TABLE EptTable)
{
    //
    // Set execute access for PML4s
    //
    for (size_t i = 0; i < VMM_EPT_PML4E_COUNT; i++)
    {
        //
        // We only set the top-level PML4 for intercepting user-mode execution
        //
        EptTable->PML4[i].UserModeExecute = FALSE;
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
}

/**
 * @brief Initialize the needed structure for hooking mode execution
 * @details should be called from vmx non-root mode
 *
 * @return BOOLEAN
 */
BOOLEAN
ModeBasedExecHookAllocateMbecEptPageTable()
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
    // Enable all EPT user-mode execution bit
    //
    ModeBasedExecHookEnableUsermodeExecution(ModeBasedEptTable);

    //
    // Set the global address for MBEC EPT page table
    //
    g_EptState->ModeBasedEptPageTable = ModeBasedEptTable;

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
    g_EptState->ModeBasedEptPointer.AsUInt = EPTP.AsUInt;

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
ModeBasedExecHookEnableExecuteOnlyPages(PVMM_EPT_PAGE_TABLE EptTable)
{
    INT64  RemainingSize  = 0;
    UINT64 CurrentAddress = 0;

    for (size_t i = 0; i < MAX_PHYSICAL_RAM_RANGE_COUNT; i++)
    {
        if (PhysicalRamRegions[i].RamPhysicalAddress != NULL)
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
}

/**
 * @brief Initialize the needed structure for execute-only pages
 * @details should be called from vmx non-root mode
 *
 * @return BOOLEAN
 */
BOOLEAN
ModeBasedExecHookAllocateExecuteOnlyEptPageTable()
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
    // Enable all execute-only bit
    //
    ModeBasedExecHookEnableExecuteOnlyPages(ModeBasedEptTable);

    //
    // Set the global address for execute only EPT page table
    //
    g_EptState->ExecuteOnlyEptPageTable = ModeBasedEptTable;

    //
    // For performance, we let the processor know it can cache the EPT
    //
    EPTP.MemoryType = MEMORY_TYPE_WRITE_BACK;

    //
    // We won't utilize the 'access' and 'dirty' flags
    //
    EPTP.EnableAccessAndDirtyFlags = FALSE;

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
    g_EptState->ExecuteOnlyEptPointer.AsUInt = EPTP.AsUInt;

    return TRUE;
}

/**
 * @brief Initialize the needed structure for hooking mode execution
 * @details should be called from vmx non-root mode
 *
 * @return BOOLEAN
 */
BOOLEAN
ModeBasedExecHookInitialize()
{
    //
    // Check if MBEC supported by this processors
    //
    if (!g_CompatibilityCheck.ModeBasedExecutionSupport)
    {
        return FALSE;
    }

    //
    // Check if execute-only feature is supported on this processor or not
    //
    if (!g_CompatibilityCheck.ExecuteOnlySupport)
    {
        return FALSE;
    }

    //
    // Check if it's already initialized or not
    //
    if (g_CheckForModeBasedExecutionControl)
    {
        return FALSE;
    }

    //
    // Allocate MBEC EPT page-table
    //
    if (!ModeBasedExecHookAllocateMbecEptPageTable())
    {
        //
        // There was an error allocating MBEC page table for EPT tables
        //
        return FALSE;
    }

    //
    // Allocate execute-only EPT page-table
    //
    if (!ModeBasedExecHookAllocateExecuteOnlyEptPageTable())
    {
        //
        // There was an error allocating execute-only page table for EPT tables
        //
        return FALSE;
    }

    //
    // Enable the interception of Cr3 to change the EPTP in the case of reaching to
    // the target process
    //
    g_CheckForModeBasedExecutionControl = TRUE;

    //
    // Change EPT on all core's to a MBEC supported EPTP
    //
    BroadcastChangeToMbecSupportedEptpOnAllProcessors();

    //
    // Enable Mode-based execution control by broadcasting MOV to CR3 exiting
    //
    BroadcastEnableMovToCr3ExitingOnAllProcessors();

    return TRUE;
}

/**
 * @brief Uinitialize the needed structure for hooking mode execution
 * @details should be called from vmx non-root mode
 *
 * @return VOID
 */
VOID
ModeBasedExecHookUninitialize()
{
    //
    // Disable the interception of Cr3 to change the EPTP in the case of reaching to
    // the target process
    //
    g_CheckForModeBasedExecutionControl = FALSE;

    //
    // Disable MOV to CR3 exiting
    //
    BroadcastDisableMovToCr3ExitingOnAllProcessors();

    //
    // Restore to normal EPTP
    //
    BroadcastRestoreToNormalEptpOnAllProcessors();

    //
    // Free Identity Page Table for MBEC hooks
    //
    if (g_EptState->ModeBasedEptPageTable != NULL)
    {
        MmFreeContiguousMemory(g_EptState->ModeBasedEptPageTable);
    }

    //
    // Free Identity Page Table for execute-only hooks
    //
    if (g_EptState->ExecuteOnlyEptPageTable != NULL)
    {
        MmFreeContiguousMemory(g_EptState->ExecuteOnlyEptPageTable);
    }
}

/**
 * @brief restore to normal EPTP
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
ModeBasedExecHookRestoreToNormalEptp(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Change EPTP
    //
    __vmx_vmwrite(VMCS_CTRL_EPT_POINTER, g_EptState->EptPointer.AsUInt);

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
ModeBasedExecHookChangeToExecuteOnlyEptp(VIRTUAL_MACHINE_STATE * VCpu)
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
 * @brief change to MBEC enabled EPTP
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
ModeBasedExecHookChangeToMbecEnabledEptp(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Change EPTP
    //
    __vmx_vmwrite(VMCS_CTRL_EPT_POINTER, g_EptState->ModeBasedEptPointer.AsUInt);

    //
    // It's not on normal EPTP
    //
    VCpu->NotNormalEptp = TRUE;
}

/**
 * @brief Handle EPT Violations related to the MBEC hooks
 * @param VCpu The virtual processor's state
 * @param ViolationQualification
 * @param GuestPhysicalAddr
 *
 * @return BOOLEAN
 */
BOOLEAN
ModeBasedExecHookHandleEptViolationVmexit(VIRTUAL_MACHINE_STATE *                VCpu,
                                          VMX_EXIT_QUALIFICATION_EPT_VIOLATION * ViolationQualification,
                                          UINT64                                 GuestPhysicalAddr)
{
    //
    // Check if this mechanism is use or not
    //
    if (!g_CheckForModeBasedExecutionControl)
    {
        return FALSE;
    }

    if (!ViolationQualification->EptReadable || !ViolationQualification->EptWriteable)
    {
        // LogInfo("Access log (0x%x) is executed address: %llx", PsGetCurrentProcessId(), VCpu->LastVmexitRip);

        //
        // Show the disassembly of current instruction
        //
        /* LogInfo("Tid: %x, Guest physical address: %llx, Virtual Address: %llx , RIP: %llx",
                PsGetCurrentThreadId(),
                GuestPhysicalAddr,
                PhysicalAddressToVirtualAddressByCr3(GuestPhysicalAddr, LayoutGetCurrentProcessCr3()),
                VCpu->LastVmexitRip);
                */
        DisassemblerShowOneInstructionInVmxRootMode(VCpu->LastVmexitRip, FALSE);

        //
        // Change to all enable EPTP
        //
        ModeBasedExecHookRestoreToNormalEptp(VCpu);

        //
        // Supress the RIP increment
        //
        HvSuppressRipIncrement(VCpu);

        //
        // Set MTF and adjust external interrupts
        //
        // HvSetExternalInterruptExiting(VCpu, FALSE);
        // HvSetInterruptWindowExiting(TRUE);

        // HvWriteExceptionBitmap(0xffffffff);
        // VCpu->ModeBasedHookIgnoreInterruptAndExceptions = FALSE;
        HvEnableMtfAndChangeExternalInterruptState(VCpu);

        //
        // Set the indicator to handle MTF
        //
        VCpu->RestoreNonReadableWriteEptp = TRUE;
    }
    else if (ViolationQualification->EptExecutable && !ViolationQualification->EptExecutableForUserMode)
    {
        //
        // For test purposes
        //
        /* LogInfo("User-mode process (0x%x) is executed address: %llx",
                PsGetCurrentProcessId(),
                VCpu->LastVmexitRip); */

        //
        // Show the disassembly of current instruction
        //
        DisassemblerShowOneInstructionInVmxRootMode(VCpu->LastVmexitRip, FALSE);

        //
        // Disable MBEC again
        //
        HvSetModeBasedExecutionEnableFlag(FALSE);

        //
        // Supress the RIP increment
        //
        HvSuppressRipIncrement(VCpu);

        /////////////////////////////////////////////////////////////////

        //
        // Change guest interrupt-state
        //
        HvSetExternalInterruptExiting(VCpu, TRUE);

        //
        // Do not vm-exit on interrupt windows
        //
        HvSetInterruptWindowExiting(FALSE);

        //
        // Indicate that we should enable external interrupts and configure external interrupt
        // window exiting somewhere at MTF
        //
        VCpu->EnableExternalInterruptsOnContinueMtf = TRUE;

        //  HvWriteExceptionBitmap(0xffffffff);

        // VCpu->Test = TRUE;
        // VCpu->ModeBasedHookIgnoreInterruptAndExceptions = TRUE;

        //
        // Change EPTP to execute-only pages
        //
        ModeBasedExecHookChangeToExecuteOnlyEptp(VCpu);

        /////////////////////////////////////////////////////////////////
    }
    else
    {
        //
        // Unexpected violation
        //
        LogInfo("Testtttt: should be removed here");
        DbgBreakPoint();
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
 * @param NewCr3 New cr3
 *
 * @return VOID
 */
VOID
ModeBasedExecHookHandleCr3Vmexit(VIRTUAL_MACHINE_STATE * VCpu, UINT64 NewCr3)
{
    if (PsGetCurrentProcessId() == 10344 && VCpu->Test == FALSE)
    {
        //
        // Enable MBEC to detect execution in user-mode
        //
        HvSetModeBasedExecutionEnableFlag(TRUE);
    }
    else
    {
        //
        // In case, the process is changed, we've disable the MBEC
        //
        HvSetModeBasedExecutionEnableFlag(FALSE);

        //
        // As we need to make other processes to normally behave, so
        // we restore the EPTP to normal EPTP if it's not
        //
        if (VCpu->NotNormalEptp)
        {
            //
            // This function itself sets the flag to FALSE
            //
            ModeBasedExecHookRestoreToNormalEptp(VCpu);
        }
    }
}

/**
 * @brief Read the RAM regions (physical address)
 *
 * @return VOID
 */
VOID
ModeBasedExecHookReadRamPhysicalRegions()
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
 * @param RevServiceRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
ModeBasedExecHookReversingMachineInitialize(PREVERSING_MACHINE_RECONSTRUCT_MEMORY_REQUEST RevServiceRequest)
{
    //
    // Read the RAM regions
    //
    ModeBasedExecHookReadRamPhysicalRegions();

    //
    // Call the function responsible for initializing Mode-based hooks
    //
    if (ModeBasedExecHookInitialize() == FALSE)
    {
        //
        // The initialization was not successfull
        //
        return FALSE;
    }

    //
    // Set the error/success code
    //
    RevServiceRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    return TRUE;
}
