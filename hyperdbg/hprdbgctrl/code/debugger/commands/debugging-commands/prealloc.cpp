/**
 * @file prealloc.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief prealloc command
 * @details
 * @version 0.1
 * @date 2020-11-13
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\hprdbgctrl\pch.h"

/**
 * @brief help of prealloc command
 *
 * @return VOID
 */
VOID
CommandPreallocHelp()
{
    ShowMessages("prealloc : Pre-allocates buffer for special purposes.\n\n");
    ShowMessages("syntax : \tprealloc [Type (monitor)] [Count (hex value)]\n");

    ShowMessages("\t\te.g : prealloc monitor 10\n");
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

    if (!SplittedCommand.at(1).compare("monitor"))
    {
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
        // Set the details
        //
        PreallocRequest.Type  = DEBUGGER_PREALLOC_COMMAND_TYPE_MONITOR;
        PreallocRequest.Count = Count;
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

    if (!g_DeviceHandle)
    {
        ShowMessages("handle of the driver not found, probably the driver is not loaded. Did you "
                     "use 'load' command?\n");
        return;
    }

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

    if (PreallocRequest.KernelStatus == DEBUGEER_OPERATION_WAS_SUCCESSFULL)
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
