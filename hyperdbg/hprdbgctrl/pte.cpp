/**
 * @file pte.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !pte command
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of !pte command
 *
 * @return VOID
 */
VOID
CommandPteHelp()
{
    ShowMessages("!pte : Find virtual address of different paging-levels.\n\n");
    ShowMessages("syntax : \t!pte [Virtual Address (hex value)]\n");
    ShowMessages("\t\te.g : !pte fffff801deadbeef\n");
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
    DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS PteRead = {0};

    if (SplittedCommand.size() != 2)
    {
        ShowMessages("incorrect use of '!pte'\n\n");
        CommandPteHelp();
        return;
    }

    if (!ConvertStringToUInt64(SplittedCommand.at(1), &TargetVa))
    {
        ShowMessages("incorrect address, please enter a valid virtual address\n\n");
        return;
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
    PteRead.VirtualAddress = TargetVa;

    //
    // Send IOCTL
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                                  // Handle to device
        IOCTL_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS,  // IO Control code
        &PteRead,                                        // Input Buffer to driver.
        SIZEOF_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS, // Input buffer length
        &PteRead,                                        // Output Buffer from driver.
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

    if (PteRead.KernelStatus != DEBUGEER_OPERATION_WAS_SUCCESSFULL)
    {
        ShowErrorMessage(PteRead.KernelStatus);
        return;
    }

    //
    // Show the results
    //
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
                 PteRead.Pml4eVirtualAddress,
                 PteRead.Pml4eValue,
                 PteRead.PdpteVirtualAddress,
                 PteRead.PdpteValue,
                 PteRead.PdeVirtualAddress,
                 PteRead.PdeValue);

    //
    // Check if it's a large PDE
    //
    if (PteRead.PdeVirtualAddress == PteRead.PteVirtualAddress)
    {
        ShowMessages("PDE is a large page, so it doesn't have a PTE\n");
    }
    else
    {
        ShowMessages("PTE at %016llx\tcontains %016llx\n",
                     PteRead.PteVirtualAddress,
                     PteRead.PteValue);
    }
}
