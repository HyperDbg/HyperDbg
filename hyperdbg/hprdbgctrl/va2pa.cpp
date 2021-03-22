/**
 * @file va2pa.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !va2pa command
 * @details
 * @version 0.1
 * @date 2020-07-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of !va2pa command
 *
 * @return VOID
 */
VOID
CommandVa2paHelp()
{
    ShowMessages("!va2pa : Converts virtual address to physical address.\n\n");
    ShowMessages("syntax : \t!va2pa [Virtual Address (hex value)] pid [Process "
                 "id (hex value)]\n");
    ShowMessages("\t\te.g : !va2pa fffff801deadbeef\n");
    ShowMessages("\t\te.g : !va2pa fffff801deadbeef pid 0xc8\n");
}

/**
 * @brief !va2pa command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandVa2pa(vector<string> SplittedCommand, string Command)
{
    BOOL                              Status;
    ULONG                             ReturnedLength;
    UINT64                            TargetVa;
    UINT32                            Pid            = GetCurrentProcessId();
    DEBUGGER_VA2PA_AND_PA2VA_COMMANDS AddressDetails = {0};

    if (SplittedCommand.size() == 1 || SplittedCommand.size() >= 5 ||
        SplittedCommand.size() == 3)
    {
        ShowMessages("incorrect use of '!va2pa'\n\n");
        CommandVa2paHelp();
        return;
    }

    if (SplittedCommand.size() == 2)
    {
        //
        // It's just a address for current process
        //
        if (!ConvertStringToUInt64(SplittedCommand.at(1), &TargetVa))
        {
            ShowMessages(
                "incorrect address, please enter a valid virtual address\n\n");
            return;
        }
    }
    else
    {
        //
        // It might be address + pid
        //
        if (!SplittedCommand.at(1).compare("pid"))
        {
            if (!ConvertStringToUInt32(SplittedCommand.at(2), &Pid))
            {
                ShowMessages("incorrect address, please enter a valid process id\n\n");
                return;
            }
            if (!ConvertStringToUInt64(SplittedCommand.at(3), &TargetVa))
            {
                ShowMessages(
                    "incorrect address, please enter a valid virtual address\n\n");
                return;
            }
        }
        else if (!SplittedCommand.at(2).compare("pid"))
        {
            if (!ConvertStringToUInt64(SplittedCommand.at(1), &TargetVa))
            {
                ShowMessages(
                    "incorrect address, please enter a valid virtual address\n\n");
                return;
            }
            if (!ConvertStringToUInt32(SplittedCommand.at(3), &Pid))
            {
                ShowMessages("incorrect address, please enter a valid process id\n\n");
                return;
            }
        }
        else
        {
            ShowMessages("incorrect use of '!va2pa'\n\n");
            CommandVa2paHelp();
            return;
        }
    }

    if (!g_DeviceHandle)
    {
        ShowMessages("Handle not found, probably the driver is not loaded. Did you "
                     "use 'load' command?\n");
        return;
    }

    //
    // Prepare the buffer
    // We use same buffer for input and output
    //
    AddressDetails.VirtualAddress     = TargetVa;
    AddressDetails.ProcessId          = Pid;
    AddressDetails.IsVirtual2Physical = TRUE;

    //
    // Send IOCTL
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                           // Handle to device
        IOCTL_DEBUGGER_VA2PA_AND_PA2VA_COMMANDS,  // IO Control code
        &AddressDetails,                          // Input Buffer to driver.
        SIZEOF_DEBUGGER_VA2PA_AND_PA2VA_COMMANDS, // Input buffer length
        &AddressDetails,                          // Output Buffer from driver.
        SIZEOF_DEBUGGER_VA2PA_AND_PA2VA_COMMANDS, // Length of output
                                                  // buffer in bytes.
        &ReturnedLength,                          // Bytes placed in buffer.
        NULL                                      // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return;
    }

    //
    // Show the results
    //
    ShowMessages("%llx\n", AddressDetails.PhysicalAddress);
}
