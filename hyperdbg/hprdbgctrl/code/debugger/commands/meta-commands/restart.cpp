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

//
// Global Variables
//
extern std::wstring g_StartCommandPath;
extern std::wstring g_StartCommandPathAndArguments;

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
    if (SplittedCommand.size() != 1)
    {
        ShowMessages("incorrect use of '.restart'\n\n");
        CommandRestartHelp();
        return;
    }

    //
    // Check if the .start command is previously called or not
    //
    if (g_StartCommandPath.empty())
    {
        ShowMessages("nothing to restart, did you use the '.start' command before?\n");
        return;
    }

    //
    // Perform run of the target file
    //
    if (g_StartCommandPathAndArguments.empty())
    {
        UsermodeDebuggingAttachToProcess(NULL,
                                         NULL,
                                         g_StartCommandPath.c_str(),
                                         NULL);
    }
    else
    {
        UsermodeDebuggingAttachToProcess(NULL,
                                         NULL,
                                         g_StartCommandPath.c_str(),
                                         (WCHAR *)g_StartCommandPathAndArguments.c_str());
    }
}
