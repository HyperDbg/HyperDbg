/**
 * @file pe.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .pe command
 * @details
 * @version 0.1
 * @date 2021-12-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

using namespace std;

/**
 * @brief help of the .pe command
 *
 * @return VOID
 */
VOID
CommandPeHelp()
{
    ShowMessages(".pe : parses portable executable (PE) files and dump sections.\n\n");

    ShowMessages("syntax : \t.pe [header] [FilePath (string)]\n");
    ShowMessages("syntax : \t.pe [section] [SectionName (string)] [FilePath (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .pe header c:\\reverse files\\myfile.exe\n");
    ShowMessages("\t\te.g : .pe section .text c:\\reverse files\\myfile.exe\n");
    ShowMessages("\t\te.g : .pe section .rdata c:\\reverse files\\myfile.exe\n");
}

/**
 * @brief .pe command handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandPe(vector<string> SplitCommand, string Command)
{
    BOOLEAN Is32Bit = FALSE;
    wstring Filepath;
    BOOLEAN ShowDumpOfSection = FALSE;

    if (SplitCommand.size() <= 2)
    {
        ShowMessages("err, incorrect use of the '.pe' command\n\n");
        CommandPeHelp();
        return;
    }

    //
    // Check for first option
    //
    if (!SplitCommand.at(1).compare("section"))
    {
        if (SplitCommand.size() == 3)
        {
            ShowMessages("please specify a valid PE file\n\n");
            CommandPeHelp();
            return;
        }
        ShowDumpOfSection = TRUE;
    }
    else if (!SplitCommand.at(1).compare("header"))
    {
        ShowDumpOfSection = FALSE;
    }
    else
    {
        //
        // Couldn't resolve or unknown parameter
        //
        ShowMessages("err, couldn't resolve error at '%s'\n\n",
                     SplitCommand.at(1).c_str());
        CommandPeHelp();
        return;
    }

    //
    // Trim the command
    //
    Trim(Command);

    //
    // Remove .pe from it
    //
    Command.erase(0, SplitCommand.at(0).size());

    if (!ShowDumpOfSection)
    {
        //
        // Remove header + space
        //
        Command.erase(0, 6 + 1);
    }
    else
    {
        //
        // Remove section + space
        //
        Command.erase(0, 7 + 1);

        //
        // Remove the string param for section + space
        //
        Command.erase(0, SplitCommand.at(2).size() + 1);
    }

    //
    // Trim it again
    //
    Trim(Command);

    //
    // Convert path to wstring
    //
    StringToWString(Filepath, Command);

    //
    // Detect whether PE is 32-bit or 64-bit
    //
    if (!PeIsPE32BitOr64Bit(Filepath.c_str(), &Is32Bit))
    {
        //
        // File was invalid, the error message is shown in the above function
        //
        return;
    }

    //
    // Parse PE file
    //
    if (!ShowDumpOfSection)
    {
        PeShowSectionInformationAndDump(Filepath.c_str(), NULL, Is32Bit);
    }
    else
    {
        PeShowSectionInformationAndDump(Filepath.c_str(), SplitCommand.at(2).c_str(), Is32Bit);
    }
}
