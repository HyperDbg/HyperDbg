/**
 * @file idt.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !idt command
 * @details
 * @version 0.12
 * @date 2024-12-30
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN g_AddressConversion;

/**
 * @brief help of the !idt command
 *
 * @return VOID
 */
VOID
CommandIdtHelp()
{
    ShowMessages("!ioapic : shows entries of Interrupt Descriptor Table (IDT).\n\n");

    ShowMessages("syntax : \t!idt [IdtEntry (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !idt\n");
    ShowMessages("\t\te.g : !idt 1d\n");
}

/**
 * @brief Send IDT entry requests
 *
 * @param IdtPacket
 *
 * @return VOID
 */
BOOLEAN
HyperDbgGetIdtEntry(INTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS * IdtPacket)
{
    BOOL  Status;
    ULONG ReturnedLength;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Send the request over serial kernel debugger
        //
        if (!KdSendQueryIdtPacketsToDebuggee(IdtPacket))
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else
    {
        AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

        //
        // Send IOCTL
        //
        Status = DeviceIoControl(
            g_DeviceHandle,                                    // Handle to device
            IOCTL_QUERY_IDT_ENTRY,                             // IO Control Code (IOCTL)
            IdtPacket,                                         // Input Buffer to driver.
            SIZEOF_INTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS, // Input buffer length (not used in this case)
            IdtPacket,                                         // Output Buffer from driver.
            SIZEOF_INTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS, // Length of output buffer in bytes.
            &ReturnedLength,                                   // Bytes placed in buffer.
            NULL                                               // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());

            return FALSE;
        }

        if (IdtPacket->KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
        {
            return TRUE;
        }
        else
        {
            //
            // An err occurred, no results
            //
            ShowErrorMessage(IdtPacket->KernelStatus);

            return FALSE;
        }
    }
}

/**
 * @brief !idt command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandIdt(vector<CommandToken> CommandTokens, string Command)
{
    UINT32                                       IdtEntry;
    INTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS * IdtPacket       = NULL;
    BOOLEAN                                      ShowAllEntries  = TRUE;
    UINT64                                       UsedBaseAddress = NULL;

    //
    // Check if the command should show all entries or just one entry
    //
    if (CommandTokens.size() == 1)
    {
        ShowAllEntries = TRUE;
    }
    else if (CommandTokens.size() == 2)
    {
        ShowAllEntries = FALSE;

        //
        // Get the IDT entry number
        //
        if (ConvertTokenToUInt32(CommandTokens.at(1), &IdtEntry) == FALSE)
        {
            ShowMessages("err, invalid IDT entry number\n");
            return;
        }

        if (IdtEntry > MAX_NUMBER_OF_IDT_ENTRIES)
        {
            ShowMessages("err, invalid IDT entry number\n");
            return;
        }
    }
    else
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());

        CommandIdtHelp();
        return;
    }

    //
    // Allocate buffer for IDT entry
    //
    IdtPacket = (INTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS *)malloc(sizeof(INTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS));

    if (IdtPacket == NULL)
    {
        ShowMessages("err, allocating buffer for receiving IDT entries");
    }

    RtlZeroMemory(IdtPacket, sizeof(INTERRUPT_DESCRIPTOR_TABLE_ENTRIES_PACKETS));

    //
    // Get the IDT buffer
    //
    if (HyperDbgGetIdtEntry(IdtPacket) == TRUE)
    {
        //
        // Show (dump) entries
        //
        if (ShowAllEntries)
        {
            ShowMessages("IDT Entries:\n\n");

            for (UINT32 i = 0; i < MAX_NUMBER_OF_IDT_ENTRIES; i++)
            {
                ShowMessages("IDT[0x%x:%d]\t: %s\t",
                             i,
                             i,
                             SeparateTo64BitValue(IdtPacket->IdtEntry[i]).c_str());

                //
                // Apply addressconversion of settings here
                //
                if (g_AddressConversion)
                {
                    //
                    // Showing function names here
                    //
                    if (SymbolShowFunctionNameBasedOnAddress(IdtPacket->IdtEntry[i], &UsedBaseAddress))
                    {
                        //
                        // The symbol address is showed (nothing to do)
                        //
                    }
                }

                ShowMessages("\n");
            }
        }
        else
        {
            ShowMessages("IDT[0x%x:%d] : %s\t",
                         IdtEntry,
                         IdtEntry,
                         SeparateTo64BitValue(IdtPacket->IdtEntry[IdtEntry]).c_str());

            //
            // Apply addressconversion of settings here
            //
            if (g_AddressConversion)
            {
                //
                // Showing function names here
                //
                if (SymbolShowFunctionNameBasedOnAddress(IdtPacket->IdtEntry[IdtEntry], &UsedBaseAddress))
                {
                    //
                    // The symbol address is showed (nothing to do)
                    //
                }

                ShowMessages("\n");
            }
        }
    }

    //
    // Deallocate the buffer
    //
    free(IdtPacket);
}
