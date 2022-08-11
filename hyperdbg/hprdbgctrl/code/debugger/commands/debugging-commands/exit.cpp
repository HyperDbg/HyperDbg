/**
 * @file exit.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
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
extern HANDLE g_DeviceHandle;

/**
 * @brief help of exit command
 *
 * @return VOID
 */
VOID
CommandExitHelp()
{
    ShowMessages(
        "exit : unloads and uninstalls the drivers and closes the debugger.\n\n");

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
    // unload and exit if the vmm module is loaded
    //
    if (g_DeviceHandle)
    {
        HyperDbgUnloadVmm();
    }

    exit(0);
}
