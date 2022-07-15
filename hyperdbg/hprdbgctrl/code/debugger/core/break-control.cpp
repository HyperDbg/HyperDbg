/**
 * @file breakcontrol.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief break control is the handler for CTRL+C and CTRL+BREAK Signals
 * @details
 * @version 0.1
 * @date 2020-07-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN                  g_BreakPrintingOutput;
extern BOOLEAN                  g_IsDebuggerModulesLoaded;
extern BOOLEAN                  g_AutoUnpause;
extern BOOLEAN                  g_IsConnectedToRemoteDebuggee;
extern BOOLEAN                  g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN                  g_IsExecutingSymbolLoadingRoutines;
extern BOOLEAN                  g_IsInstrumentingInstructions;
extern BOOLEAN                  g_IgnorePauseRequests;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

/**
 * @brief handle CTRL+C and CTRL+Break events
 *
 * @param CtrlType
 * @return BOOL
 */
BOOL
BreakController(DWORD CtrlType)
{
    switch (CtrlType)
    {
        //
        // Handle the CTRL + C & CTRL + Break signal
        //
    case CTRL_BREAK_EVENT:
    case CTRL_C_EVENT:

        //
        // check if we should ignore the break requests or not
        //
        if (g_IgnorePauseRequests)
        {
            return TRUE;
        }

        //
        // Check for aborting symbol loading routines
        //
        if (g_IsExecutingSymbolLoadingRoutines)
        {
            //
            // Avoid to calling it again
            //
            g_IsExecutingSymbolLoadingRoutines = FALSE;

            //
            // Abort the execution
            //
            ScriptEngineSymbolAbortLoadingWrapper();
        }

        //
        // Check if the debuggee is running because of pausing or not
        //
        if (g_IsSerialConnectedToRemoteDebuggee)
        {
            if (g_IsInstrumentingInstructions)
            {
                g_IsInstrumentingInstructions = FALSE;
            }
            else
            {
                KdBreakControlCheckAndPauseDebugger();
            }
        }
        else if (!g_IsDebuggerModulesLoaded && !g_IsConnectedToRemoteDebuggee)
        {
            //
            // vmm module is not loaded here
            //
        }
        else
        {
            if (g_IsInstrumentingInstructions)
            {
                g_IsInstrumentingInstructions = FALSE;
            }
            else
            {
                //
                // Sleep because the other thread that shows must be stopped
                //
                g_BreakPrintingOutput = TRUE;

                //
                // Check if its a remote debuggee then we should send the 'pause' command
                //
                if (g_IsConnectedToRemoteDebuggee)
                {
                    RemoteConnectionSendCommand("pause", strlen("pause") + 1);
                }

                Sleep(300);

                //
                // It is because we didn't query the target debuggee auto-unpause variable
                //
                if (!g_IsConnectedToRemoteDebuggee)
                {
                    if (g_AutoUnpause)
                    {
                        ShowMessages(
                            "\npausing...\nauto-unpause mode is enabled, "
                            "debugger will automatically continue when you run a new "
                            "event command, if you want to change this behaviour then "
                            "run run 'settings autounpause off'\n\n");
                    }
                    else
                    {
                        ShowMessages(
                            "\npausing...\nauto-unpause mode is disabled, you "
                            "should run 'g' when you want to continue, otherwise run "
                            "'settings "
                            "autounpause on'\n\n");
                    }

                    //
                    // Show the signature of HyperDbg
                    //
                    HyperDbgShowSignature();

                    if (g_ActiveProcessDebuggingState.IsActive)
                    {
                        UdPauseProcess(g_ActiveProcessDebuggingState.ProcessDebuggingToken);
                    }
                }
            }
        }

        return TRUE;

        //
        // CTRL+CLOSE: confirm that the user wants to exit.
        //
    case CTRL_CLOSE_EVENT:
        return TRUE;

    case CTRL_LOGOFF_EVENT:
        return FALSE;

    case CTRL_SHUTDOWN_EVENT:
        return FALSE;

    default:

        //
        // Return TRUE if handled this message, further handler functions won't be
        // called.
        // Return FALSE to pass this message to further handlers until default
        // handler calls ExitProcess().
        //
        return FALSE;
    }
}
