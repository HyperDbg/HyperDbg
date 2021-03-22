/**
 * @file g.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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
extern BOOLEAN g_BreakPrintingOutput;
extern BOOLEAN g_IsConnectedToRemoteDebuggee;
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of g command
 *
 * @return VOID
 */
VOID
CommandGHelp()
{
    ShowMessages("g : continue debuggee or processing kernel messages.\n\n");
    ShowMessages("syntax : \tg\n");
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
        //
        if (g_IsConnectedToRemoteDebuggee)
        {
            RemoteConnectionSendCommand("g", strlen("g") + 1);
        }
    }
}
