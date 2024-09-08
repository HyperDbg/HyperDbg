/**
 * @file load.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief load command
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
extern HANDLE  g_IsDriverLoadedSuccessfully;
extern HANDLE  g_DeviceHandle;
extern BOOLEAN g_IsConnectedToHyperDbgLocally;
extern BOOLEAN g_IsDebuggerModulesLoaded;

/**
 * @brief help of the load command
 *
 * @return VOID
 */
VOID
CommandLoadHelp()
{
    ShowMessages("load : installs the drivers and load the modules.\n\n");

    ShowMessages("syntax : \tload [ModuleName (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : load vmm\n");
}

/**
 * @brief load command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandLoad(vector<CommandToken> CommandTokens, string Command)
{
    if (CommandTokens.size() != 2)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandLoadHelp();
        return;
    }

    if (!g_IsConnectedToHyperDbgLocally)
    {
        ShowMessages("you're not connected to any instance of HyperDbg, did you "
                     "use '.connect'? \n");
        return;
    }

    //
    // Check for the module
    //
    if (CompareLowerCaseStrings(CommandTokens.at(1), "vmm"))
    {
        //
        // Check to make sure that the driver is not already loaded
        //
        if (g_DeviceHandle)
        {
            ShowMessages("handle of the driver found, if you use 'load' before, please "
                         "first unload it then call 'unload'\n");
            return;
        }

        //
        // Load VMM Module
        //
        ShowMessages("loading the vmm driver\n");

        if (HyperDbgInstallVmmDriver() == 1 || HyperDbgLoadVmmModule() == 1)
        {
            ShowMessages("failed to install or load the driver\n");
            return;
        }

        //
        // If in vmi-mode then initialize and load symbols (pdb)
        // for previously downloaded symbols
        // When the VMM module is loaded, we use the current
        // process (HyperDbg's process) as the base for user-mode
        // symbols
        //
        SymbolLocalReload(GetCurrentProcessId());
    }
    else
    {
        //
        // Module not found
        //
        ShowMessages("err, module not found\n");
    }
}
