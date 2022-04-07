/**
 * @file dt.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief dt command
 * @details
 * @version 0.1
 * @date 2021-12-13
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\hprdbgctrl\pch.h"

/**
 * @brief help of dt command
 *
 * @return VOID
 */
VOID
CommandDtHelp()
{
    ShowMessages(".formats : Show a value or register in different formats.\n\n");
    ShowMessages("syntax : \t.formats [Expression (string)]\n");

    ShowMessages("\t\te.g : .formats nt!ExAllocatePoolWithTag\n");
    ShowMessages("\t\te.g : .formats nt!Kd_DEFAULT_Mask\n");
    ShowMessages("\t\te.g : .formats nt!Kd_DEFAULT_Mask+5\n");
    ShowMessages("\t\te.g : .formats 55\n");
    ShowMessages("\t\te.g : .formats @rax\n");
    ShowMessages("\t\te.g : .formats @rbx+@rcx\n");
    ShowMessages("\t\te.g : .formats $pid\n");
}

/**
 * @brief dt command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandDt(vector<string> SplittedCommand, string Command)
{
    if (SplittedCommand.size() == 1)
    {
        ShowMessages("incorrect use of 'dt'\n\n");
        CommandDtHelp();
        return;
    }

    //
    // Trim the command
    //
    Trim(Command);

    //
    // Remove dt from it
    //
    Command.erase(0, 2);

    //
    // Trim it again
    //
    Trim(Command);

    //
    // Check for the first and second arguments
    //
    vector<string> TempSplittedCommand {Split(Command, ' ')};

    //
    // If the size is zero, then it's only a type name
    //
    if (TempSplittedCommand.size() == 1)
    {
        ScriptEngineShowDataBasedOnSymbolTypesWrapper(TempSplittedCommand.at(0).c_str(), NULL, NULL, NULL);
    }
    else
    {
        //
        // When we're here, it means we have size() >= 2, so we have to check
        // the first and the second method to see which one is the type and
        // which one is the symbol
        //
    }
}
