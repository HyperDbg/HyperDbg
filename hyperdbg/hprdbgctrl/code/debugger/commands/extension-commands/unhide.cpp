/**
 * @file unhide.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !unhide command
 * @details
 * @version 0.1
 * @date 2020-07-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of !unhide command
 *
 * @return VOID
 */
VOID
CommandUnhideHelp()
{
    ShowMessages("!unhide : reverts the transparency measures of the '!hide' command.\n\n");

    ShowMessages("syntax : \t!unhide\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !unhide\n");
}

/**
 * @brief !unhide command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandUnhide(vector<string> SplittedCommand, string Command)
{
    BOOLEAN                                     Status;
    ULONG                                       ReturnedLength;
    DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE UnhideRequest = {0};

    if (SplittedCommand.size() >= 2)
    {
        ShowMessages("incorrect use of '!unhide'\n\n");
        CommandUnhideHelp();
        return;
    }

    //
    // Check if debugger is loaded or not
    //
    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

    //
    // We don't wanna hide the debugger and make transparent vm-exits
    //
    UnhideRequest.IsHide = FALSE;

    //
    // Send the request to the kernel
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                                             // Handle to device
        IOCTL_DEBUGGER_HIDE_AND_UNHIDE_TO_TRANSPARENT_THE_DEBUGGER, // IO Control
                                                                    // code
        &UnhideRequest,                                             // Input Buffer to driver.
        SIZEOF_DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE,         // Input buffer length
        &UnhideRequest,                                             // Output Buffer from driver.
        SIZEOF_DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE,         // Length of output
                                                                    // buffer in bytes.
        &ReturnedLength,                                            // Bytes placed in buffer.
        NULL                                                        // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return;
    }

    if (UnhideRequest.KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
    {
        ShowMessages("transparent debugging successfully disabled :)\n");
    }
    else if (UnhideRequest.KernelStatus ==
             DEBUGGER_ERROR_DEBUGGER_ALREADY_UHIDE)
    {
        ShowMessages("debugger is not in transparent-mode\n");
    }
    else
    {
        ShowMessages("unknown error occurred :(\n");
    }
}
