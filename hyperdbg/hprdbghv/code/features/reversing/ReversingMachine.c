/**
 * @file ReversingMachine.c
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
_Use_decl_annotations_
BOOLEAN
ReversingMachineTraverseThroughOsPageTables(PVMM_EPT_PAGE_TABLE EptTable, CR3_TYPE TargetCr3, CR3_TYPE KernelCr3)
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
 * @brief Initialize the needed structure for hooking mode execution
 * @details should be called from vmx non-root mode
 *
 * @return BOOLEAN
 */
BOOLEAN
ReversingMachineAllocateMbecEptPageTable()
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
    ModeBasedExecHookDisableUsermodeExecution(ModeBasedEptTable);

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
ReversingMachineEnableExecuteOnlyPages(PVMM_EPT_PAGE_TABLE EptTable)
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

    return TRUE;
}

/**
 * @brief Initialize the needed structure for execute-only pages
 * @details should be called from vmx non-root mode
 *
 * @return BOOLEAN
 */
BOOLEAN
ReversingMachineAllocateExecuteOnlyEptPageTable()
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
    ReversingMachineEnableExecuteOnlyPages(ModeBasedEptTable);

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
 * @brief Read the RAM regions (physical address)
 *
 * @return VOID
 */
VOID
ReversingMachineReadRamPhysicalRegions()
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
ReversingMachineInitialize()
{
    //
    // Check if the reversing machine is already initialized
    //
    if (g_ReversingMachineInitialized)
    {
        //
        // Already initialized
        //
        return FALSE;
    }

    //
    // Read the RAM regions
    //
    ReversingMachineReadRamPhysicalRegions();

    //
    // Allocate MBEC EPT page-table
    //
    if (!ReversingMachineAllocateMbecEptPageTable())
    {
        //
        // There was an error allocating MBEC page table for EPT tables
        //
        return FALSE;
    }

    //
    // Allocate execute-only EPT page-table
    //
    if (!ReversingMachineAllocateExecuteOnlyEptPageTable())
    {
        //
        // There was an error allocating execute-only page table for EPT tables
        //
        return FALSE;
    }

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
    // Change EPT on all core's to a MBEC supported EPTP
    //
    BroadcastChangeToMbecSupportedEptpOnAllProcessors();

    //
    // Enable Mode-based execution control by broadcasting MOV to CR3 exiting
    //
    BroadcastEnableMovToCr3ExitingOnAllProcessors();

    //
    // Indicate that the reversing machine is initialized
    //
    g_ReversingMachineInitialized = TRUE;

    return TRUE;
}

/**
 * @brief Uinitialize the needed structure for the reversing machine
 * @details should be called from vmx non-root mode
 *
 * @return VOID
 */
VOID
ReversingMachineUninitialize()
{
    //
    // Indicate that the reversing machine is disabled
    //
    g_ReversingMachineInitialized = FALSE;

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
ReversingMachineRestoreToNormalEptp(VIRTUAL_MACHINE_STATE * VCpu)
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
ReversingMachineChangeToExecuteOnlyEptp(VIRTUAL_MACHINE_STATE * VCpu)
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
ReversingMachineChangeToMbecEnabledEptp(VIRTUAL_MACHINE_STATE * VCpu)
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
ReversingMachineHandleEptViolationVmexit(VIRTUAL_MACHINE_STATE *                VCpu,
                                         VMX_EXIT_QUALIFICATION_EPT_VIOLATION * ViolationQualification,
                                         UINT64                                 GuestPhysicalAddr)
{
    //
    // Check if this mechanism is use or not
    //
    if (!g_ReversingMachineInitialized)
    {
        return FALSE;
    }

    if (!ViolationQualification->EptReadable || !ViolationQualification->EptWriteable)
    {
        //
        // Show the disassembly of current instruction
        //
        // LogInfo("Tid: %x, Guest physical address: %llx, Virtual Address: %llx , RIP: %llx",
        //         PsGetCurrentThreadId(),
        //         GuestPhysicalAddr,
        //         PhysicalAddressToVirtualAddressByCr3(GuestPhysicalAddr, LayoutGetCurrentProcessCr3()),
        //         VCpu->LastVmexitRip);

        //
        // Supress the RIP increment
        //
        HvSuppressRipIncrement(VCpu);

        //
        // Check whether the current RIP instruction is in kernel-mode or user-mode
        //
        if (VCpu->LastVmexitRip & 0xff00000000000000)
        {
            //
            // Restore to normal state for current process
            //
            ReversingMachineRestoreNormalStateOnTargetProcess(VCpu);
        }
        else
        {
            //
            // Change to all enable EPTP
            //
            ReversingMachineRestoreToNormalEptp(VCpu);

            //
            // Set MTF
            // Note that external interrupts are previously masked
            //
            HvEnableMtfAndChangeExternalInterruptState(VCpu);

            //
            // Disable the user-mode execution interception
            //
            HvSetModeBasedExecutionEnableFlag(FALSE);

            //
            // Disassemble instructions
            //
            DisassemblerShowOneInstructionInVmxRootMode(VCpu->LastVmexitRip, FALSE);

            //
            // Set the indicator to handle MTF
            //
            VCpu->RestoreNonReadableWriteEptp = TRUE;
        }
    }
    else if (ViolationQualification->EptExecutable && !ViolationQualification->EptExecutableForUserMode)
    {
        //
        // For test purposes
        //
        // LogInfo("User-mode process (0x%x) is executed address: %llx", PsGetCurrentProcessId(), VCpu->LastVmexitRip);

        //
        // Supress the RIP increment
        //
        HvSuppressRipIncrement(VCpu);

        //
        // Prevent external-interrupts
        //
        HvPreventExternalInterrupts(VCpu);

        //
        // Mask all exceptions
        //
        HvWriteExceptionBitmap(EXCEPTION_BITMAP_MASK_ALL);

        //
        // Change EPTP to execute-only pages
        //
        ReversingMachineChangeToExecuteOnlyEptp(VCpu);
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
 * @brief Callback for handling VM-exits for MTF in the case of MBEC hooks
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
ReversingMachineHandleMtfCallback(VIRTUAL_MACHINE_STATE * VCpu)
{
    if (VCpu->TestNumber != 1000000)
    {
        //
        // Check for re-enabling external interrupts
        //
        if (VCpu->PendingExternalInterrupts[0] != NULL)
        {
            //
            // Restore to normal state for current process
            //
            ReversingMachineRestoreNormalStateOnTargetProcess(VCpu);
        }
        else
        {
            //
            // Restore non-readable/writeable EPTP
            //
            ReversingMachineChangeToExecuteOnlyEptp(VCpu);
        }

        VCpu->TestNumber++;
    }
    else
    {
        ReversingMachineRestoreToNormalEptp(VCpu);

        //
        // Check for reenabling external interrupts
        //
        HvEnableAndCheckForPreviousExternalInterrupts(VCpu);

        VCpu->Test = TRUE;
    }

    //
    // Unset the indicator to avoid further handling
    //
    VCpu->RestoreNonReadableWriteEptp = TRUE;
}

/**
 * @brief Restore to the normal state for the current process
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
ReversingMachineRestoreNormalStateOnTargetProcess(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Change to the MBEC Enabled EPTP
    //
    ReversingMachineChangeToMbecEnabledEptp(VCpu);

    //
    // Check for reenabling external interrupts
    //
    HvEnableAndCheckForPreviousExternalInterrupts(VCpu);

    //
    // Enable the user-mode execution interception
    //
    HvSetModeBasedExecutionEnableFlag(TRUE);

    //
    // Mask all exceptions
    //
    HvWriteExceptionBitmap(0x0);
}

/**
 * @brief Handle MOV to CR3 vm-exits for hooking mode execution
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
ReversingMachineHandleCr3Vmexit(VIRTUAL_MACHINE_STATE * VCpu)
{
    if (PsGetCurrentProcessId() == ReversingMachineProcessId && VCpu->Test == FALSE)
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

        HvWriteExceptionBitmap(0x0);

        //
        // Enable interrupts
        //
        HvEnableAndCheckForPreviousExternalInterrupts(VCpu);
    }
}

/**
 * @brief Add the process/thread to the watching list
 * @param VCpu The virtual processor's state
 * @param ProcessId
 * @param ThreadId
 *
 * @return VOID
 */
VOID
ReversingAddProcessThreadToTheWatchList(VIRTUAL_MACHINE_STATE * VCpu,
                                        UINT32                  ProcessId,
                                        UINT32                  ThreadId)
{
    //
    // Store the process id/thread id
    //
    ReversingMachineProcessId = ProcessId;
    ReversingMachineThreadId  = ThreadId;

    LogInfo("Process added to the watch list (pid: %x, tid: %x)", ProcessId, ThreadId);

    //
    // Here we're also in the target thread/process, so we set it to handle the
    // current process
    //
    // ReversingMachineHandleCr3Vmexit(VCpu);
}
