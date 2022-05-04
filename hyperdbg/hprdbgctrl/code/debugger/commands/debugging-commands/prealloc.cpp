/**
 * @file prealloc.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief prealloc command
 * @details
 * @version 0.1
 * @date 2020-11-13
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of prealloc command
 *
 * @return VOID
 */
VOID
CommandPreallocHelp()
{
    ShowMessages("prealloc : pre-allocates buffer for special purposes.\n\n");

    ShowMessages("syntax : \tprealloc  [Type (string)] [Count (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : prealloc monitor 10\n");
    ShowMessages("\t\te.g : prealloc thread-interception 8\n");
}

/**
 * @brief prealloc command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandPrealloc(vector<string> SplittedCommand, string Command)
{
    BOOL                      Status;
    ULONG                     ReturnedLength;
    UINT64                    Count;
    DEBUGGER_PREALLOC_COMMAND PreallocRequest = {0};

    if (SplittedCommand.size() != 3)
    {
        ShowMessages("incorrect use of 'prealloc'\n\n");
        CommandPreallocHelp();
        return;
    }

    //
    // Set the type of pre-allocation
    //
    if (!SplittedCommand.at(1).compare("monitor"))
    {
        PreallocRequest.Type = DEBUGGER_PREALLOC_COMMAND_TYPE_MONITOR;
    }
    else if (!SplittedCommand.at(1).compare("thread-interception"))
    {
        PreallocRequest.Type = DEBUGGER_PREALLOC_COMMAND_TYPE_THREAD_INTERCEPTION;
    }
    else
    {
        //
        // Couldn't resolve or unkonwn parameter
        //
        ShowMessages("err, couldn't resolve error at '%s'\n",
                     SplittedCommand.at(1).c_str());
        return;
    }

    //
    // Get the count of needed pre-allocated buffers
    //
    if (!SymbolConvertNameOrExprToAddress(SplittedCommand.at(2), &Count))
    {
        //
        // Couldn't resolve or unkonwn parameter
        //
        ShowMessages("err, couldn't resolve error at '%s'\n",
                     SplittedCommand.at(2).c_str());
        return;
    }

    //
    // Set the counter
    //
    PreallocRequest.Count = Count;

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

    //
    // Send IOCTL
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                    // Handle to device
        IOCTL_RESERVE_PRE_ALLOCATED_POOLS, // IO Control code
        &PreallocRequest,                  // Input Buffer to driver.
        SIZEOF_DEBUGGER_PREALLOC_COMMAND,  // Input buffer length
        &PreallocRequest,                  // Output Buffer from driver.
        SIZEOF_DEBUGGER_PREALLOC_COMMAND,  // Length of output
                                           // buffer in bytes.
        &ReturnedLength,                   // Bytes placed in buffer.
        NULL                               // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return;
    }

    if (PreallocRequest.KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
    {
        ShowMessages("the requested pools are allocated and reserved\n");
    }
    else
    {
        //
        // An err occurred, no results
        //
        ShowErrorMessage(PreallocRequest.KernelStatus);
    }
}
