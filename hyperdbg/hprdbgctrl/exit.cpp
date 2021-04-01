/**
 * @file exit.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief exit command
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsConnectedToHyperDbgLocally;
extern BOOLEAN g_IsDebuggerModulesLoaded;

/**
 * @brief help of exit command
 *
 * @return VOID
 */
VOID
CommandExitHelp()
{
    ShowMessages(
        "exit : unload and uninstalls the drivers and closes the debugger.\n\n");
    ShowMessages("syntax : \texit\n");
}

/**
 * @brief exit command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandExit(vector<string> SplittedCommand, string Command)
{
    if (SplittedCommand.size() != 1)
    {
        ShowMessages("incorrect use of 'exit'\n\n");
        CommandExitHelp();
        return;
    }

    //
    // unload and exit
    //
    if (g_IsDebuggerModulesLoaded)
    {
        HyperdbgUnload();
    }

    exit(0);
}
