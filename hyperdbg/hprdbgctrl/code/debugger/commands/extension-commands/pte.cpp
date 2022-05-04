/**
 * @file pte.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !pte command
 * @details
 * @version 0.1
 * @date 2020-05-27
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
 * @brief help of !pte command
 *
 * @return VOID
 */
VOID
CommandPteHelp()
{
    ShowMessages("!pte : finds virtual addresses of different paging-levels.\n\n");

    ShowMessages("syntax : \t!pte [VirtualAddress (hex)] [pid ProcessId (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !pte nt!ExAllocatePoolWithTag\n");
    ShowMessages("\t\te.g : !pte nt!ExAllocatePoolWithTag+5\n");
    ShowMessages("\t\te.g : !pte fffff801deadbeef\n");
    ShowMessages("\t\te.g : !pte 0x400021000 pid 1c0\n");
}

/**
 * @brief show results of !pte command
 *
 * @param TargetVa
 * @param PteRead
 * @return VOID
 */
VOID
CommandPteShowResults(UINT64 TargetVa, PDEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS PteRead)
{
    /*
    VA fffff8003abc9370
    PXE at FFFF83C1E0F07F80    PPE at FFFF83C1E0FF0000    PDE at
    FFFF83C1FE000EA8    PTE at FFFF83FC001D5E48 contains 0000000004108063
    contains 0000000004109063  contains 00000000026008E3  contains
    0000000000000000 pfn 4108      ---DA--KWEV  pfn 4109      ---DA--KWEV  pfn
    2600      --LDA--KWEV  LARGE PAGE pfn 27c9
  */
    ShowMessages("VA %llx\n", TargetVa);
    ShowMessages("PML4E (PXE) at %016llx\tcontains %016llx\nPDPT (PPE) at "
                 "%016llx\tcontains "
                 "%016llx\nPDE at %016llx\tcontains %016llx\n",
                 PteRead->Pml4eVirtualAddress,
                 PteRead->Pml4eValue,
                 PteRead->PdpteVirtualAddress,
                 PteRead->PdpteValue,
                 PteRead->PdeVirtualAddress,
                 PteRead->PdeValue);

    //
    // Check if it's a large PDE
    //
    if (PteRead->PdeVirtualAddress == PteRead->PteVirtualAddress)
    {
        ShowMessages("PDE is a large page, so it doesn't have a PTE\n");
    }
    else
    {
        ShowMessages("PTE at %016llx\tcontains %016llx\n",
                     PteRead->PteVirtualAddress,
                     PteRead->PteValue);
    }
}

/**
 * @brief !pte command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandPte(vector<string> SplittedCommand, string Command)
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

        if (AddressDetails.KernelStatus != DEBUGGER_OPERATION_WAS_SUCCESSFULL)
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
