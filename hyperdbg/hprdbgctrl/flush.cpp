/**
 * @file flush.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief flush command
 * @details
 * @version 0.1
 * @date 2020-08-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of flush command
 *
 * @return VOID
 */
VOID
CommandFlushHelp()
{
    ShowMessages("flush : Removes all the buffer and messages from kernel-mode "
                 "buffers.\n\n");
    ShowMessages("syntax : \tflush\n");
}

/**
 * @brief flush command handler
 *
 * @return VOID
 */
VOID
CommandFlushRequestFlush()
{
    BOOL                           Status;
    ULONG                          ReturnedLength;
    DEBUGGER_FLUSH_LOGGING_BUFFERS FlushRequest = {0};

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // It's a debug-mode
        //
        KdSendFlushPacketToDebuggee();
    }
    else
    {
        //
        // It's a vmi-mode
        //

        if (!g_DeviceHandle)
        {
            ShowMessages(
                "Handle not found, probably the driver is not loaded. Did you "
                "use 'load' command?\n");
            return;
        }

        //
        // By the way, we don't need to send an input buffer
        // to the kernel, but let's keep it like this, if we
        // want to pass some other aguments to the kernel in
        // the future
        //
        Status = DeviceIoControl(
            g_DeviceHandle,                        // Handle to device
            IOCTL_DEBUGGER_FLUSH_LOGGING_BUFFERS,  // IO Control code
            &FlushRequest,                         // Input Buffer to driver.
            SIZEOF_DEBUGGER_FLUSH_LOGGING_BUFFERS, // Input buffer length
            &FlushRequest,                         // Output Buffer from driver.
            SIZEOF_DEBUGGER_FLUSH_LOGGING_BUFFERS, // Length of output buffer in
                                                   // bytes.
            &ReturnedLength,                       // Bytes placed in buffer.
            NULL                                   // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
            return;
        }

        if (FlushRequest.KernelStatus == DEBUGEER_OPERATION_WAS_SUCCESSFULL)
        {
            //
            // The amount of message that are deleted are the amount of
            // vmx-root messages and vmx non-root messages
            //
            ShowMessages(
                "flushing buffers was successful, total %d messages were cleared.\n",
                FlushRequest.CountOfMessagesThatSetAsReadFromVmxNonRoot +
                    FlushRequest.CountOfMessagesThatSetAsReadFromVmxRoot);
        }
        else
        {
            ShowMessages("flushing buffers was not successful :(\n");
        }
    }
}

/**
 * @brief flush command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandFlush(vector<string> SplittedCommand, string Command)
{
    if (SplittedCommand.size() != 1)
    {
        ShowMessages("incorrect use of 'flush'\n\n");
        CommandFlushHelp();
        return;
    }

    //
    // Flush the buffer
    //
    CommandFlushRequestFlush();
}
