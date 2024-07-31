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
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandX(vector<CommandToken> CommandTokens, string Command)
{
    if (CommandTokens.size() != 2)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandXHelp();
        return;
    }

    //
    // Search for mask
    //
    ScriptEngineSearchSymbolForMaskWrapper(GetCaseSensitiveStringFromCommandToken(CommandTokens.at(1)).c_str());
}
