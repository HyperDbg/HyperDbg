/**
 * @file cls.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .cls, cls, clear commands implementations
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of the .cls command
 *
 * @return VOID
 */
VOID
CommandClsHelp()
{
    ShowMessages(".cls : clears the screen.\n\n");

    ShowMessages("syntax : \t.cls\n");
}

/**
 * @brief .cls command handler
 *
 * @param CommandTokens
 *
 * @return VOID
 */
VOID
CommandCls(vector<CommandToken> CommandTokens)
{
    if (CommandTokens.size() != 1)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandClsHelp();
        return;
    }

    system("cls");
}
