/**
 * @file kill.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .kill command
 * @details
 * @version 0.1
 * @date 2022-01-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

/**
 * @brief help of .kill command
 *
 * @return VOID
 */
VOID
CommandKillHelp()
{
    ShowMessages(".kill : terminates the current running process.\n\n");

    ShowMessages("syntax : \t.kill \n");
}

/**
 * @brief .kill command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandKill(vector<string> SplittedCommand, string Command)
{
    if (SplittedCommand.size() != 1)
    {
        ShowMessages("incorrect use of '.kill'\n\n");
        CommandKillHelp();
        return;
    }

    if (!g_ActiveProcessDebuggingState.IsActive)
    {
        ShowMessages("nothing to terminate!\n");
        return;
    }

    //
    // Kill the current active process
    //
    UdKillProcess(g_ActiveProcessDebuggingState.ProcessId);
}
