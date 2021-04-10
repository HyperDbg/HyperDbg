/**
 * @file bc.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief bc command
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
 * @brief help of bc command
 *
 * @return VOID
 */
VOID
CommandBcHelp()
{
    ShowMessages("bc : clears a breakpoint using breakpoint id.\n\n");
    ShowMessages("syntax : \tbc [breakpoint id (hex value)]\n");
    ShowMessages("\t\te.g : bc 0\n");
    ShowMessages("\t\te.g : bc 2\n");
}

/**
 * @brief handler of bc command
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandBc(vector<string> SplittedCommand, string Command)
{
    UINT64                            BreakpointId;
    DEBUGGEE_BP_LIST_OR_MODIFY_PACKET Request = {0};

    //
    // Validate the commands
    //
    if (SplittedCommand.size() != 2)
    {
        ShowMessages("incorrect use of 'bc'\n\n");
        CommandBcHelp();
        return;
    }

    //
    // Get the breakpoint id
    //
    if (!ConvertStringToUInt64(SplittedCommand.at(1), &BreakpointId))
    {
        ShowMessages("please specify a correct hex value for breakpoint id\n\n");
        CommandBcHelp();

        return;
    }

    //
    // Check if the remote serial debuggee is paused or not (connected or not)
    //
    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Perform clearing breakpoint
        //
        Request.Request = DEBUGGEE_BREAKPOINT_MODIFICATION_REQUEST_CLEAR;

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
        ShowMessages("err, clearing breakpoints is only valid if you connected to "
                     "a debuggee in debugger-mode\n");
    }
}
