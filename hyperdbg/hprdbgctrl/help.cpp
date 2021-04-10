/**
 * @file help.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .help command
 * @details
 * @version 0.1
 * @date 2020-08-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of help command :)
 * 
 * @param SplittedCommand
 * @param Command
 * @return VOID 
 */
VOID
CommandHelpHelp()
{
    ShowMessages(".help : Show help and example(s) of a specific command.\n\n");
    ShowMessages("syntax : \t.help [command (string)]\n");
    ShowMessages("\t\te.g : .help !monitor\n");
}

//
// Implementation of .help command is on interpreter.cpp
//
