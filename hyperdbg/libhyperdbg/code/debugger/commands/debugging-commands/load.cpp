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
extern BOOLEAN g_IsConnectedToHyperDbgLocally;
extern BOOLEAN g_IsKdModuleLoaded;
extern BOOLEAN g_IsVmmModuleLoaded;
extern BOOLEAN g_IsHyperTraceModuleLoaded;

/**
 * @brief help of the load command
 *
 * @return VOID
 */
VOID
CommandLoadHelp()
{
    ShowMessages("load : installs drivers and load modules.\n\n");

    ShowMessages("syntax : \tload [ModuleNameOrAll (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : load vmm\n");
    ShowMessages("\t\te.g : load kd\n");
    ShowMessages("\t\te.g : load trace\n");
    ShowMessages("\t\te.g : load all\n");
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
    if (CompareLowerCaseStrings(CommandTokens.at(1), "all") || CompareLowerCaseStrings(CommandTokens.at(1), "."))
    {
        //
        // Load aa Modules
        //
        ShowMessages("loading the all modules\n");

        if (HyperDbgInstallKdDriver() == 1 || HyperDbgLoadAllModules() == 1)
        {
            ShowMessages("failed to install or load drivers\n");
            return;
        }

        //
        // If in vmi-mode then initialize and load symbols (pdb)
        // for previously downloaded symbols
        // When the VMM module is loaded, we use the current
        // process (HyperDbg's process) as the base for user-mode
        // symbols
        //
        SymbolLocalReload(PlatformGetCurrentProcessId());
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "vmm") ||
             CompareLowerCaseStrings(CommandTokens.at(1), "vm"))
    {
        //
        // Check to make sure that the driver is not already loaded
        //
        if (g_IsVmmModuleLoaded)
        {
            ShowMessages("the vmm module is already running, if you use 'load' before, please "
                         "first unload it using the 'unload' command\n");
            return;
        }

        //
        // Load the VMM Module
        //
        ShowMessages("loading the vmm module\n");

        if (HyperDbgInstallKdDriver() == 1 || HyperDbgLoadVmmModule() == 1)
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
        SymbolLocalReload(PlatformGetCurrentProcessId());
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "trace") ||
             CompareLowerCaseStrings(CommandTokens.at(1), "hypertrace"))
    {
        //
        // Check to make sure that the driver is not already loaded
        //
        if (g_IsHyperTraceModuleLoaded)
        {
            ShowMessages("the trace module is already running, if you use 'load' before, please "
                         "first unload it using the 'unload' command\n");
            return;
        }

        //
        // Load the HyperTrace Module
        //
        ShowMessages("loading the trace module\n");

        if (HyperDbgInstallKdDriver() == 1 || HyperDbgLoadHyperTraceModule() == 1)
        {
            ShowMessages("failed to install or load the driver\n");
            return;
        }
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "kd") ||
             CompareLowerCaseStrings(CommandTokens.at(1), "dbg") ||
             CompareLowerCaseStrings(CommandTokens.at(1), "debugger") ||
             CompareLowerCaseStrings(CommandTokens.at(1), "debug") ||
             CompareLowerCaseStrings(CommandTokens.at(1), "kerneldebugger") ||
             CompareLowerCaseStrings(CommandTokens.at(1), "kerneldebug"))
    {
        //
        // Check to make sure that the driver is not already loaded
        //
        if (g_IsKdModuleLoaded)
        {
            ShowMessages("the kd module is already running, if you use 'load' before, please "
                         "first unload it using the 'unload' command\n");
            return;
        }

        //
        // Load the KD Module
        //
        ShowMessages("loading the kd module\n");

        if (HyperDbgInstallKdDriver() == 1 || HyperDbgLoadKdModule() == 1)
        {
            ShowMessages("failed to install or load the driver\n");
            return;
        }
    }
    else
    {
        //
        // Module not found
        //
        ShowMessages("err, module not found\n");
    }
}
