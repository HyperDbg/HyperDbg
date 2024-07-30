/**
 * @file logopen.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .logopen command
 * @details
 * @version 0.1
 * @date 2020-07-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

using namespace std;

//
// Global Variables
//
extern BOOLEAN  g_LogOpened;
extern ofstream g_LogOpenFile;

/**
 * @brief help of the .logopen command
 *
 * @return VOID
 */
VOID
CommandLogopenHelp()
{
    ShowMessages(".logopen : saves commands and results in a file.\n\n");

    ShowMessages("syntax : \t.logopen [FilePath (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .logopen c:\\users\\sina\\desktop\\log.txt\n");
    ShowMessages("\t\te.g : .logopen \"c:\\users\\sina\\desktop\\log with space.txt\"\n");
}

/**
 * @brief .logopen command handler
 *
 * @param CommandTokens
 *
 * @return VOID
 */
VOID
CommandLogopen(vector<CommandToken> CommandTokens)
{
    if (CommandTokens.size() != 2)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandLogopenHelp();
        return;
    }

    if (g_LogOpened)
    {
        ShowMessages("log was opened previously, you have the close it first "
                     "(using .logclose)\n");
        return;
    }

    //
    // Try to open it as file
    //
    g_LogOpenFile.open(GetCaseSensitiveStringFromCommandToken(CommandTokens.at(1)).c_str());

    //
    // Check if it's okay
    //
    if (g_LogOpenFile.is_open())
    {
        //
        // Start intercepting logs
        //
        g_LogOpened = TRUE;

        //
        // Enable save to the file (Message + time)
        //
        time_t    t  = time(NULL);
        struct tm tm = *localtime(&t);

        ShowMessages("save commands and results into file : %s (%d-%02d-%02d "
                     "%02d:%02d:%02d)\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(1)).c_str(),
                     tm.tm_year + 1900,
                     tm.tm_mon + 1,
                     tm.tm_mday,
                     tm.tm_hour,
                     tm.tm_min,
                     tm.tm_sec);
    }
    else
    {
        ShowMessages("unable to open file : %s\n", GetCaseSensitiveStringFromCommandToken(CommandTokens.at(1)).c_str());
        return;
    }
}

/**
 * @brief Append text to the file object
 *
 * @param Text
 * @return VOID
 */
VOID
LogopenSaveToFile(const char * Text)
{
    g_LogOpenFile << Text;
}
