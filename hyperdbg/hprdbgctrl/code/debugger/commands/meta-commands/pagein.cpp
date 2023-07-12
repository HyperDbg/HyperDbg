/**
 * @file pagein.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .pagein command
 * @details
 * @version 0.4
 * @date 2023-07-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN                  g_IsSerialConnectedToRemoteDebuggee;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

/**
 * @brief help of .pagein command
 *
 * @return VOID
 */
VOID
CommandPageinHelp()
{
    ShowMessages(".pagein : brings the page in, making it available in the RAM.\n\n");

    ShowMessages("syntax : \t!pagein [Mode (string)] [VirtualAddress (hex)] [pid ProcessId (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .pagein fffff801deadbeef\n");
    ShowMessages("\t\te.g : .pagein w 00007ff8349f2224\n");
    ShowMessages("\t\te.g : .pagein pw 00007ff8349f2224\n");
    ShowMessages("\t\te.g : .pagein wu 00007ff8349f2224\n");
    ShowMessages("\t\te.g : .pagein pwu @rax+5\n");
    ShowMessages("\t\te.g : .pagein pf @rip\n");
    ShowMessages("\t\te.g : .pagein uf @rip\n");

    ShowMessages("\n");
    ShowMessages("valid mode formats: \n");

    ShowMessages("\tp : present\n");
    ShowMessages("\tw : write\n");
    ShowMessages("\tu : user\n");
    ShowMessages("\tf : fetch\n");

    ShowMessages("\n");
    ShowMessages("common page-fault codes: \n");

    ShowMessages("\t0x0:  (default)\n");
    ShowMessages("\t0x2:  w (write access fault)\n");
    ShowMessages("\t0x3:  pw (present, write access fault)\n");
    ShowMessages("\t0x4:  u (user access fault)\n");
    ShowMessages("\t0x6:  wu (write, user access fault)\n");
    ShowMessages("\t0x7:  pwu (present, write, user access fault)\n");
    ShowMessages("\t0x10: f (fetch instruction fault)\n");
    ShowMessages("\t0x11: pf (present, fetch instruction fault)\n");
    ShowMessages("\t0x14: uf (user, fetch instruction fault)\n");
}

/**
 * @brief .pagein command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandPagein(vector<string> SplittedCommand, string Command)
{
    BOOL                                     Status;
    ULONG                                    ReturnedLength;
    UINT64                                   TargetVa;
    UINT32                                   Pid            = 0;
    DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS AddressDetails = {0};
    vector<string>                           SplittedCommandCaseSensitive {Split(Command, ' ')};

    if (SplittedCommand.size() == 1 || SplittedCommand.size() >= 5 ||
        SplittedCommand.size() == 3)
    {
        ShowMessages("incorrect use of '!pte'\n\n");
        CommandPteHelp();
        return;
    }

    //
    // By default if the user-debugger is active, we use these commands
    // on the memory layout of the debuggee process
    //
    if (g_ActiveProcessDebuggingState.IsActive)
    {
        Pid = g_ActiveProcessDebuggingState.ProcessId;
    }

    if (SplittedCommand.size() == 2)
    {
        //
        // It's just an address for current process
        //
        if (!SymbolConvertNameOrExprToAddress(SplittedCommandCaseSensitive.at(1), &TargetVa))
        {
            //
            // Couldn't resolve or unkonwn parameter
            //
            ShowMessages("err, couldn't resolve error at '%s'\n",
                         SplittedCommandCaseSensitive.at(1).c_str());
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
                ShowMessages("incorrect address, please enter a valid process id\n");
                return;
            }

            if (!SymbolConvertNameOrExprToAddress(SplittedCommandCaseSensitive.at(3), &TargetVa))
            {
                //
                // Couldn't resolve or unkonwn parameter
                //
                ShowMessages("err, couldn't resolve error at '%s'\n",
                             SplittedCommandCaseSensitive.at(3).c_str());
                return;
            }
        }
        else if (!SplittedCommand.at(2).compare("pid"))
        {
            if (!SymbolConvertNameOrExprToAddress(SplittedCommandCaseSensitive.at(1), &TargetVa))
            {
                //
                // Couldn't resolve or unkonwn parameter
                //
                ShowMessages("err, couldn't resolve error at '%s'\n\n",
                             SplittedCommandCaseSensitive.at(1).c_str());
                return;
            }

            if (!ConvertStringToUInt32(SplittedCommand.at(3), &Pid))
            {
                ShowMessages("incorrect address, please enter a valid process id\n");
                return;
            }
        }
        else
        {
            ShowMessages("incorrect use of '!pte'\n\n");
            CommandPteHelp();
            return;
        }
    }

    //
    // Prepare the buffer
    // We use same buffer for input and output
    //
    AddressDetails.VirtualAddress = TargetVa;
    AddressDetails.ProcessId      = Pid; // null in debugger mode

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Check to prevent using process id in !pte command
        //
        if (Pid != 0)
        {
            ShowMessages("err, you cannot specify 'pid' in the debugger mode\n");
            return;
        }

        //
        // Send the request over serial kernel debugger
        //

        KdSendPtePacketToDebuggee(&AddressDetails);
    }
    else
    {
        AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

        if (Pid == 0)
        {
            Pid                      = GetCurrentProcessId();
            AddressDetails.ProcessId = Pid;
        }

        //
        // Send IOCTL
        //
        Status = DeviceIoControl(
            g_DeviceHandle,                                  // Handle to device
            IOCTL_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS,  // IO Control code
            &AddressDetails,                                 // Input Buffer to driver.
            SIZEOF_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS, // Input buffer length
            &AddressDetails,                                 // Output Buffer from driver.
            SIZEOF_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS, // Length of output
                                                             // buffer in bytes.
            &ReturnedLength,                                 // Bytes placed in buffer.
            NULL                                             // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
            return;
        }

        if (AddressDetails.KernelStatus != DEBUGGER_OPERATION_WAS_SUCCESSFUL)
        {
            ShowErrorMessage(AddressDetails.KernelStatus);
            return;
        }

        //
        // Show the results
        //
        CommandPteShowResults(TargetVa, &AddressDetails);
    }
}
