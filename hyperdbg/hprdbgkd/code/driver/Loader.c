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
 * @brief Load the VMM and Debugger
 *
 * @return VOID
 */
BOOLEAN
LoaderInitVmmAndDebugger()
{
    LogDebugInfo("Starting HyperDbg...");

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
