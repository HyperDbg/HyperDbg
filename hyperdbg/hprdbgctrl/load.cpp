/**
 * @file load.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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
 * @brief help of load command
 *
 * @return VOID
 */
VOID
CommandLoadHelp()
{
    ShowMessages("load : installs the drivers and load the modules.\n\n");
    ShowMessages("syntax : \tload [module name]\n");
    ShowMessages("\t\te.g : load vmm\n");
}

/**
 * @brief load vmm module
 *
 * @return BOOLEAN
 */
BOOLEAN
CommandLoadVmmModule()
{
    BOOL   Status;
    HANDLE hToken;

    //
    // Enable Debug privilege
    //
    Status =
        OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
    if (!Status)
    {
        ShowMessages("OpenProcessToken failed, error : %u \n");
        return FALSE;
    }

    Status = SetPrivilege(hToken, SE_DEBUG_NAME, TRUE);
    if (!Status)
    {
        CloseHandle(hToken);
        return FALSE;
    }

    if (HyperdbgInstallVmmDriver() == 1)
    {
        return FALSE;
    }

    //
    // Create event to show if the hypervisor is loaded or not
    //
    g_IsDriverLoadedSuccessfully = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (HyperdbgLoadVmm() == 1)
    {
        //
        // No need to handle anymore
        //
        CloseHandle(g_IsDriverLoadedSuccessfully);
        return FALSE;
    }

    //
    // We wait for the first message from the kernel debugger to continue
    //
    WaitForSingleObject(
        g_IsDriverLoadedSuccessfully,
        INFINITE);

    //
    // No need to handle anymore
    //
    CloseHandle(g_IsDriverLoadedSuccessfully);

    //
    // If we reach here so the module are loaded
    //
    g_IsDebuggerModulesLoaded = TRUE;

    ShowMessages("HyperDbg hypervisor is loaded successfully !\n");

    return TRUE;
}

/**
 * @brief load command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandLoad(vector<string> SplittedCommand, string Command)
{
    if (SplittedCommand.size() != 2)
    {
        ShowMessages("incorrect use of 'load'\n\n");
        CommandLoadHelp();
        return;
    }

    if (!g_IsConnectedToHyperDbgLocally)
    {
        ShowMessages("You're not connected to any instance of HyperDbg, did you "
                     "use '.connect'? \n");
        return;
    }

    //
    // Check for the module
    //
    if (!SplittedCommand.at(1).compare("vmm"))
    {
        //
        // Check to make sure that the driver is not already loaded
        //
        if (g_DeviceHandle)
        {
            ShowMessages("Handle of driver found, if you use 'load' before, please "
                         "first unload it then call 'unload'.\n");
            return;
        }

        //
        // Load VMM Module
        //
        ShowMessages("Try to install and load the VMM driver...\n");

        if (!CommandLoadVmmModule())
        {
            ShowMessages("Failed to install or load the driver\n");
            return;
        }
    }
    else
    {
        //
        // Module not found
        //
        ShowMessages("module not found, currently 'vmm' is the only available "
                     "module for HyperDbg.\n");
    }
}
