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
extern BOOLEAN g_IsKdModuleLoaded;
extern BOOLEAN g_IsVmmModuleLoaded;
extern BOOLEAN g_IsHyperTraceModuleLoaded;
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN g_IsSerialConnectedToRemoteDebugger;

/**
 * @brief help of the unload command
 *
 * @return VOID
 */
VOID
CommandUnloadHelp()
{
    ShowMessages("unload : unloads the kernel modules and uninstalls drivers.\n\n");

    ShowMessages("syntax : \tunload [ModuleNameOrAll (string)]\n");
    ShowMessages("syntax : \tunload [remove] [all]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : unload vmm\n");
    ShowMessages("\t\te.g : unload trace\n");
    ShowMessages("\t\te.g : unload kd\n");
    ShowMessages("\t\te.g : unload all\n");
    ShowMessages("\t\te.g : unload remove all\n");
}

/**
 * @brief check the environment for unload command
 *
 * @return BOOLEAN
 */
BOOLEAN
CommandUnloadCheckEnvironment()
{
    if (!g_IsConnectedToHyperDbgLocally)
    {
        ShowMessages("you're not connected to any instance of HyperDbg, did you "
                     "use '.connect'? \n");
        return FALSE;
    }

    //
    // Check to avoid using this command in debugger-mode
    //
    if (g_IsSerialConnectedToRemoteDebuggee || g_IsSerialConnectedToRemoteDebugger)
    {
        ShowMessages("you're connected to a an instance of HyperDbg, please use "
                     "'.debug close' command\n");
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief unload command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandUnload(vector<CommandToken> CommandTokens, string Command)
{
    if (CommandTokens.size() != 2 && CommandTokens.size() != 3)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandUnloadHelp();
        return;
    }

    //
    // Check for the module
    //
    if (CommandTokens.size() == 2 &&
        (CompareLowerCaseStrings(CommandTokens.at(1), "vmm") || CompareLowerCaseStrings(CommandTokens.at(1), "vm")))
    {
        //
        // Check the environment
        //
        if (!CommandUnloadCheckEnvironment())
        {
            return;
        }

        if (g_IsVmmModuleLoaded)
        {
            HyperDbgUnloadVmm();

            HyperDbgUnloadKd(); // Test: Should be removed
        }
        else
        {
            ShowMessages("the vmm module is not loadedd\n");
        }
    }
    else if (CommandTokens.size() == 2 &&
             (CompareLowerCaseStrings(CommandTokens.at(1), "trace") || CompareLowerCaseStrings(CommandTokens.at(1), "hypertrace")))
    {
        //
        // Check the environment
        //
        if (!CommandUnloadCheckEnvironment())
        {
            return;
        }

        if (g_IsHyperTraceModuleLoaded)
        {
            HyperDbgUnloadHyperTrace();
        }
        else
        {
            ShowMessages("the trace (hypertrace) module is not loadedd\n");
        }
    }
    else if (CommandTokens.size() == 2 &&
             (CompareLowerCaseStrings(CommandTokens.at(1), "kd") || CompareLowerCaseStrings(CommandTokens.at(1), "dbg") ||
              CompareLowerCaseStrings(CommandTokens.at(1), "debugger") || CompareLowerCaseStrings(CommandTokens.at(1), "debug") ||
              CompareLowerCaseStrings(CommandTokens.at(1), "kerneldebugger") || CompareLowerCaseStrings(CommandTokens.at(1), "kerneldebug")))
    {
        //
        // Check the environment
        //
        if (!CommandUnloadCheckEnvironment())
        {
            return;
        }

        if (g_IsKdModuleLoaded)
        {
            HyperDbgUnloadKd();
        }
        else
        {
            ShowMessages("the kd (kernel debugger) module is not loadedd\n");
        }
    }
    else if (CommandTokens.size() == 2 &&
             (CompareLowerCaseStrings(CommandTokens.at(1), "all") || CompareLowerCaseStrings(CommandTokens.at(1), ".")))
    {
        //
        // Check the environment
        //
        if (!CommandUnloadCheckEnvironment())
        {
            return;
        }

        ShowMessages("unloading all modules\n");

        //
        // Unload all modules
        //
        if (HyperDbgUnloadAllModules() != 0)
        {
            ShowMessages("err, failed to unload all modules\n");
            return;
        }
        else
        {
            ShowMessages("all modules are unloaded\n");
        }
    }
    else if (CommandTokens.size() == 3 && CompareLowerCaseStrings(CommandTokens.at(1), "remove") &&
             (CompareLowerCaseStrings(CommandTokens.at(2), "all") ||
              CompareLowerCaseStrings(CommandTokens.at(2), ".") ||
              CompareLowerCaseStrings(CommandTokens.at(2), "vmm") ||
              CompareLowerCaseStrings(CommandTokens.at(2), "vm")))
    {
        //
        // Check the environment
        //
        if (!CommandUnloadCheckEnvironment())
        {
            return;
        }

        //
        // Unload all modules before removing the driver
        //
        if (HyperDbgUnloadAllModules())
        {
            ShowMessages("err, failed to unload all modules\n");
            return;
        }

        //
        // Stop the driver
        //
        if (HyperDbgStopKdDriver())
        {
            ShowMessages("err, failed to stop driver\n");
            return;
        }

        //
        // Uninstall the driver
        //
        if (HyperDbgUninstallKdDriver())
        {
            ShowMessages("err, failed to uninstall the driver\n");
            return;
        }

        ShowMessages("all drivers are removed\n");
    }
    else
    {
        //
        // Module not found
        //
        ShowMessages("err, module not found\n");
    }
}
