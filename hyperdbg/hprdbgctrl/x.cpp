/**
 * @file x.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief parse symbols
 * @details
 * @version 0.1
 * @date 2021-05-31
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
BOOLEAN g_TestIsModulesLoaded = FALSE;

/**
 * @brief help of x command
 *
 * @return VOID
 */
VOID
CommandXHelp()
{
    ShowMessages("x : search and show symbols (wildcard) and corresponding addresses.\n\n");
    ShowMessages("syntax : \tx [module!name (string)]\n");
    ShowMessages("\t\te.g : x nt!ExAllocatePoolWithTag \n");
    ShowMessages("\t\te.g : x nt!ExAllocatePool* \n");
    ShowMessages("\t\te.g : x nt!* \n");
}

/**
 * @brief x command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandX(vector<string> SplittedCommand, string Command)
{
    if (SplittedCommand.size() == 1)
    {
        ShowMessages("incorrect use of 'x'\n\n");
        CommandXHelp();
        return;
    }

    //
    // Trim the command
    //
    Trim(Command);

    //
    // Remove x from it
    //
    Command.erase(0, 1);

    //
    // Trim it again
    //
    Trim(Command);

    //
    // Test should be removed
    //
    if (!g_TestIsModulesLoaded)
    {
        //SymbolLoadNtoskrnlSymbol(DebuggerGetNtoskrnlBase());
        ScriptEngineLoadFileSymbolWrapper(DebuggerGetNtoskrnlBase(), "C:\\symbols\\ntkrnlmp.pdb\\D572501B90246C4BE1992A7842D966E41\\ntkrnlmp.pdb");
        //ScriptEngineLoadFileSymbolWrapper(DebuggerGetNtoskrnlBase() + 0x1000000, "C:\\symbols\\win32k.pdb\\ED706A38659240A066E6FB19B994BAAA1\\win32k.pdb");
        g_TestIsModulesLoaded = TRUE;
    }

    //
    // Search for mask
    //
    ScriptEngineSearchSymbolForMaskWrapper(Command.c_str());

    //
    // Test should be removed
    //
    // ScriptEngineUnloadAllSymbolsWrapper();
}
