/**
 * @file detach.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .detach command
 * @details
 * @version 0.1
 * @date 2020-08-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\hprdbgctrl\pch.h"

//
// Global Variables
//
extern PTHREAD_DEBUGGING_STATE g_ActiveThreadDebuggingState;

/**
 * @brief help of .detach command
 *
 * @return VOID
 */
VOID
CommandDetachHelp()
{
    ShowMessages(".detach : detach from debugging a user-mode process.\n\n");
    ShowMessages("syntax : \t.detach\n");
}

/**
 * @brief perform detach from process
 *
 * @return VOID
 */
VOID
DetachFromProcess()
{
    BOOLEAN                                  Status;
    ULONG                                    ReturnedLength;
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS DetachRequest = {0};

    ShowMessages("this command is not yet ready!\nplease don't use it for now\n\n");
    return;

    //
    // Check if we attached to a process or not
    //
    if (!g_ActiveThreadDebuggingState)
    {
        ShowMessages("you're not attached to any thread\n");
        return;
    }

    //
    // Check if debugger is loaded or not
    //
    if (!g_DeviceHandle)
    {
        ShowMessages("handle of the driver not found, probably the driver is not loaded. Did you "
                     "use 'load' command?\n");
        return;
    }
}

/**
 * @brief .detach command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandDetach(vector<string> SplittedCommand, string Command)
{
    if (SplittedCommand.size() >= 2)
    {
        ShowMessages("incorrect use of '.detach'\n\n");
        CommandDetachHelp();
        return;
    }

    //
    // Perform detach from the process
    //
    DetachFromProcess();
}
