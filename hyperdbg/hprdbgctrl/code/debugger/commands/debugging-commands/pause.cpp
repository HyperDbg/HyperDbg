/**
 * @file pause.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief pause command
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
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

/**
 * @brief help of pause command
 *
 * @return VOID
 */
VOID
CommandPauseHelp()
{
    ShowMessages("pause : pauses the kernel events.\n\n");

    ShowMessages("syntax : \tpause \n");
}

/**
 * @brief request to pause 
 *
 * @return VOID
 */
VOID
CommandPauseRequest()
{
    //
    // Set the g_BreakPrintingOutput to TRUE
    //
    g_BreakPrintingOutput = TRUE;
    ShowMessages("pausing...\n");

    //
    // If it's a remote debugger then we send the remote debuggee a 'g'
    //
    if (g_IsConnectedToRemoteDebuggee)
    {
        RemoteConnectionSendCommand("pause", strlen("pause") + 1);
    }
    else if (g_ActiveProcessDebuggingState.IsActive && UdPauseProcess(g_ActiveProcessDebuggingState.ProcessDebuggingToken))
    {
        ShowMessages("please keep interacting with the process until all the "
                     "threads are intercepted and halted; whenever you execute "
                     "the first command, the thread interception will be stopped\n");
    }
}

/**
 * @brief pause command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandPause(vector<string> SplittedCommand, string Command)
{
    if (SplittedCommand.size() != 1)
    {
        ShowMessages("incorrect use of 'pause'\n\n");
        CommandPauseHelp();
        return;
    }

    CommandPauseRequest();
}
