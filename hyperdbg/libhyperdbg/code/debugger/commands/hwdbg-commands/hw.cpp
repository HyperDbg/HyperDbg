/**
 * @file hw.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !hw command
 * @details
 * @version 0.11
 * @date 2024-09-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern HWDBG_INSTANCE_INFORMATION g_HwdbgInstanceInfo;
extern BOOLEAN                    g_HwdbgInstanceInfoIsValid;

/**
 * @brief help of the !hw command
 *
 * @return VOID
 */
VOID
CommandHwHelp()
{
    ShowMessages("!hw : runs a hardware script in the target device.\n\n");

    ShowMessages("syntax : \t!hw script [script { Script (string) }]\n");
    ShowMessages("syntax : \t!hw script [unload]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !hw script { @hw_pin1 = 0; }\n");
    ShowMessages("\t\te.g : !hw unload\n");
}

/**
 * @brief !hw command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandHw(vector<CommandToken> CommandTokens, string Command)
{
    if (CommandTokens.size() >= 2 && CompareLowerCaseStrings(CommandTokens.at(1), "script"))
    {
        //
        // Perform test with default file path and initial BRAM buffer size
        //
        HwdbgScriptRunScript(GetCaseSensitiveStringFromCommandToken(CommandTokens.at(2)).c_str(),
                             HWDBG_TEST_READ_INSTANCE_INFO_PATH,
                             HWDBG_TEST_WRITE_SCRIPT_BUFFER_PATH,
                             DEFAULT_INITIAL_BRAM_BUFFER_SIZE);
    }
    else if (CommandTokens.size() >= 2 &&
             (CompareLowerCaseStrings(CommandTokens.at(1), "eval") || CompareLowerCaseStrings(CommandTokens.at(1), "evaluation")))
    {
        //
        // Perform test evaluation
        //
        ScriptEngineWrapperTestParserForHwdbg(GetCaseSensitiveStringFromCommandToken(CommandTokens.at(2)));
    }
    else if (CommandTokens.size() == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "unload"))
    {
        //
        // Unload the script
        //
        g_HwdbgInstanceInfoIsValid = FALSE;
    }
    else
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandHwHelp();
        return;
    }
}
