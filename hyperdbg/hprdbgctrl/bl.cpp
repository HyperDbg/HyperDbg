/**
 * @file bl.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief bl command
 * @details
 * @version 0.1
 * @date 2021-03-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of bl command
 *
 * @return VOID
 */
VOID
CommandBlHelp()
{
    ShowMessages("bl : lists all the enabled and disabled breakpoints.\n\n");
    ShowMessages("syntax : \tbl\n");
}

/**
 * @brief handler of bl command
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandBl(vector<string> SplittedCommand, string Command)
{
    DEBUGGEE_BP_LIST_OR_MODIFY_PACKET Request = {0};

    //
    // Validate the commands
    //
    if (SplittedCommand.size() != 1)
    {
        ShowMessages("incorrect use of 'bl'\n\n");
        CommandBlHelp();
        return;
    }

    //
    // Check if the remote serial debuggee is paused or not (connected or not)
    //
    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Perform listing breakpoint
        //
        Request.Request = DEBUGGEE_BREAKPOINT_MODIFICATION_REQUEST_LIST_BREAKPOINTS;

        //
        // Send the request
        //
        KdSendListOrModifyPacketToDebuggee(&Request);
        ShowMessages("\n");
    }
    else
    {
        ShowMessages("err, listing breakpoints is only valid if you connected to "
                     "a debuggee in debugger-mode\n");
    }
}
