/**
 * @file logclose.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .logclose command
 * @details
 * @version 0.1
 * @date 2020-07-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN  g_LogOpened;
extern ofstream g_LogOpenFile;

/**
 * @brief help .logclose command
 *
 * @return VOID
 */
VOID
CommandLogcloseHelp()
{
    ShowMessages(".logclose : close the previously opened log.\n\n");
    ShowMessages("syntax : \.logclose\n");
}

/**
 * @brief .logclose command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandLogclose(vector<string> SplittedCommand, string Command)
{
    if (SplittedCommand.size() != 1)
    {
        ShowMessages("incorrect use of '.logclose'\n\n");
        CommandLogcloseHelp();
        return;
    }
    if (!g_LogOpened)
    {
        ShowMessages("there is no opened log, did you use '.logopen'? \n");
        return;
    }

    //
    // Show the time and message before close
    //
    time_t    t  = time(NULL);
    struct tm tm = *localtime(&t);
    ShowMessages("log file closed (%d-%02d-%02d "
                 "%02d:%02d:%02d)\n",
                 tm.tm_year + 1900,
                 tm.tm_mon + 1,
                 tm.tm_mday,
                 tm.tm_hour,
                 tm.tm_min,
                 tm.tm_sec);
    //
    // close the file
    //
    g_LogOpenFile.close();

    //
    // Globally indicate that file is no longer available
    //
    g_LogOpened = FALSE;
}
