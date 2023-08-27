/**
 * @file dump.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .dump command implementation
 * @details
 * @version 0.6
 * @date 2023-08-26
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
 * @brief help of the .dump command
 *
 * @return VOID
 */
VOID
CommandDumpHelp()
{
    ShowMessages(".dump & !dump : saves memory context into a file.\n\n");

    ShowMessages("syntax : \t.dump [FromAddress (hex)] [ToAddress (hex)] [pid ProcessId (hex)] [path Path (string)]\n");
    ShowMessages("\nIf you want to dump physical memory then add '!' at the "
                 "start of the command\n\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .dump 401000 40b000 path c:\\users\\sina\\desktop\\dump1.dmp\n");
    ShowMessages("\t\te.g : .dump 401000 40b000 pid 1c0 path c:\\users\\sina\\desktop\\dump2.dmp\n");
    ShowMessages("\t\te.g : .dump fffff801deadb000 fffff801deade054 path c:\\users\\sina\\desktop\\dump3.dmp\n");
    ShowMessages("\t\te.g : .dump fffff801deadb000 fffff801deade054 path c:\\users\\sina\\desktop\\dump4.dmp\n");
    ShowMessages("\t\te.g : .dump 00007ff8349f2000 00007ff8349f8000 path c:\\users\\sina\\desktop\\dump5.dmp\n");
    ShowMessages("\t\te.g : .dump @rax+@rcx @rax+@rcx+1000 path c:\\users\\sina\\desktop\\dump6.dmp\n");
    ShowMessages("\t\te.g : !dump 1000 2100 path c:\\users\\sina\\desktop\\dump7.dmp\n");
}

/**
 * @brief .dump command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandDump(vector<string> SplittedCommand, string Command)
{
    wstring                   Filepath;
    UINT64                    StartAddress        = 0;
    UINT64                    EndAddress          = 0;
    UINT32                    Pid                 = 0;
    string                    Delimiter           = "path";
    BOOLEAN                   IsFirstCommand      = TRUE;
    BOOLEAN                   NextIsProcId        = FALSE;
    BOOLEAN                   NextIsPath          = FALSE;
    BOOLEAN                   IsDumpPathSpecified = FALSE;
    string                    FirstCommand        = SplittedCommand.front();
    DEBUGGER_READ_MEMORY_TYPE MemoryType          = DEBUGGER_READ_VIRTUAL_ADDRESS;

    if (SplittedCommand.size() <= 4)
    {
        ShowMessages("err, incorrect use of the '.dump' command\n\n");
        CommandDumpHelp();
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

    for (auto Section : SplittedCommand)
    {
        if (IsFirstCommand == TRUE)
        {
            IsFirstCommand = FALSE;
            continue;
        }
        else if (NextIsProcId)
        {
            if (!ConvertStringToUInt32(Section, &Pid))
            {
                ShowMessages("please specify a correct hex value for process id\n\n");
                CommandDumpHelp();
                return;
            }
            NextIsProcId = FALSE;
            continue;
        }
        else if (NextIsPath)
        {
            //
            // Convert path to wstring
            //
            StringToWString(Filepath, Section);
            IsDumpPathSpecified = TRUE;

            NextIsPath = FALSE;
        }
        else if (!Section.compare("pid"))
        {
            NextIsProcId = TRUE;
            continue;
        }
        else if (!Section.compare("path"))
        {
            NextIsPath = TRUE;
            continue;
        }
        //
        // Check the 'From' address
        //
        else if (!SymbolConvertNameOrExprToAddress(
                     Section,
                     &StartAddress))
        {
            //
            // couldn't resolve or unkonwn parameter
            //
            ShowMessages("err, couldn't resolve error at '%s'\n\n",
                         Section.c_str());

            CommandDumpHelp();
            return;
        }

        //
        // Check the 'To' address
        //
        else if (!SymbolConvertNameOrExprToAddress(
                     Section,
                     &EndAddress))
        {
            //
            // couldn't resolve or unkonwn parameter
            //
            ShowMessages("err, couldn't resolve error at '%s'\n\n",
                         Section.c_str());

            CommandDumpHelp();
            return;
        }
        else
        {
            //
            // invalid input
            //
            ShowMessages("err, incorrect use of the '%s' command\n\n",
                         Section.c_str());
            CommandDumpHelp();

            return;
        }
    }

    //
    // Check if 'pid' is not specified
    //
    if (NextIsProcId)
    {
        ShowMessages("please specify a correct hex value for process id\n\n");
        CommandDumpHelp();
        return;
    }

    //
    // Check if 'path' is either specified, not completely specified
    //
    if (NextIsPath || IsDumpPathSpecified)
    {
        ShowMessages("please specify a correct path for saving the dump\n\n");
        CommandDumpHelp();
        return;
    }

    //
    // Check if start address or end address is null
    //
    if (StartAddress == NULL || EndAddress == NULL)
    {
        ShowMessages("err, please specify the start and end address in hex format\n");
        return;
    }

    //
    // Check if end address is bigger than start address
    //
    if (StartAddress >= EndAddress)
    {
        ShowMessages("err, please note that the 'to' address should be greater than the 'from' address\n");
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
        Pid = GetCurrentProcessId();
    }

    //
    // Check whether it's physical or virtual address
    //
    if (!FirstCommand.compare("!dump"))
    {
        MemoryType = DEBUGGER_READ_PHYSICAL_ADDRESS;
    }

    ShowMessages("the dump path is : %ls\n", Filepath.c_str());

    /*
    HyperDbgReadMemoryAndDisassemble(
        DEBUGGER_SHOW_COMMAND_DUMP,
        StartAddress,
        MemoryType,
        READ_FROM_KERNEL,
        Pid,
        Length,
        NULL);
        */
}
