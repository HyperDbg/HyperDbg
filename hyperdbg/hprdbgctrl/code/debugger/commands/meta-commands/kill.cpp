/**
 * @file kill.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .kill command
 * @details
 * @version 0.1
 * @date 2022-01-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\hprdbgctrl\pch.h"

/**
 * @brief help of .kill command
 *
 * @return VOID
 */
VOID
CommandKillHelp()
{
    ShowMessages(".kill : terminate the current running process or a special "
                 "process.\n\n");
    ShowMessages(
        "syntax : \t.kill [pid (hex)]\n");
    ShowMessages("\t\te.g : .kill \n");
    ShowMessages("\t\te.g : .kill pid b60 \n");
}

/**
 * @brief .kill command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandKill(vector<string> SplittedCommand, string Command)
{
}
