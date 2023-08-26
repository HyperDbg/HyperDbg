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
    ShowMessages(".dump : saves memory context into a file.\n\n");

    ShowMessages("syntax : \t.dump [FromAddress (hex)] [ToAddress (hex)] [path Path (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .dump 401000 40b000 path c:\\users\\sina\\desktop\\dump1.dmp\n");
    ShowMessages("\t\te.g : .dump fffff801deadb000 fffff801deade054 path c:\\users\\sina\\desktop\\dump2.dmp\n");
    ShowMessages("\t\te.g : .dump fffff801deadb000 fffff801deade054 path c:\\users\\sina\\desktop\\dump3.dmp\n");
    ShowMessages("\t\te.g : .dump 00007ff8349f2000 00007ff8349f8000 path c:\\users\\sina\\desktop\\dump4.dmp\n");
    ShowMessages("\t\te.g : .dump rax+rcx rax+rcx+1000 path c:\\users\\sina\\desktop\\dump5.dmp\n");
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
    wstring     Filepath;
    UINT64      StartAddress = 0;
    UINT64      EndAddress   = 0;
    std::string Delimiter    = "path";

    if (SplittedCommand.size() <= 4)
    {
        ShowMessages("err, incorrect use of the '.dump' command\n\n");
        CommandDumpHelp();
        return;
    }

    //
    // Check the 'From' address
    //
    if (!SymbolConvertNameOrExprToAddress(
            SplittedCommand.at(1),
            &StartAddress))
    {
        //
        // couldn't resolve or unkonwn parameter
        //
        ShowMessages("err, couldn't resolve error at '%s'\n\n",
                     SplittedCommand.at(1).c_str());

        CommandDumpHelp();
        return;
    }

    //
    // Check the 'To' address
    //
    if (!SymbolConvertNameOrExprToAddress(
            SplittedCommand.at(2),
            &EndAddress))
    {
        //
        // couldn't resolve or unkonwn parameter
        //
        ShowMessages("err, couldn't resolve error at '%s'\n\n",
                     SplittedCommand.at(2).c_str());

        CommandDumpHelp();
        return;
    }

    //
    // Check the 'path' file names
    //
    if (SplittedCommand.at(3).compare("path"))
    {
        //
        // couldn't resolve or unkonwn parameter
        //
        ShowMessages("err, couldn't resolve error at '%s'\n\n",
                     SplittedCommand.at(3).c_str());

        CommandDumpHelp();
        return;
    }

    size_t DelimiterPos = Command.find(SplittedCommand.at(3));

    if (DelimiterPos != std::string::npos)
    {
        string FirstPart  = Command.substr(0, DelimiterPos);
        string SecondPart = Command.substr(DelimiterPos + Delimiter.length());

        //
        // Trim the command
        //
        Trim(SecondPart);

        //
        // Convert path to wstring
        //
        StringToWString(Filepath, SecondPart);

        ShowMessages("The path is : %ws\n", Filepath);
        //
        // Check whether addesss are valid or not , and check whether the 'to' is greater than 'from'
        //
    }
    else
    {
        ShowMessages("err, incorrect use of the '.dump' command\n\n");
        CommandDumpHelp();
        return;
    }
}
