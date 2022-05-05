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
    ShowMessages("db dc dd dq !db !dc !dd !dq & u !u u2 !u2 : reads the  "
                 "memory different shapes (hex) and disassembler\n");
    ShowMessages("db  Byte and ASCII characters\n");
    ShowMessages("dc  Double-word values (4 bytes) and ASCII characters\n");
    ShowMessages("dd  Double-word values (4 bytes)\n");
    ShowMessages("dq  Quad-word values (8 bytes). \n");
    ShowMessages("u  Disassembler at the target address (x64) \n");
    ShowMessages("u2  Disassembler at the target address (x86) \n");
    ShowMessages("\nIf you want to read physical memory then add '!' at the "
                 "start of the command\n");
    ShowMessages("you can also disassemble physical memory using '!u'\n\n");

    ShowMessages("syntax : \tdb [Address (hex)] [l Length (hex)] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \tdc [Address (hex)] [l Length (hex)] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \tdd [Address (hex)] [l Length (hex)] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \tdq [Address (hex)] [l Length (hex)] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \tu [Address (hex)] [l Length (hex)] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \tu2 [Address (hex)] [l Length (hex)] [pid ProcessId (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : db nt!Kd_DEFAULT_Mask\n");
    ShowMessages("\t\te.g : db nt!Kd_DEFAULT_Mask+10\n");
    ShowMessages("\t\te.g : db @rax\n");
    ShowMessages("\t\te.g : db @rax+50\n");
    ShowMessages("\t\te.g : db fffff8077356f010\n");
    ShowMessages("\t\te.g : !dq 100000\n");
    ShowMessages("\t\te.g : !dq @rax+77\n");
    ShowMessages("\t\te.g : u nt!ExAllocatePoolWithTag\n");
    ShowMessages("\t\te.g : u nt!ExAllocatePoolWithTag+30\n");
    ShowMessages("\t\te.g : u fffff8077356f010\n");
    ShowMessages("\t\te.g : u fffff8077356f010+@rcx\n");
}

/**
 * @brief u* d* !u* !d* commands handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandReadMemoryAndDisassembler(vector<string> SplittedCommand, string Command)
{
    UINT32         Pid             = 0;
    UINT32         Length          = 0;
    UINT64         TargetAddress   = 0;
    BOOLEAN        IsNextProcessId = FALSE;
    BOOLEAN        IsFirstCommand  = TRUE;
    BOOLEAN        IsNextLength    = FALSE;
    vector<string> SplittedCommandCaseSensitive {Split(Command, ' ')};
    UINT32         IndexInCommandCaseSensitive = 0;

    string FirstCommand = SplittedCommand.front();

    //
    // By default if the user-debugger is active, we use these commands
    // on the memory layout of the debuggee process
    //
    if (g_ActiveProcessDebuggingState.IsActive)
    {
        Pid = g_ActiveProcessDebuggingState.ProcessId;
    }

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
        IndexInCommandCaseSensitive++;

        if (IsFirstCommand)
        {
            IsFirstCommand = FALSE;
            continue;
        }
        if (IsNextProcessId == TRUE)
        {
            if (!ConvertStringToUInt32(Section, &Pid))
            {
                ShowMessages("err, you should enter a valid process id\n\n");
                return;
            }
            IsNextProcessId = FALSE;
            continue;
        }

        if (IsNextLength == TRUE)
        {
            if (!ConvertStringToUInt32(Section, &Length))
            {
                ShowMessages("err, you should enter a valid length\n\n");
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
            if (!SymbolConvertNameOrExprToAddress(SplittedCommandCaseSensitive.at(IndexInCommandCaseSensitive - 1),
                                                  &TargetAddress))
            {
                //
                // Couldn't resolve or unkonwn parameter
                //
                ShowMessages("err, couldn't resolve error at '%s'\n",
                             SplittedCommandCaseSensitive.at(IndexInCommandCaseSensitive - 1).c_str());
                return;
            }
        }
        else
        {
            //
            // User inserts two address
            //
            ShowMessages("err, incorrect use of '%s' command\n\n",
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
        ShowMessages("err, please enter a valid address\n\n");

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

    //
    // Check to prevent using process id in d* and u* commands
    //
    if (g_IsSerialConnectedToRemoteDebuggee && Pid != 0)
    {
        ShowMessages("err, you cannot specify 'pid' in the debugger mode\n");
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
                                         Length,
                                         NULL);
    }
    else if (!FirstCommand.compare("dc"))
    {
        HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DC,
                                         TargetAddress,
                                         DEBUGGER_READ_VIRTUAL_ADDRESS,
                                         READ_FROM_KERNEL,
                                         Pid,
                                         Length,
                                         NULL);
    }
    else if (!FirstCommand.compare("dd"))
    {
        HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DD,
                                         TargetAddress,
                                         DEBUGGER_READ_VIRTUAL_ADDRESS,
                                         READ_FROM_KERNEL,
                                         Pid,
                                         Length,
                                         NULL);
    }
    else if (!FirstCommand.compare("dq"))
    {
        HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DQ,
                                         TargetAddress,
                                         DEBUGGER_READ_VIRTUAL_ADDRESS,
                                         READ_FROM_KERNEL,
                                         Pid,
                                         Length,
                                         NULL);
    }
    else if (!FirstCommand.compare("!db"))
    {
        HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DB,
                                         TargetAddress,
                                         DEBUGGER_READ_PHYSICAL_ADDRESS,
                                         READ_FROM_KERNEL,
                                         Pid,
                                         Length,
                                         NULL);
    }
    else if (!FirstCommand.compare("!dc"))
    {
        HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DC,
                                         TargetAddress,
                                         DEBUGGER_READ_PHYSICAL_ADDRESS,
                                         READ_FROM_KERNEL,
                                         Pid,
                                         Length,
                                         NULL);
    }
    else if (!FirstCommand.compare("!dd"))
    {
        HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DD,
                                         TargetAddress,
                                         DEBUGGER_READ_PHYSICAL_ADDRESS,
                                         READ_FROM_KERNEL,
                                         Pid,
                                         Length,
                                         NULL);
    }
    else if (!FirstCommand.compare("!dq"))
    {
        HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DQ,
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
    else if (!FirstCommand.compare("u"))
    {
        HyperDbgReadMemoryAndDisassemble(
            DEBUGGER_SHOW_COMMAND_DISASSEMBLE64,
            TargetAddress,
            DEBUGGER_READ_VIRTUAL_ADDRESS,
            READ_FROM_KERNEL,
            Pid,
            Length,
            NULL);
    }
    else if (!FirstCommand.compare("!u"))
    {
        HyperDbgReadMemoryAndDisassemble(
            DEBUGGER_SHOW_COMMAND_DISASSEMBLE64,
            TargetAddress,
            DEBUGGER_READ_PHYSICAL_ADDRESS,
            READ_FROM_KERNEL,
            Pid,
            Length,
            NULL);
    }
    else if (!FirstCommand.compare("u2"))
    {
        HyperDbgReadMemoryAndDisassemble(
            DEBUGGER_SHOW_COMMAND_DISASSEMBLE32,
            TargetAddress,
            DEBUGGER_READ_VIRTUAL_ADDRESS,
            READ_FROM_KERNEL,
            Pid,
            Length,
            NULL);
    }
    else if (!FirstCommand.compare("!u2"))
    {
        HyperDbgReadMemoryAndDisassemble(
            DEBUGGER_SHOW_COMMAND_DISASSEMBLE32,
            TargetAddress,
            DEBUGGER_READ_PHYSICAL_ADDRESS,
            READ_FROM_KERNEL,
            Pid,
            Length,
            NULL);
    }
}
