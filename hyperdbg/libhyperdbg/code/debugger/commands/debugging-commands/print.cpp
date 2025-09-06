/**
 * @file print.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 * @brief print command
 * @details
 * @version 0.1
 * @date 2020-10-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

using namespace std;

/**
 * @brief help of the print command
 *
 * @return VOID
 */
VOID
CommandPrintHelp()
{
    ShowMessages("print : evaluates expressions.\n\n");

    ShowMessages("syntax : \tprint [Expression (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : print dq(poi(@rcx))\n");
}

/**
 * @brief handler of print command
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandPrint(vector<CommandToken> CommandTokens, string Command)
{
    if (CommandTokens.size() == 1)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandPrintHelp();
        return;
    }

    //
    // Trim the command
    //
    Trim(Command);

    //
    // Remove print from it
    //
    Command.erase(0, GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).size());

    //
    // Trim it again
    //
    Trim(Command);

    //
    // Prepend and append 'print(' and ')'
    //
    Command.insert(0, "print(");
    Command.append(");");

    //
    // Execute the expression
    //
    ScriptEngineExecuteSingleExpression((CHAR *)Command.c_str(), TRUE, FALSE);
}
