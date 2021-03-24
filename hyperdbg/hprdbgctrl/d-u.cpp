/**
 * @file d-u.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief !u* u* , !d* d* commands
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of u* d* !u* !d* commands
 *
 * @return VOID
 */
VOID
CommandReadMemoryAndDisassemblerHelp()
{
    ShowMessages("u !u u2 !u2 & db dc dd dq !db !dc !dd !dq : read the  "
                 "memory different shapes (hex) and disassembler\n");
    ShowMessages("d[b]  Byte and ASCII characters\n");
    ShowMessages("d[c]  Double-word values (4 bytes) and ASCII characters\n");
    ShowMessages("d[d]  Double-word values (4 bytes)\n");
    ShowMessages("d[q]  Quad-word values (8 bytes). \n");
    ShowMessages("u  Disassembler at the target address (x64) \n");
    ShowMessages("u2  Disassembler at the target address (x86) \n");
    ShowMessages("\n If you want to read physical memory then add '!' at the "
                 "start of the command\n");
    ShowMessages("You can also disassemble physical memory using '!u'\n");

    ShowMessages("syntax : \t[!]d[b|c|d|q] [address] l [length (hex)] pid "
                 "[process id (hex)]\n");
    ShowMessages("\t\te.g : db fffff8077356f010 \n");
    ShowMessages("\t\te.g : !dq 100000\n");
    ShowMessages("\t\te.g : u fffff8077356f010\n");
}

/**
 * @brief u* d* !u* !d* commands handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandReadMemoryAndDisassembler(vector<string> SplittedCommand,
                                 string         Command)
{
    UINT32  Pid             = 0;
    UINT32  Length          = 0;
    UINT64  TargetAddress   = 0;
    BOOLEAN IsNextProcessId = FALSE;
    BOOLEAN IsFirstCommand  = TRUE;
    BOOLEAN IsNextLength    = FALSE;

    string FirstCommand = SplittedCommand.front();

    if (SplittedCommand.size() == 1)
    {
        //
        // Means that user entered just a connect so we have to
        // ask to connect to what ?
        //
        ShowMessages("incorrect use of '%s' command\n\n", FirstCommand.c_str());
        CommandReadMemoryAndDisassemblerHelp();
        return;
    }

    for (auto Section : SplittedCommand)
    {
        if (IsFirstCommand)
        {
            IsFirstCommand = FALSE;
            continue;
        }
        if (IsNextProcessId == TRUE)
        {
            if (!ConvertStringToUInt32(Section, &Pid))
            {
                ShowMessages("Err, you should enter a valid proc id\n\n");
                return;
            }
            IsNextProcessId = FALSE;
            continue;
        }

        if (IsNextLength == TRUE)
        {
            if (!ConvertStringToUInt32(Section, &Length))
            {
                ShowMessages("Err, you should enter a valid length\n\n");
                return;
            }
            IsNextLength = FALSE;
            continue;
        }

        if (!Section.compare("l"))
        {
            IsNextLength = TRUE;
            continue;
        }

        if (!Section.compare("pid"))
        {
            IsNextProcessId = TRUE;
            continue;
        }

        //
        // Probably it's address
        //
        if (TargetAddress == 0)
        {
            string TempAddress = Section;
            TempAddress.erase(remove(TempAddress.begin(), TempAddress.end(), '`'),
                              TempAddress.end());

            if (!ConvertStringToUInt64(TempAddress, &TargetAddress))
            {
                ShowMessages("Err, you should enter a valid address\n\n");
                return;
            }
        }
        else
        {
            //
            // User inserts two address
            //
            ShowMessages("Err, incorrect use of '%s' command\n\n",
                         FirstCommand.c_str());
            CommandReadMemoryAndDisassemblerHelp();

            return;
        }
    }
    if (!TargetAddress)
    {
        //
        // User inserts two address
        //
        ShowMessages("Err, Please enter a valid address.\n\n");

        return;
    }
    if (Length == 0)
    {
        //
        // Default length (user doesn't specified)
        //
        if (!FirstCommand.compare("u") || !FirstCommand.compare("!u"))
        {
            Length = 0x40;
        }
        else
        {
            Length = 0x80;
        }
    }
    if (IsNextLength || IsNextProcessId)
    {
        ShowMessages("incorrect use of '%s' command\n\n", FirstCommand.c_str());
        CommandReadMemoryAndDisassemblerHelp();
        return;
    }
    if (Pid == 0)
    {
        //
        // Default process we read from current process
        //
        Pid = GetCurrentProcessId();
    }

    if (!FirstCommand.compare("db"))
    {
        HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DB,
                                         TargetAddress,
                                         DEBUGGER_READ_VIRTUAL_ADDRESS,
                                         READ_FROM_KERNEL,
                                         Pid,
                                         Length);
    }
    else if (!FirstCommand.compare("dc"))
    {
        HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DC,
                                         TargetAddress,
                                         DEBUGGER_READ_VIRTUAL_ADDRESS,
                                         READ_FROM_KERNEL,
                                         Pid,
                                         Length);
    }
    else if (!FirstCommand.compare("dd"))
    {
        HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DD,
                                         TargetAddress,
                                         DEBUGGER_READ_VIRTUAL_ADDRESS,
                                         READ_FROM_KERNEL,
                                         Pid,
                                         Length);
    }
    else if (!FirstCommand.compare("dq"))
    {
        HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DQ,
                                         TargetAddress,
                                         DEBUGGER_READ_VIRTUAL_ADDRESS,
                                         READ_FROM_KERNEL,
                                         Pid,
                                         Length);
    }
    else if (!FirstCommand.compare("!db"))
    {
        HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DB,
                                         TargetAddress,
                                         DEBUGGER_READ_PHYSICAL_ADDRESS,
                                         READ_FROM_KERNEL,
                                         Pid,
                                         Length);
    }
    else if (!FirstCommand.compare("!dc"))
    {
        HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DC,
                                         TargetAddress,
                                         DEBUGGER_READ_PHYSICAL_ADDRESS,
                                         READ_FROM_KERNEL,
                                         Pid,
                                         Length);
    }
    else if (!FirstCommand.compare("!dd"))
    {
        HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DD,
                                         TargetAddress,
                                         DEBUGGER_READ_PHYSICAL_ADDRESS,
                                         READ_FROM_KERNEL,
                                         Pid,
                                         Length);
    }
    else if (!FirstCommand.compare("!dq"))
    {
        HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DQ,
                                         TargetAddress,
                                         DEBUGGER_READ_PHYSICAL_ADDRESS,
                                         READ_FROM_KERNEL,
                                         Pid,
                                         Length);
    }

    //
    // Disassembler (!u or u or u2 !u2)
    //
    else if (!FirstCommand.compare("u"))
    {
        HyperDbgReadMemoryAndDisassemble(
            DEBUGGER_SHOW_COMMAND_DISASSEMBLE64,
            TargetAddress,
            DEBUGGER_READ_VIRTUAL_ADDRESS,
            READ_FROM_KERNEL,
            Pid,
            Length);
    }
    else if (!FirstCommand.compare("!u"))
    {
        HyperDbgReadMemoryAndDisassemble(
            DEBUGGER_SHOW_COMMAND_DISASSEMBLE64,
            TargetAddress,
            DEBUGGER_READ_PHYSICAL_ADDRESS,
            READ_FROM_KERNEL,
            Pid,
            Length);
    }
    else if (!FirstCommand.compare("u2"))
    {
        HyperDbgReadMemoryAndDisassemble(
            DEBUGGER_SHOW_COMMAND_DISASSEMBLE32,
            TargetAddress,
            DEBUGGER_READ_VIRTUAL_ADDRESS,
            READ_FROM_KERNEL,
            Pid,
            Length);
    }
    else if (!FirstCommand.compare("!u2"))
    {
        HyperDbgReadMemoryAndDisassemble(
            DEBUGGER_SHOW_COMMAND_DISASSEMBLE32,
            TargetAddress,
            DEBUGGER_READ_PHYSICAL_ADDRESS,
            READ_FROM_KERNEL,
            Pid,
            Length);
    }
}
