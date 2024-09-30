/**
 * @file hw_clk.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !hw_clk command
 * @details
 * @version 0.9
 * @date 2024-05-29
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
extern std::vector<UINT32>        g_HwdbgPortConfiguration;
extern const char *               HwdbgActionEnumNames[];

/**
 * @brief help of the !hw_clk command
 *
 * @return VOID
 */
VOID
CommandHwClkHelp()
{
    ShowMessages("!hw_clk : performs actions related to hwdbg hardware debugging events for each clock cycle.\n\n");

    ShowMessages("syntax : \t!hw_clk  [script { Script (string) }]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !hw_clk script { @hw_pin1 = 0; }\n");
}

/**
 * @brief !hw_clk perform test
 *
 * @param CommandTokens
 * @param InstanceFilePathToRead
 * @param InstanceFilePathToSave
 * @param HardwareScriptFilePathToSave
 * @param InitialBramBufferSize
 *
 * @return BOOLEAN
 */
BOOLEAN
CommandHwClkPerfomTest(vector<CommandToken> CommandTokens,
                       const TCHAR *        InstanceFilePathToRead,
                       const TCHAR *        InstanceFilePathToSave,
                       const TCHAR *        HardwareScriptFilePathToSave,
                       UINT32               InitialBramBufferSize)
{
    UINT32                             EventLength;
    DEBUGGER_EVENT_PARSING_ERROR_CAUSE EventParsingErrorCause;
    PDEBUGGER_GENERAL_EVENT_DETAIL     Event                       = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionBreakToDebugger       = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionCustomCode            = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionScript                = NULL;
    UINT32                             ActionBreakToDebuggerLength = 0;
    UINT32                             ActionCustomCodeLength      = 0;
    UINT32                             ActionScriptLength          = 0;
    CHAR *                             ScriptBuffer                = NULL;
    BOOLEAN                            Result                      = FALSE;

    //
    // Load the instance info
    //
    if (!HwdbgLoadInstanceInfo(InstanceFilePathToRead, InitialBramBufferSize))
    {
        //
        // No need for freeing memory so reuturn directly
        //
        return FALSE;
    }

    //
    // Interpret and fill the general event and action fields for the target script
    //
    if (!InterpretGeneralEventAndActionsFields(
            &CommandTokens,
            (VMM_EVENT_TYPE_ENUM)NULL, // not an event
            &Event,
            &EventLength,
            &ActionBreakToDebugger,
            &ActionBreakToDebuggerLength,
            &ActionCustomCode,
            &ActionCustomCodeLength,
            &ActionScript,
            &ActionScriptLength,
            &EventParsingErrorCause))
    {
        //
        // No need for freeing memory so reuturn directly
        //
        return FALSE;
    }

    //
    // Print the actual script
    //
    ScriptBuffer = (CHAR *)((UINT64)ActionScript + sizeof(DEBUGGER_GENERAL_ACTION));
    HwdbgScriptPrintScriptBuffer(ScriptBuffer, ActionScript->ScriptBufferSize);

    //
    // Create hwdbg script
    //
    if (!HwdbgScriptCreateHwdbgScript(ScriptBuffer,
                                      ActionScript->ScriptBufferSize,
                                      HardwareScriptFilePathToSave))
    {
        ShowMessages("err, unable to create hwdbg script\n");
        Result = FALSE;
        goto FreeAndReturnResult;
    }

    //
    // Write an additional updated test instance info request into a file
    //
    if (!HwdbgWriteTestInstanceInfoRequestIntoFile(&g_HwdbgInstanceInfo,
                                                   InstanceFilePathToSave))
    {
        ShowMessages("err, unable to write instance info request\n");
        Result = FALSE;
        goto FreeAndReturnResult;
    }

    //
    // The test is performed successfully
    //
    Result = TRUE;

FreeAndReturnResult:

    //
    // Free the allocated memory
    //
    FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);

    //
    // Return the result
    //
    return Result;
}

/**
 * @brief !hw_clk command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandHwClk(vector<CommandToken> CommandTokens, string Command)
{
    if (CommandTokens.size() >= 2 && CompareLowerCaseStrings(CommandTokens.at(1), "test"))
    {
        //
        // Perform test with default file path and initial BRAM buffer size
        //
        CommandHwClkPerfomTest(CommandTokens,
                               HWDBG_TEST_READ_INSTANCE_INFO_PATH,
                               HWDBG_TEST_WRITE_INSTANCE_INFO_PATH,
                               HWDBG_TEST_WRITE_SCRIPT_BUFFER_PATH,
                               DEFAULT_INITIAL_BRAM_BUFFER_SIZE);
    }
    else
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandHwClkHelp();
        return;
    }
}
