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
extern UINT32                   g_ProcessIdOfLatestStartingProcess;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

/**
 * @brief help of the .kill command
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
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandKill(vector<string> SplitCommand, string Command)
{
    if (SplitCommand.size() != 1)
    {
        ShowMessages("incorrect use of the '.kill'\n\n");
        CommandKillHelp();
        return;
    }

    if (g_ActiveProcessDebuggingState.IsActive)
    {
        //
        // Kill the current active process
        //
        if (!UdKillProcess(g_ActiveProcessDebuggingState.ProcessId))
        {
            ShowMessages("process does not exists, is it already terminated?\n");
        }
    }
    else if (g_ProcessIdOfLatestStartingProcess != NULL)
    {
        if (!UdKillProcess(g_ProcessIdOfLatestStartingProcess))
        {
            ShowMessages("process does not exists, is it already terminated?\n");
        }

        //
        // No longer the last process exists
        //
        g_ProcessIdOfLatestStartingProcess = NULL;
    }
    else
    {
        ShowMessages("nothing to terminate!\n");
        return;
    }
}
