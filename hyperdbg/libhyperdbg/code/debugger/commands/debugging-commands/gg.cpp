/**
 * @file gg.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief gg command
 * @details
 * @version 0.11
 * @date 2024-11-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of the g command
 *
 * @return VOID
 */
VOID
CommandGgHelp()
{
    ShowMessages("gg : shows and changes the operating processor.\n\n");

    ShowMessages("syntax : \tgg\n");
    ShowMessages("syntax : \tgg [wp]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : gg \n");
    ShowMessages("\t\te.g : gg wp \n");
}

/**
 * @brief gg command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandGg(vector<CommandToken> CommandTokens, string Command)
{
    if (CommandTokens.size() == 1)
    {
        ShowMessages("Good Game! :)\n");
    }
    else if (CommandTokens.size() == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "wp"))
    {
        ShowMessages("Good Game, Well Played! :)\n");
    }
    else
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandGgHelp();
        return;
    }
}
