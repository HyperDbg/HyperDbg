/**
 * @file cls.c
 * @author Sina Karvandi (sina@rayanfam.com)
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
 * @brief help of .cls command
 *
 * @return VOID
 */
VOID
CommandClearScreenHelp()
{
    ShowMessages(".cls : clears the screen.\n\n");
    ShowMessages("syntax : \t.cls\n");
}

/**
 * @brief .cls command handler
 * 
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandClearScreen(vector<string> SplittedCommand, string Command)
{
    system("cls");
}
