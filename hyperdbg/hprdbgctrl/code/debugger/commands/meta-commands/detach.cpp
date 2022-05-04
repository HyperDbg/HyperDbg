/**
 * @file detach.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .detach command
 * @details
 * @version 0.1
 * @date 2020-08-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;
extern BOOLEAN                  g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of .detach command
 *
 * @return VOID
 */
VOID
CommandDetachHelp()
{
    ShowMessages(".detach : detaches from debugging a user-mode process.\n\n");

    ShowMessages("syntax : \t.detach \n");
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

    //
    // Check if debugger is loaded or not
    //
    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

    //
    // Check if we attached to a process or not
    //
    if (!g_ActiveProcessDebuggingState.IsActive)
    {
        ShowMessages("you're not attached to any thread\n");
        return;
    }

    //
    // Perform the detaching of the target process
    //
    UdDetachProcess(g_ActiveProcessDebuggingState.ProcessId, g_ActiveProcessDebuggingState.ProcessDebuggingToken);
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
    // .attach and .detach commands are only supported in VMI Mode
    //
    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, '.attach', and '.detach' commands are only usable "
                     "in VMI Mode, you can use the '.process', or the '.thread' "
                     "in Debugger Mode\n");
        return;
    }

    //
    // Perform detach from the process
    //
    DetachFromProcess();
}
