/**
 * @file unload.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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
    ShowMessages("syntax : \tunload [Module Name]\n");
    ShowMessages("\t\te.g : unload vmm\n");
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
    if (SplittedCommand.size() != 2)
    {
        ShowMessages("incorrect use of 'unload'\n\n");
        CommandUnloadHelp();
        return;
    }

    //
    // Check for the module
    //
    if (!SplittedCommand.at(1).compare("vmm"))
    {
        if (!g_IsConnectedToHyperDbgLocally)
        {
            ShowMessages("You're not connected to any instance of HyperDbg, did you "
                         "use '.connect'? \n");
            return;
        }

        if (g_IsDebuggerModulesLoaded)
        {
            HyperdbgUnload();

            //
            // Uninstalling Driver
            //
            if (HyperdbgUninstallDriver())
            {
                ShowMessages("Failed to uninstall driver\n");
            }
        }
        else
        {
            ShowMessages("there is nothing to unload\n");
        }
    }
    else
    {
        //
        // Module not found
        //
        ShowMessages("module not found, currently, 'vmm' is the only available "
                     "module for HyperDbg.\n");
    }
}
