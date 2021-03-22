/**
 * @file be.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief be command
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
 * @brief help of be command
 *
 * @return VOID
 */
VOID
CommandBeHelp()
{
    ShowMessages("be : enables a breakpoint using breakpoint id.\n\n");
    ShowMessages("syntax : \tbe [breakpoint id (hex value)]\n");
    ShowMessages("\t\te.g : be 0\n");
    ShowMessages("\t\te.g : be 2\n");
}

/**
 * @brief handler of be command
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandBe(vector<string> SplittedCommand, string Command)
{
    UINT64                            BreakpointId;
    DEBUGGEE_BP_LIST_OR_MODIFY_PACKET Request = {0};

    //
    // Validate the commands
    //
    if (SplittedCommand.size() != 2)
    {
        ShowMessages("incorrect use of 'be'\n\n");
        CommandBeHelp();
        return;
    }

    //
    // Get the breakpoint id
    //
    if (!ConvertStringToUInt64(SplittedCommand.at(1), &BreakpointId))
    {
        ShowMessages("please specify a correct hex value for breakpoint id\n\n");
        CommandBeHelp();

        return;
    }

    //
    // Check if the remote serial debuggee is paused or not (connected or not)
    //
    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Perform enabling breakpoint
        //
        Request.Request = DEBUGGEE_BREAKPOINT_MODIFICATION_REQUEST_ENABLE;

        //
        // Set breakpoint id
        //
        Request.BreakpointId = BreakpointId;

        //
        // Send the request
        //
        KdSendListOrModifyPacketToDebuggee(&Request);
    }
    else
    {
        ShowMessages("err, enabling breakpoints is only valid if you connected to "
                     "a debuggee in debugger-mode\n");
    }
}
