/**
 * @file lbrdump.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !lbrdump command
 * @details
 * @version 0.19
 * @date 2026-05-03
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of the !lbrdump command
 *
 * @return VOID
 */
VOID
CommandLbrdumpHelp()
{
    ShowMessages("!lbrdump : dumps Last Branch Record (LBR).\n");

    ShowMessages("syntax : \t!lbrdump [core CoreId (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !lbrdump core 1\n");
}

/**
 * @brief !lbrdump command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandLbrdump(vector<CommandToken> CommandTokens, string Command)
{
    if (CommandTokens.size() != 1)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandLbrdumpHelp();
        return;
    }
}
