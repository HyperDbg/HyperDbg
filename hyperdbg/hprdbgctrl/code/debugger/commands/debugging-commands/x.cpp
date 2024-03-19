/**
 * @file x.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief parse symbols
 * @details
 * @version 0.1
 * @date 2021-05-31
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of the x command
 *
 * @return VOID
 */
VOID
CommandXHelp()
{
    ShowMessages("x : searches and shows symbols (wildcard) and corresponding addresses.\n\n");

    ShowMessages("syntax : \tx [Module!Name (wildcard string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : x nt!ExAllocatePoolWithTag \n");
    ShowMessages("\t\te.g : x nt!ExAllocatePool* \n");
    ShowMessages("\t\te.g : x nt!* \n");
}

/**
 * @brief x command handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandX(vector<string> SplitCommand, string Command)
{
    if (SplitCommand.size() == 1)
    {
        ShowMessages("incorrect use of the 'x'\n\n");
        CommandXHelp();
        return;
    }

    //
    // Trim the command
    //
    Trim(Command);

    //
    // Remove x from it
    //
    Command.erase(0, SplitCommand.at(0).size());

    //
    // Trim it again
    //
    Trim(Command);

    //
    // Search for mask
    //
    ScriptEngineSearchSymbolForMaskWrapper(Command.c_str());
}
