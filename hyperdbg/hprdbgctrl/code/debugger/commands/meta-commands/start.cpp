/**
 * @file start.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .start command
 * @details
 * @version 0.1
 * @date 2022-01-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\hprdbgctrl\pch.h"

/**
 * @brief help of .start command
 *
 * @return VOID
 */
VOID
CommandStartHelp()
{
    ShowMessages(".start : run a user-mode process.\n\n");
    ShowMessages("syntax : \t.start [path (path string)]\n");
    ShowMessages("\t\te.g : .start path c:\\users\\sina\\reverse eng\\my_file.exe\n");
}

/**
 * @brief .start command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandStart(vector<string> SplittedCommand, string Command)
{
    wstring Filepath;

    if (SplittedCommand.size() <= 2)
    {
        ShowMessages("incorrect use of '.start'\n\n");
        CommandStartHelp();
        return;
    }

    if (!SplittedCommand.at(1).compare("path"))
    {
        //
        // It's a run of target PE file
        //

        //
        // Trim the command
        //
        Trim(Command);

        //
        // Remove .attach from it
        //
        Command.erase(0, 7);

        //
        // Remove path + space
        //
        Command.erase(0, 4 + 1);

        //
        // Trim it again
        //
        Trim(Command);

        //
        // Convert path to wstring
        //
        StringToWString(Filepath, Command);
    }
    else
    {
        ShowMessages("err, couldn't resolve error at '%s'\n\n",
                     SplittedCommand.at(1).c_str());
        CommandStartHelp();
        return;
    }

    //
    // Perform run of the target file
    //
    UsermodeDebuggingAttachToProcess(NULL, NULL, Filepath.c_str());
}
