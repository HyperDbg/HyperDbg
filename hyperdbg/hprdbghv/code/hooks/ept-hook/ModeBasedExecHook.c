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
ModeBasedExecHookDisableUserModeExecution(PVMM_EPT_PAGE_TABLE EptTable)
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

    return TRUE;
}

/**
 * @brief Adjust (unset) kernel-mode execution bit of target page-table
 * but allow user-mode execution
 * @details should be called from vmx non-root mode
 * @param EptTable
 *
 * @return BOOLEAN
 */
BOOLEAN
ModeBasedExecHookDisableKernelModeExecution(PVMM_EPT_PAGE_TABLE EptTable)
{
    //
    // From Intel Manual:
    // [Bit 2] If the "mode-based execute control for EPT" VM - execution control is 0, execute access;
    // indicates whether instruction fetches are allowed from the 2-MByte page controlled by this entry.
    // If that control is 1, execute access for supervisor-mode linear addresses; indicates whether instruction
    // fetches are allowed from supervisor - mode linear addresses in the 2 - MByte page controlled by this entry
    //

    //
    // Set execute access for PML4s
    //
    for (size_t i = 0; i < VMM_EPT_PML4E_COUNT; i++)
    {
        EptTable->PML4[i].UserModeExecute = TRUE;

        //
        // We only set the top-level PML4 for intercepting kernel-mode execution
        //
        EptTable->PML4[i].ExecuteAccess = FALSE;
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

    return TRUE;
}

/**
 * @brief Enables user-mode execution bit of target page-table
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

    return TRUE;
}

/**
 * @brief Enable/disable MBEC
 * @param VCpu The virtual processor's state
 *
 * @return VOID
 */
VOID
ModeBasedExecHookEnableOrDisable(VIRTUAL_MACHINE_STATE * VCpu, UINT32 State)
{
    if (State == 0x0)
    {
        HvSetModeBasedExecutionEnableFlag(FALSE);

        //
        // MBEC is not enabled anymore!
        //
        VCpu->MbecEnabled = FALSE;
    }
    else
    {
        HvSetModeBasedExecutionEnableFlag(TRUE);
    }
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
    ULONG ProcessorsCount;

    //
    // Get number of processors
    //
    ProcessorsCount = KeQueryActiveProcessorCount(0);

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
    if (g_ModeBasedExecutionControlState)
    {
        return FALSE;
    }

    //
    // Enable EPT user-mode execution bit for the target EPTP
    //
    for (size_t i = 0; i < ProcessorsCount; i++)
    {
        ModeBasedExecHookEnableUsermodeExecution(g_GuestState[i].EptPageTable);
    }

    //
    // Invalidate ALL context on all cores (not necessary here because we will change
    // it later but let's respect memory primitives)
    //
    BroadcastNotifyAllToInvalidateEptAllCores();

    //
    // Indicate that MBEC is initialized
    //
    g_ModeBasedExecutionControlState = TRUE;

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
    // Broadcast to disable MBEC on all cores
    //
    BroadcasDisableMbecOnAllProcessors();

    //
    // Indicate that MBEC is disabled
    //
    g_ModeBasedExecutionControlState = FALSE;
}
