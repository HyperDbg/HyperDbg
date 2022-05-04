/**
 * @file g.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief g command
 * @details
 * @version 0.1
 * @date 2020-07-25
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN                  g_BreakPrintingOutput;
extern BOOLEAN                  g_IsConnectedToRemoteDebuggee;
extern BOOLEAN                  g_IsSerialConnectedToRemoteDebuggee;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

/**
 * @brief help of g command
 *
 * @return VOID
 */
VOID
CommandGHelp()
{
    ShowMessages("g : continues debuggee or continues processing kernel messages.\n\n");

    ShowMessages("syntax : \tg \n");
}

/**
 * @brief Request to unpause
 *
 * @return VOID
 */
VOID
CommandGRequest()
{
    //
    // Check if the remote serial debuggee is paused or not
    //
    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        KdBreakControlCheckAndContinueDebugger();
    }
    else
    {
        //
        // Set the g_BreakPrintingOutput to FALSE
        //
        g_BreakPrintingOutput = FALSE;

        //
        // If it's a remote debugger then we send the remote debuggee a 'g'
        // and if we're connect to user debugger then we send the packet
        // with current debugging thread token
        //
        if (g_IsConnectedToRemoteDebuggee)
        {
            RemoteConnectionSendCommand("g", strlen("g") + 1);
        }
        else if (g_ActiveProcessDebuggingState.IsActive)
        {
            if (g_ActiveProcessDebuggingState.IsPaused)
            {
                UdContinueDebuggee(g_ActiveProcessDebuggingState.ProcessDebuggingToken);

                //
                // Target process is running
                //
                g_ActiveProcessDebuggingState.IsPaused = FALSE;
            }
            else
            {
                ShowMessages("err, target process is already running\n");
            }
        }
    }
}

/**
 * @brief handler of g command
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandG(vector<string> SplittedCommand, string Command)
{
    if (SplittedCommand.size() != 1)
    {
        ShowMessages("incorrect use of 'g'\n\n");
        CommandGHelp();
        return;
    }

    CommandGRequest();
}
