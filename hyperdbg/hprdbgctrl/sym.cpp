/**
 * @file sym.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .sym command
 * @details
 * @version 0.1
 * @date 2021-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//

/**
 * @brief help of .sym command
 *
 * @return VOID
 */
VOID
CommandSymHelp()
{
    ShowMessages(".sym : perfrom the symbol actions.\n\n");

    ShowMessages("syntax : \t.sym [download | ]\n");
    ShowMessages("\t\te.g : .sym \n");
}

/**
 * @brief .sym command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandSym(vector<string> SplittedCommand, string Command)
{
}
