/**
 * @file pa2va.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !pa2va command
 * @details
 * @version 0.1
 * @date 2020-07-16
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
 * @brief help of !pa2va command
 *
 * @return VOID
 */
VOID
CommandPa2vaHelp()
{
    ShowMessages("!pa2va : Converts virtual address to physical address.\n\n");
    ShowMessages("syntax : \t!pa2va [Virtual Address (hex value)] pid [Process "
                 "id (hex value)]\n");
    ShowMessages("\t\te.g : !pa2va nt!ExAllocatePoolWithTag\n");
    ShowMessages("\t\te.g : !pa2va fffff801deadbeef\n");
    ShowMessages("\t\te.g : !pa2va fffff801deadbeef pid 0xc8\n");
}

/**
 * @brief !pa2va command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandPa2va(vector<string> SplittedCommand, string Command)
{
    BOOL                              Status;
    ULONG                             ReturnedLength;
    UINT64                            TargetPa;
    UINT32                            Pid            = 0;
    DEBUGGER_VA2PA_AND_PA2VA_COMMANDS AddressDetails = {0};

    if (SplittedCommand.size() == 1 || SplittedCommand.size() >= 5 ||
        SplittedCommand.size() == 3)
    {
        ShowMessages("incorrect use of '!pa2va'\n\n");
        CommandPa2vaHelp();
        return;
    }

    if (SplittedCommand.size() == 2)
    {
        //
        // It's just a address for current process
        //
        if (!SymbolConvertNameToAddress(SplittedCommand.at(1), &TargetPa))
        {
            ShowMessages("incorrect address or object name, please enter a valid physical"
                         " address\n\n");
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
            if (!SymbolConvertNameToAddress(SplittedCommand.at(3), &TargetPa))
            {
                ShowMessages("incorrect address or object name, please enter a valid physical"
                             " address\n\n");
                return;
            }
        }
        else if (!SplittedCommand.at(2).compare("pid"))
        {
            if (!SymbolConvertNameToAddress(SplittedCommand.at(1), &TargetPa))
            {
                ShowMessages("incorrect address or object name, please enter a valid physical"
                             " address\n\n");
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
            ShowMessages("incorrect use of '!pa2va'\n\n");
            CommandPa2vaHelp();
            return;
        }
    }

    if (!g_DeviceHandle)
    {
        ShowMessages("handle of the driver not found, probably the driver is not loaded. Did you "
                     "use 'load' command?\n");
        return;
    }

    //
    // Check to prevent using process id in !pa2va command
    //
    if (g_IsSerialConnectedToRemoteDebuggee && Pid != 0)
    {
        ShowMessages("err, you cannot specify 'pid' in the debugger mode\n\n");
        return;
    }

    if (Pid == 0)
    {
        Pid = GetCurrentProcessId();
    }

    //
    // Prepare the buffer
    // We use same buffer for input and output
    //
    AddressDetails.PhysicalAddress    = TargetPa;
    AddressDetails.ProcessId          = Pid;
    AddressDetails.IsVirtual2Physical = FALSE;

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

    if (AddressDetails.KernelStatus == DEBUGEER_OPERATION_WAS_SUCCESSFULL)
    {
        //
        // Show the results
        //
        ShowMessages("%llx\n", AddressDetails.VirtualAddress);
    }
    else
    {
        //
        // An err occurred, no results
        //
        ShowErrorMessage(AddressDetails.KernelStatus);
    }
}
