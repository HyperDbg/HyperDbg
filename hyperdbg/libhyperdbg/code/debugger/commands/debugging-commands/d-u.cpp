/**
 * @file d-u.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !u* u* , !d* d* commands
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
 * @brief help of u* d* !u* !d* commands
 *
 * @return VOID
 */
VOID
CommandReadMemoryAndDisassemblerHelp()
{
    ShowMessages("db dc dd dq !db !dc !dd !dq & u u64 !u !u64 u2 u32 !u2 !u32 & dl & !dl : reads the  "
                 "memory in different shapes (hex), disassembles, or walks linked lists\n");
    ShowMessages("db  Byte and ASCII characters\n");
    ShowMessages("dc  Double-word values (4 bytes) and ASCII characters\n");
    ShowMessages("dd  Double-word values (4 bytes)\n");
    ShowMessages("dq  Quad-word values (8 bytes). \n");
    ShowMessages("u u64 Disassembler at the target address (x64) \n");
    ShowMessages("u2 u32  Disassembler at the target address (x86) \n");
    ShowMessages("dl  Walks a linked list starting at an address and shows each node\n");
    ShowMessages("\nIf you want to read physical memory then add '!' at the "
                 "start of the command\n");
    ShowMessages("you can also disassemble physical memory using '!u'\n\n");

    ShowMessages("syntax : \tdb [Address (hex)] [l Length (hex)] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \tdc [Address (hex)] [l Length (hex)] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \tdd [Address (hex)] [l Length (hex)] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \tdq [Address (hex)] [l Length (hex)] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \tu [Address (hex)] [l Length (hex)] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \tu64 [Address (hex)] [l Length (hex)] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \tu2 [Address (hex)] [l Length (hex)] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \tu32 [Address (hex)] [l Length (hex)] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \tdl [Address (hex)] [o Offset (hex)] [l Count (hex)] [pid ProcessId (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : db nt!Kd_DEFAULT_Mask\n");
    ShowMessages("\t\te.g : db nt!Kd_DEFAULT_Mask+10\n");
    ShowMessages("\t\te.g : db @rax\n");
    ShowMessages("\t\te.g : db @rax+50\n");
    ShowMessages("\t\te.g : db fffff8077356f010\n");
    ShowMessages("\t\te.g : !dq 100000\n");
    ShowMessages("\t\te.g : !dq @rax+77\n");
    ShowMessages("\t\te.g : u32 @eip\n");
    ShowMessages("\t\te.g : u nt!ExAllocatePoolWithTag\n");
    ShowMessages("\t\te.g : u nt!ExAllocatePoolWithTag+30\n");
    ShowMessages("\t\te.g : u fffff8077356f010\n");
    ShowMessages("\t\te.g : u fffff8077356f010+@rcx\n");
    ShowMessages("\t\te.g : dl nt!PsActiveProcessHead\n");
    ShowMessages("\t\te.g : dl @rax o 8\n");
    ShowMessages("\t\te.g : dl fffff8077356f010 o 8 l 20 pid 4\n");
}

/**
 * @brief u* d* !u* !d* commands handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandReadMemoryAndDisassembler(vector<CommandToken> CommandTokens, string Command)
{
    UINT32  Pid             = 0;
    UINT32  Length          = 0;
    UINT64  Offset          = 0;
    UINT64  MaxNodes        = DL_DEFAULT_MAX_NODES;
    UINT64  TargetAddress   = 0;
    BOOLEAN IsNextProcessId = FALSE;
    BOOLEAN IsFirstCommand  = TRUE;
    BOOLEAN IsNextLength    = FALSE;
    BOOLEAN IsNextOffset    = FALSE;
    BOOLEAN IsDlCommand     = FALSE;

    string FirstCommand = GetCaseSensitiveStringFromCommandToken(CommandTokens.front());

    //
    // By default if the user-debugger is active, we use these commands
    // on the memory layout of the debuggee process
    //
    if (g_ActiveProcessDebuggingState.IsActive)
    {
        Pid = g_ActiveProcessDebuggingState.ProcessId;
    }

    if (CommandTokens.size() == 1)
    {
        //
        // Means that user entered one command without any parameter
        //
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandReadMemoryAndDisassemblerHelp();
        return;
    }

    for (auto Section : CommandTokens)
    {
        if (IsFirstCommand)
        {
            IsFirstCommand = FALSE;
            IsDlCommand    = CompareLowerCaseStrings(CommandTokens.at(0), "dl") |
                             CompareLowerCaseStrings(CommandTokens.at(0), "!dl");

            continue;
        }

        if (IsNextProcessId == TRUE)
        {
            if (!ConvertTokenToUInt32(Section, &Pid))
            {
                ShowMessages("err, you should enter a valid process id\n\n");
                return;
            }
            IsNextProcessId = FALSE;
            continue;
        }

        if (IsNextLength == TRUE)
        {
            //
            // For 'dl', 'l' means max node count rather than a byte length,
            // but it's parsed the same way
            //
            if (IsDlCommand)
            {
                if (!ConvertTokenToUInt64(Section, &MaxNodes))
                {
                    ShowMessages("err, you should enter a valid count\n\n");
                    return;
                }
            }
            else if (!ConvertTokenToUInt32(Section, &Length))
            {
                ShowMessages("err, you should enter a valid length\n\n");
                return;
            }
            IsNextLength = FALSE;
            continue;
        }

        if (IsNextOffset == TRUE)
        {
            if (!ConvertTokenToUInt64(Section, &Offset))
            {
                ShowMessages("err, you should enter a valid offset\n\n");
                return;
            }
            IsNextOffset = FALSE;
            continue;
        }

        if (CompareLowerCaseStrings(Section, "l"))
        {
            IsNextLength = TRUE;
            continue;
        }

        if (IsDlCommand && CompareLowerCaseStrings(Section, "o"))
        {
            IsNextOffset = TRUE;
            continue;
        }

        if (CompareLowerCaseStrings(Section, "pid"))
        {
            IsNextProcessId = TRUE;
            continue;
        }

        //
        // Probably it's address
        //
        if (TargetAddress == 0)
        {
            if (!SymbolConvertNameOrExprToAddress(GetCaseSensitiveStringFromCommandToken(Section), &TargetAddress))
            {
                //
                // Couldn't resolve or unknown parameter
                //
                ShowMessages("err, couldn't resolve error at '%s'\n",
                             GetCaseSensitiveStringFromCommandToken(Section).c_str());
                return;
            }
        }
        else
        {
            //
            // User inserts two address
            //
            ShowMessages("err, incorrect use of the '%s' command\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
            CommandReadMemoryAndDisassemblerHelp();

            return;
        }
    }

    if (!TargetAddress)
    {
        //
        // User inserts two address
        //
        ShowMessages("err, please enter a valid address\n\n");

        return;
    }

    //
    // Check if the user didn't specify a length for d* and u* commands, then we use default value
    //
    if (Length == 0 && !IsDlCommand)
    {
        //
        // Default length (user doesn't specified)
        //
        if (CompareLowerCaseStrings(CommandTokens.at(0), "u") ||
            CompareLowerCaseStrings(CommandTokens.at(0), "!u") ||
            CompareLowerCaseStrings(CommandTokens.at(0), "u64") ||
            CompareLowerCaseStrings(CommandTokens.at(0), "!u64"))
        {
            Length = 0x40;
        }
        else
        {
            Length = 0x80;
        }
    }

    //
    // Check if the user didn't specify a max node count for dl command, then we use default value
    //
    if (IsDlCommand && MaxNodes == 0)
    {
        MaxNodes = DL_DEFAULT_MAX_NODES;
    }

    if (IsNextLength || IsNextProcessId || IsNextOffset)
    {
        ShowMessages("incorrect use of the '%s' command\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandReadMemoryAndDisassemblerHelp();
        return;
    }

    //
    // Check to prevent using process id in d* and u* commands
    //
    if (g_IsSerialConnectedToRemoteDebuggee && Pid != 0)
    {
        ShowMessages(ASSERT_MESSAGE_CANNOT_SPECIFY_PID);
        return;
    }

    if (Pid == 0)
    {
        //
        // Default process we read from current process
        //
        Pid = PlatformGetCurrentProcessId();
    }

    if (CompareLowerCaseStrings(CommandTokens.at(0), "db"))
    {
        HyperDbgShowMemoryOrDisassemble(DEBUGGER_SHOW_COMMAND_DB,
                                        TargetAddress,
                                        DEBUGGER_READ_VIRTUAL_ADDRESS,
                                        READ_FROM_KERNEL,
                                        Pid,
                                        Length,
                                        NULL);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(0), "dc"))
    {
        HyperDbgShowMemoryOrDisassemble(DEBUGGER_SHOW_COMMAND_DC,
                                        TargetAddress,
                                        DEBUGGER_READ_VIRTUAL_ADDRESS,
                                        READ_FROM_KERNEL,
                                        Pid,
                                        Length,
                                        NULL);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(0), "dd"))
    {
        HyperDbgShowMemoryOrDisassemble(DEBUGGER_SHOW_COMMAND_DD,
                                        TargetAddress,
                                        DEBUGGER_READ_VIRTUAL_ADDRESS,
                                        READ_FROM_KERNEL,
                                        Pid,
                                        Length,
                                        NULL);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(0), "dq"))
    {
        HyperDbgShowMemoryOrDisassemble(DEBUGGER_SHOW_COMMAND_DQ,
                                        TargetAddress,
                                        DEBUGGER_READ_VIRTUAL_ADDRESS,
                                        READ_FROM_KERNEL,
                                        Pid,
                                        Length,
                                        NULL);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(0), "!db"))
    {
        HyperDbgShowMemoryOrDisassemble(DEBUGGER_SHOW_COMMAND_DB,
                                        TargetAddress,
                                        DEBUGGER_READ_PHYSICAL_ADDRESS,
                                        READ_FROM_KERNEL,
                                        Pid,
                                        Length,
                                        NULL);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(0), "!dc"))
    {
        HyperDbgShowMemoryOrDisassemble(DEBUGGER_SHOW_COMMAND_DC,
                                        TargetAddress,
                                        DEBUGGER_READ_PHYSICAL_ADDRESS,
                                        READ_FROM_KERNEL,
                                        Pid,
                                        Length,
                                        NULL);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(0), "!dd"))
    {
        HyperDbgShowMemoryOrDisassemble(DEBUGGER_SHOW_COMMAND_DD,
                                        TargetAddress,
                                        DEBUGGER_READ_PHYSICAL_ADDRESS,
                                        READ_FROM_KERNEL,
                                        Pid,
                                        Length,
                                        NULL);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(0), "!dq"))
    {
        HyperDbgShowMemoryOrDisassemble(DEBUGGER_SHOW_COMMAND_DQ,
                                        TargetAddress,
                                        DEBUGGER_READ_PHYSICAL_ADDRESS,
                                        READ_FROM_KERNEL,
                                        Pid,
                                        Length,
                                        NULL);
    }

    //
    // Disassembler (!u or u or u2 !u2)
    //
    else if (CompareLowerCaseStrings(CommandTokens.at(0), "u") || CompareLowerCaseStrings(CommandTokens.at(0), "u64"))
    {
        HyperDbgShowMemoryOrDisassemble(
            DEBUGGER_SHOW_COMMAND_DISASSEMBLE64,
            TargetAddress,
            DEBUGGER_READ_VIRTUAL_ADDRESS,
            READ_FROM_KERNEL,
            Pid,
            Length,
            NULL);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(0), "!u") || CompareLowerCaseStrings(CommandTokens.at(0), "!u64"))
    {
        HyperDbgShowMemoryOrDisassemble(
            DEBUGGER_SHOW_COMMAND_DISASSEMBLE64,
            TargetAddress,
            DEBUGGER_READ_PHYSICAL_ADDRESS,
            READ_FROM_KERNEL,
            Pid,
            Length,
            NULL);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(0), "u2") || CompareLowerCaseStrings(CommandTokens.at(0), "u32"))
    {
        HyperDbgShowMemoryOrDisassemble(
            DEBUGGER_SHOW_COMMAND_DISASSEMBLE32,
            TargetAddress,
            DEBUGGER_READ_VIRTUAL_ADDRESS,
            READ_FROM_KERNEL,
            Pid,
            Length,
            NULL);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(0), "!u2") || CompareLowerCaseStrings(CommandTokens.at(0), "!u32"))
    {
        HyperDbgShowMemoryOrDisassemble(
            DEBUGGER_SHOW_COMMAND_DISASSEMBLE32,
            TargetAddress,
            DEBUGGER_READ_PHYSICAL_ADDRESS,
            READ_FROM_KERNEL,
            Pid,
            Length,
            NULL);
    }
    else if (IsDlCommand)
    {
        HyperDbgShowMemoryLinkedList(TargetAddress,
                                     CompareLowerCaseStrings(CommandTokens.at(0), "dl") ? DEBUGGER_READ_VIRTUAL_ADDRESS : DEBUGGER_READ_PHYSICAL_ADDRESS,
                                     Pid,
                                     Offset,
                                     MaxNodes);
    }
}
