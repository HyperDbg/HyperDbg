/**
 * @file process.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief show and change process
 * @details
 * @version 0.1
 * @date 2021-02-02
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
 * @brief help of .process command
 *
 * @return VOID
 */
VOID
CommandProcessHelp()
{
    ShowMessages(".process : show and change the current process.\n\n");
    ShowMessages("syntax : \t.process [type (pid)] [new process id (hex)]\n");
    ShowMessages("\t\te.g : .process\n");
    ShowMessages("\t\te.g : .process pid 4\n");
}

/**
 * @brief .process command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandProcess(vector<string> SplittedCommand, string Command)
{
    UINT64 TargetProcess = 0;

    if (SplittedCommand.size() != 3 && SplittedCommand.size() != 1)
    {
        ShowMessages("incorrect use of '.process'\n\n");
        CommandProcessHelp();
        return;
    }

    //
    // Check if it's connected to a remote debuggee or not
    //
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, you're not connected to any debuggee.\n");
        return;
    }

    if (SplittedCommand.size() == 1)
    {
        //
        // Send the packet to get current process
        //
        KdSendSwitchProcessPacketToDebuggee(TRUE, NULL);
    }
    else if (SplittedCommand.size() == 3 &&
             !SplittedCommand.at(1).compare("pid"))
    {
        if (!ConvertStringToUInt64(SplittedCommand.at(2), &TargetProcess))
        {
            ShowMessages(
                "please specify a correct hex value for the process that you "
                "want to operate on it\n\n");
            CommandProcessHelp();
            return;
        }

        //
        // Send the packet to change process
        //
        KdSendSwitchProcessPacketToDebuggee(FALSE, TargetProcess);
    }
    else
    {
        ShowMessages("invalid parameter\n\n");
        CommandProcessHelp();
        return;
    }
}
