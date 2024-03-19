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
 * @brief help of the prealloc command
 *
 * @return VOID
 */
VOID
CommandPreallocHelp()
{
    ShowMessages("prealloc : pre-allocates buffer for special purposes.\n\n");

    ShowMessages("syntax : \tprealloc  [Type (string)] [Count (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : prealloc thread-interception 8\n");
    ShowMessages("\t\te.g : prealloc monitor 10\n");
    ShowMessages("\t\te.g : prealloc epthook 5\n");
    ShowMessages("\t\te.g : prealloc epthook2 3\n");
    ShowMessages("\t\te.g : prealloc regular-event 12\n");
    ShowMessages("\t\te.g : prealloc big-safe-buffert 1\n");

    ShowMessages("\n");
    ShowMessages("type of allocations:\n");
    ShowMessages("\tthread-interception: used for pre-allocations of the thread holders for the thread interception mechanism\n");
    ShowMessages("\tmonitor: used for pre-allocations of the '!monitor' EPT hooks\n");
    ShowMessages("\tepthook: used for pre-allocations of the '!epthook' EPT hooks\n");
    ShowMessages("\tepthook2: used for pre-allocations of the '!epthook2' EPT hooks\n");
    ShowMessages("\tregular-event: used for pre-allocations of regular instant events\n");
    ShowMessages("\tbig-event: used for pre-allocations of big instant events\n");
    ShowMessages("\tregular-safe-buffer: used for pre-allocations of the regular event safe buffers ($buffer) for instant events\n");
    ShowMessages("\tbig-safe-buffer: used for pre-allocations of the big event safe buffers ($buffer) for instant events\n");
}

/**
 * @brief prealloc command handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandPrealloc(vector<string> SplitCommand, string Command)
{
    BOOL                      Status;
    ULONG                     ReturnedLength;
    UINT64                    Count;
    DEBUGGER_PREALLOC_COMMAND PreallocRequest = {0};

    if (SplitCommand.size() != 3)
    {
        ShowMessages("incorrect use of the 'prealloc'\n\n");
        CommandPreallocHelp();
        return;
    }

    //
    // Set the type of pre-allocation
    //
    if (!SplitCommand.at(1).compare("thread-interception"))
    {
        PreallocRequest.Type = DEBUGGER_PREALLOC_COMMAND_TYPE_THREAD_INTERCEPTION;
    }
    else if (!SplitCommand.at(1).compare("monitor") || !SplitCommand.at(1).compare("!monitor"))
    {
        PreallocRequest.Type = DEBUGGER_PREALLOC_COMMAND_TYPE_MONITOR;
    }
    else if (!SplitCommand.at(1).compare("epthook") || !SplitCommand.at(1).compare("!epthook"))
    {
        PreallocRequest.Type = DEBUGGER_PREALLOC_COMMAND_TYPE_EPTHOOK;
    }
    else if (!SplitCommand.at(1).compare("epthook2") || !SplitCommand.at(1).compare("!epthook2"))
    {
        PreallocRequest.Type = DEBUGGER_PREALLOC_COMMAND_TYPE_EPTHOOK2;
    }
    else if (!SplitCommand.at(1).compare("regular-event"))
    {
        PreallocRequest.Type = DEBUGGER_PREALLOC_COMMAND_TYPE_REGULAR_EVENT;
    }
    else if (!SplitCommand.at(1).compare("big-event"))
    {
        PreallocRequest.Type = DEBUGGER_PREALLOC_COMMAND_TYPE_BIG_EVENT;
    }
    else if (!SplitCommand.at(1).compare("regular-safe-buffer"))
    {
        PreallocRequest.Type = DEBUGGER_PREALLOC_COMMAND_TYPE_REGULAR_SAFE_BUFFER;
    }
    else if (!SplitCommand.at(1).compare("big-safe-buffer"))
    {
        PreallocRequest.Type = DEBUGGER_PREALLOC_COMMAND_TYPE_BIG_SAFE_BUFFER;
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

    //
    // Get the count of needed pre-allocated buffers
    //
    if (!SymbolConvertNameOrExprToAddress(SplitCommand.at(2), &Count))
    {
        //
        // Couldn't resolve or unknown parameter
        //
        ShowMessages("err, couldn't resolve error at '%s'\n",
                     SplitCommand.at(2).c_str());
        return;
    }

    //
    // Set the counter
    //
    PreallocRequest.Count = (UINT32)Count;

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

    //
    // Send IOCTL
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                    // Handle to device
        IOCTL_RESERVE_PRE_ALLOCATED_POOLS, // IO Control Code (IOCTL)
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

    if (PreallocRequest.KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
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
