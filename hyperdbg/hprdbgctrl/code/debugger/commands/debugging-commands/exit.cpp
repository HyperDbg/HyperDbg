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
extern HANDLE  g_DeviceHandle;
extern BOOLEAN g_IsConnectedToHyperDbgLocally;
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of the exit command
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
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandExit(vector<string> SplitCommand, string Command)
{
    if (SplitCommand.size() != 1)
    {
        ShowMessages("incorrect use of the 'exit'\n\n");
        CommandExitHelp();
        return;
    }

    if (g_IsConnectedToHyperDbgLocally)
    {
        //
        // It is in VMI mode
        //

        //
        // unload and exit if the vmm module is loaded
        //
        if (g_DeviceHandle)
        {
            HyperDbgUnloadVmm();
        }
    }
    else if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // It is in debugger mode
        //

        KdCloseConnection();
    }

    exit(0);
}
