/**
 * @file sympath.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .sympath command
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
 * @brief help of .sympath command
 *
 * @return VOID
 */
VOID
CommandSympathHelp()
{
    ShowMessages(".sympath : set the symbol server and path.\n\n");

    ShowMessages("syntax : \t.sympath [server, path]\n");
    ShowMessages("\t\te.g : .sympath\n");
    ShowMessages("\t\te.g : .sympath SRV*c:\\Symbols*https://msdl.microsoft.com/download/symbols \n");
}

/**
 * @brief .sympath command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandSympath(vector<string> SplittedCommand, string Command)
{
}
