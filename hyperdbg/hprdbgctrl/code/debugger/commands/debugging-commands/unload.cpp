/**
 * @file unload.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief unload command
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
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN g_IsSerialConnectedToRemoteDebugger;

/**
 * @brief help of unload command
 *
 * @return VOID
 */
VOID
CommandUnloadHelp()
{
    ShowMessages(
        "unload : unloads the kernel modules and uninstalls the drivers.\n\n");

    ShowMessages("syntax : \tunload [remove] [ModuleName (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : unload vmm\n");
    ShowMessages("\t\te.g : unload remove vmm\n");
}

/**
 * @brief unload command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandUnload(vector<string> SplittedCommand, string Command)
{
    if (SplittedCommand.size() != 2 && SplittedCommand.size() != 3)
    {
        ShowMessages("incorrect use of 'unload'\n\n");
        CommandUnloadHelp();
        return;
    }

    //
    // Check for the module
    //
    if ((SplittedCommand.size() == 2 && !SplittedCommand.at(1).compare("vmm")) ||
        (SplittedCommand.size() == 3 && !SplittedCommand.at(2).compare("vmm") &&
         !SplittedCommand.at(1).compare("remove")))
    {
        if (!g_IsConnectedToHyperDbgLocally)
        {
            ShowMessages("you're not connected to any instance of HyperDbg, did you "
                         "use '.connect'? \n");
            return;
        }

        //
        // Check to avoid using this command in debugger-mode
        //
        if (g_IsSerialConnectedToRemoteDebuggee || g_IsSerialConnectedToRemoteDebugger)
        {
            ShowMessages("you're connected to a an instance of HyperDbg, please use "
                         "'.debug close' command\n");
            return;
        }

        if (g_IsDebuggerModulesLoaded)
        {
            HyperDbgUnloadVmm();
        }
        else
        {
            ShowMessages("there is nothing to unload\n");
        }

        //
        // Check to remove the driver
        //
        if (!SplittedCommand.at(1).compare("remove"))
        {
            //
            // Stop the driver
            //
            if (HyperDbgStopVmmDriver())
            {
                ShowMessages("err, failed to stop driver\n");
                return;
            }

            //
            // Uninstall the driver
            //
            if (HyperDbgUninstallVmmDriver())
            {
                ShowMessages("err, failed to uninstall the driver\n");
                return;
            }

            ShowMessages("the driver is removed\n");
        }
    }
    else
    {
        //
        // Module not found
        //
        ShowMessages("module not found, currently 'vmm' is the only available "
                     "module for HyperDbg\n");
    }
}
