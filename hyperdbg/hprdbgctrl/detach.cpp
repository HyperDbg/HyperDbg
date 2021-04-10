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
#include "pch.h"

//
// Global Variables
//
extern DEBUGGING_STATE g_DebuggingState;

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

    ShowMessages("This command is not supported on this version.\n");
    return;

    //
    // Check if we attached to a process or not
    //
    if (!g_DebuggingState.IsAttachedToUsermodeProcess)
    {
        ShowMessages("you're not attached to any thread.\n");
        return;
    }

    //
    // Check if debugger is loaded or not
    //
    if (!g_DeviceHandle)
    {
        ShowMessages("Handle not found, probably the driver is not loaded. Did you "
                     "use 'load' command?\n");
        return;
    }

    //
    // We wanna detach from a process
    //
    DetachRequest.IsAttach  = FALSE;
    DetachRequest.ProcessId = g_DebuggingState.ConnectedProcessId;
    DetachRequest.ThreadId  = g_DebuggingState.ConnectedThreadId;

    //
    // Send the request to the kernel
    //

    Status = DeviceIoControl(
        g_DeviceHandle,                                  // Handle to device
        IOCTL_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS,  // IO Control
                                                         // code
        &DetachRequest,                                  // Input Buffer to driver.
        SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Input buffer length
        &DetachRequest,                                  // Output Buffer from driver.
        SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Length of output
                                                         // buffer in bytes.
        &ReturnedLength,                                 // Bytes placed in buffer.
        NULL                                             // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return;
    }

    //
    // Check if attaching was successful then we can set the attached to true
    //
    if (DetachRequest.Result == DEBUGEER_OPERATION_WAS_SUCCESSFULL)
    {
        g_DebuggingState.IsAttachedToUsermodeProcess = FALSE;
        g_DebuggingState.ConnectedProcessId          = NULL;
        g_DebuggingState.ConnectedThreadId           = NULL;
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
