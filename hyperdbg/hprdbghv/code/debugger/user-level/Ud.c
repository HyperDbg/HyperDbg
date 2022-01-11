/**
 * @file Ud.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Routines related to user mode debugging
 * @details 
 * @version 0.1
 * @date 2022-01-06
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "..\hprdbghv\pch.h"

/**
 * @brief initialize user debugger
 * @details this function should be called on vmx non-root
 * 
 * @return VOID 
 */
VOID
UdInitializeUserDebugger()
{
    //
    // Check if it's already initialized or not, we'll ignore it if it's
    // previously initialized
    //
    if (g_UserDebuggerState)
    {
        return;
    }

    //
    // Initialize attaching mechanism
    //
    if (!AttachingInitialize())
    {
        return FALSE;
    }

    //
    // Start the seed of user-mode debugging thread
    //
    g_SeedOfUserDebuggingDetails = DebuggerThreadDebuggingTagStartSeed;

    //
    // Initialize the thread debugging details list
    //
    InitializeListHead(&g_ThreadDebuggingDetailsListHead);

    //
    // Enable vm-exit on Hardware debug exceptions and breakpoints
    // so, intercept #DBs and #BP by changing exception bitmap (one core)
    //
    BroadcastEnableDbAndBpExitingAllCores();

    //
    // Indicate that the user debugger is active
    //
    g_UserDebuggerState = TRUE;
}

/**
 * @brief uninitialize user debugger
 * @details this function should be called on vmx non-root
 *
 * @return VOID 
 */
VOID
UdUninitializeUserDebugger()
{
    if (g_UserDebuggerState)
    {
        //
        // Indicate that the user debugger is not active
        //
        g_UserDebuggerState = FALSE;
    }
}
