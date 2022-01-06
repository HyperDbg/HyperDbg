/**
 * @file restart.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .restart command
 * @details
 * @version 0.1
 * @date 2022-01-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\hprdbgctrl\pch.h"

/**
 * @brief help of .restart command
 *
 * @return VOID
 */
VOID
CommandRestartHelp()
{
    ShowMessages(".restart : restart the previously started process "
                 "(using '.start' command).\n\n");
    ShowMessages(
        "syntax : \t.restart\n");
}

/**
 * @brief .restart command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandRestart(vector<string> SplittedCommand, string Command)
{
}
