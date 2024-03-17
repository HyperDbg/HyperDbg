/**
 * @file preactivate.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief preactivate command
 * @details
 * @version 0.7
 * @date 2023-10-25
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of the preactivate command
 *
 * @return VOID
 */
VOID
CommandPreactivateHelp()
{
    ShowMessages("preactivate : preactivates a special functionality.\n\n");

    ShowMessages("syntax : \tpreactivate  [Type (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : preactivate mode\n");

    ShowMessages("\n");
    ShowMessages("type of activations:\n");
    ShowMessages("\tmode: used for preactivation of the '!mode' event\n");
}

/**
 * @brief preactivate command handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandPreactivate(vector<string> SplitCommand, string Command)
{
    BOOL                         Status;
    ULONG                        ReturnedLength;
    DEBUGGER_PREACTIVATE_COMMAND PreactivateRequest = {0};

    if (SplitCommand.size() != 2)
    {
        ShowMessages("incorrect use of the 'Preactivate'\n\n");
        CommandPreactivateHelp();
        return;
    }

    //
    // Set the type of preactivation
    //
    if (!SplitCommand.at(1).compare("mode") || !SplitCommand.at(1).compare("!mode"))
    {
        PreactivateRequest.Type = DEBUGGER_PREACTIVATE_COMMAND_TYPE_MODE;
    }
    else
    {
        //
        // Couldn't resolve or unknown parameter
        //
        ShowMessages("err, couldn't resolve error at '%s'\n",
                     SplitCommand.at(1).c_str());
        return;
    }

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

    //
    // Send IOCTL
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                      // Handle to device
        IOCTL_PREACTIVATE_FUNCTIONALITY,     // IO Control Code (IOCTL)
        &PreactivateRequest,                 // Input Buffer to driver.
        SIZEOF_DEBUGGER_PREACTIVATE_COMMAND, // Input buffer length
        &PreactivateRequest,                 // Output Buffer from driver.
        SIZEOF_DEBUGGER_PREACTIVATE_COMMAND, // Length of output
                                             // buffer in bytes.
        &ReturnedLength,                     // Bytes placed in buffer.
        NULL                                 // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return;
    }

    if (PreactivateRequest.KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
    {
        ShowMessages("the requested service is activated successfully!\n");
    }
    else
    {
        //
        // An err occurred, no results
        //
        ShowErrorMessage(PreactivateRequest.KernelStatus);
    }
}
