/**
 * @file restart.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .restart command
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
extern std::wstring             g_StartCommandPath;
extern std::wstring             g_StartCommandPathAndArguments;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;
;

/**
 * @brief help of the .restart command
 *
 * @return VOID
 */
VOID
CommandRestartHelp()
{
    ShowMessages(".restart : restarts the previously started process "
                 "(using '.start' command).\n\n");

    ShowMessages(
        "syntax : \t.restart \n");
}

/**
 * @brief .restart command handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandRestart(vector<string> SplitCommand, string Command)
{
    if (SplitCommand.size() != 1)
    {
        ShowMessages("incorrect use of the '.restart'\n\n");
        CommandRestartHelp();
        return;
    }

    //
    // Check if the .start command is previously called or not
    //
    if (g_StartCommandPath.empty())
    {
        ShowMessages("nothing to restart, did you use the '.start' command before?\n");
        return;
    }

    //
    // Check to kill the current active process (if exists)
    //
    if (g_ActiveProcessDebuggingState.IsActive)
    {
        //
        // kill the process, we will restart the process even if we didn't
        // successfully killed the active process
        //
        UdKillProcess(g_ActiveProcessDebuggingState.ProcessId);
    }
    else if (g_ProcessIdOfLatestStartingProcess != NULL)
    {
        UdKillProcess(g_ProcessIdOfLatestStartingProcess);

        //
        // No longer the last process exists
        //
        g_ProcessIdOfLatestStartingProcess = NULL;
    }

    //
    // Perform run of the target file
    //
    if (g_StartCommandPathAndArguments.empty())
    {
        UdAttachToProcess(NULL,
                          g_StartCommandPath.c_str(),
                          NULL,
                          FALSE);
    }
    else
    {
        UdAttachToProcess(NULL,
                          g_StartCommandPath.c_str(),
                          (WCHAR *)g_StartCommandPathAndArguments.c_str(),
                          FALSE);
    }
}
