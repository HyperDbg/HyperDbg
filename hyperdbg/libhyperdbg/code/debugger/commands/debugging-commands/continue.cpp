/**
 * @file continue.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief continue command
 * @details
 * @version 0.14
 * @date 2025-06-25
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN                  g_BreakPrintingOutput;
extern BOOLEAN                  g_IsConnectedToRemoteDebuggee;
extern BOOLEAN                  g_IsSerialConnectedToRemoteDebuggee;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

/**
 * @brief help of the continue command
 *
 * @return VOID
 */
VOID
CommandContinueHelp()
{
    ShowMessages("continue : continues debuggee or continues processing kernel messages (in test mode, it operates as a user-mode continue command).\n\n");

    ShowMessages("syntax : \tcontinue \n");
}

/**
 * @brief Request to continue
 *
 * @return VOID
 */
VOID
CommandContinueRequest()
{
    CommandGRequest();
}

/**
 * @brief handler of continue command
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandContinue(vector<CommandToken> CommandTokens, string Command)
{
    if (CommandTokens.size() != 1)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandGHelp();
        return;
    }

    CommandContinueRequest();
}
