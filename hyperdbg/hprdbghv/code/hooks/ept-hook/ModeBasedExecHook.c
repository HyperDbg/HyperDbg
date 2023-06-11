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
    //
    // Set execute access for PML4s
    //
    for (size_t i = 0; i < VMM_EPT_PML4E_COUNT; i++)
    {
        //
        // Execute-only pages
        //
        EptTable->PML4[i].ExecuteAccess   = TRUE;
        EptTable->PML4[i].UserModeExecute = TRUE;
        EptTable->PML4[i].ReadAccess      = FALSE;
        EptTable->PML4[i].WriteAccess     = FALSE;
    }

    //
    // Set execute access for PML3s
    //
    for (size_t i = 0; i < VMM_EPT_PML3E_COUNT; i++)
    {
        EptTable->PML3[i].ExecuteAccess   = TRUE;
        EptTable->PML3[i].UserModeExecute = TRUE;
        EptTable->PML3[i].ReadAccess      = FALSE;
        EptTable->PML3[i].WriteAccess     = FALSE;
    }

    //
    // Set execute access for PML2s
    //
    for (size_t i = 0; i < VMM_EPT_PML3E_COUNT; i++)
    {
        for (size_t j = 0; j < VMM_EPT_PML2E_COUNT; j++)
        {
            EptTable->PML2[i][j].ExecuteAccess   = TRUE;
            EptTable->PML2[i][j].UserModeExecute = TRUE;
            EptTable->PML2[i][j].ReadAccess      = FALSE;
            EptTable->PML2[i][j].WriteAccess     = FALSE;
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
 *
 * @return BOOLEAN
 */
BOOLEAN
ModeBasedExecHookHandleEptViolationVmexit(VIRTUAL_MACHINE_STATE * VCpu, VMX_EXIT_QUALIFICATION_EPT_VIOLATION * ViolationQualification)
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
        //
        // For test purposes
        //
        if (VCpu->LastVmexitRip & 0xff00000000000000)
        {
            //
            // We're not gonna trace kernel-mode at this stage
            //
            LogInfo("Went to the kernel and the (0x%x) is executed address: %llx",
                    PsGetCurrentProcessId(),
                    VCpu->LastVmexitRip);

            //
            // Disable MBEC again
            //
            HvSetModeBasedExecutionEnableFlag(FALSE);

            //
            // If we're not in normal EPT then switch  to it
            //
            ModeBasedExecHookRestoreToNormalEptp(VCpu);

            //
            // Supress the RIP increment
            //
            HvSuppressRipIncrement(VCpu);
        }
        else
        {
            // LogInfo("Access log (0x%x) is executed address: %llx", PsGetCurrentProcessId(), VCpu->LastVmexitRip);

            //
            // Show the disassembly of current instruction
            //
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
            HvEnableMtfAndChangeExternalInterruptState(VCpu);

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
        LogInfo("User-mode process (0x%x) is executed address: %llx",
                PsGetCurrentProcessId(),
                VCpu->LastVmexitRip);

        //
        // Disable MBEC again
        //
        HvSetModeBasedExecutionEnableFlag(FALSE);

        //
        // Supress the RIP increment
        //
        HvSuppressRipIncrement(VCpu);

        //
        // Change EPTP to execute-only pages
        //
        // ModeBasedExecHookChangeToExecuteOnlyEptp(VCpu);
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
 * @param NewCr3 New cr3
 *
 * @return VOID
 */
VOID
ModeBasedExecHookHandleCr3Vmexit(VIRTUAL_MACHINE_STATE * VCpu, UINT64 NewCr3)
{
    if (PsGetCurrentProcessId() == 7880)
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
