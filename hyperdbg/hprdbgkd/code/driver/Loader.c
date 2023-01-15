/**
 * @file Loader.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief The functions used in loading the debugger and VMM
 * @version 0.2
 * @date 2023-01-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Initialize the VMM and Debugger
 *
 * @return BOOLEAN
 */
BOOLEAN
LoaderInitVmmAndDebugger()
{
    //
    // Allow to server IOCTL
    //
    g_AllowIOCTLFromUsermode = TRUE;

    //
    // Initialize Vmx
    //
    if (VmFuncInitVmm())
    {
        LogDebugInfo("HyperDbg's hypervisor loaded successfully");

        //
        // Initialize the debugger
        //
        if (DebuggerInitialize())
        {
            LogDebugInfo("HyperDbg's debugger loaded successfully");

            //
            // Set the variable so no one else can get a handle anymore
            //
            g_HandleInUse = TRUE;

            return TRUE;
        }
        else
        {
            LogError("Err, HyperDbg's debugger was not loaded");
        }
    }
    else
    {
        LogError("Err, HyperDbg's hypervisor was not loaded :(");
    }

    //
    // Not loaded
    //
    g_AllowIOCTLFromUsermode = FALSE;

    return FALSE;
}

/**
 * @brief Unload the VMM and Debugger
 *
 * @return VOID
 */
VOID
LoaderUnloadVmmAndDebugger()
{
    ULONG                       ProcessorCount;
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggerState = NULL;

    ProcessorCount = KeQueryActiveProcessorCount(0);

    LogDebugInfo("Unloading HyperDbg's debugger...\n");

#if !UseDbgPrintInsteadOfUsermodeMessageTracking

    //
    // Uinitialize log buffer
    //
    LogDebugInfo("Uninitializing logs\n");
    LogUnInitialize();
#endif

    //
    // Free g_Events
    //
    GlobalEventsFreeMemory();

    //
    // Free g_ScriptGlobalVariables
    //
    if (g_ScriptGlobalVariables != NULL)
    {
        ExFreePoolWithTag(g_ScriptGlobalVariables, POOLTAG);
    }

    //
    // Free core specific local and temp variables
    //
    for (SIZE_T i = 0; i < ProcessorCount; i++)
    {
        CurrentDebuggerState = &g_DbgState[i];

        if (CurrentDebuggerState->ScriptEngineCoreSpecificLocalVariable != NULL)
        {
            ExFreePoolWithTag(CurrentDebuggerState->ScriptEngineCoreSpecificLocalVariable, POOLTAG);
        }

        if (CurrentDebuggerState->ScriptEngineCoreSpecificTempVariable != NULL)
        {
            ExFreePoolWithTag(CurrentDebuggerState->ScriptEngineCoreSpecificTempVariable, POOLTAG);
        }
    }

    //
    // Free g_DbgState
    //
    GlobalDebuggingStateFreeMemory();

    //
    // Free g_GuestState
    //
    GlobalGuestStateFreeMemory();
}
