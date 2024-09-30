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
 * @brief Read the instance info from the file
 * @param FileName
 * @param MemoryBuffer
 * @param BufferSize
 *
 * @return BOOLEAN
 */
BOOLEAN
CommandHwClkReadInstanceInfoFromFile(const TCHAR * FileName, UINT32 * MemoryBuffer, size_t BufferSize)
{
    TCHAR TestFilePath[MAX_PATH] = {0};

    if (SetupPathForFileName(HWDBG_TEST_READ_INSTANCE_INFO_PATH, TestFilePath, sizeof(TestFilePath), TRUE) &&
        HwdbgInterpreterFillMemoryFromFile(TestFilePath, MemoryBuffer, BufferSize))
    {
        //
        // Print the content of MemoryBuffer for verification
        //
        for (SIZE_T I = 0; I < BufferSize; ++I)
        {
            ShowMessages("%08x ", MemoryBuffer[I]);
            ShowMessages("\n");
        }

        //
        // the instance info packet is read successfully
        //
        return TRUE;
    }

    //
    // the instance info packet is not read successfully
    //
    return FALSE;
}

/**
 * @brief Print the actual script
 *
 * @param ScriptBuffer
 * @param ScriptBufferSize
 *
 * @return VOID
 */
VOID
CommandHwClkPrintScriptBuffer(CHAR * ScriptBuffer, UINT32 ScriptBufferSize)
{
    //
    // Print the actual script
    //
    ShowMessages("\nHyperDbg (general) script buffer (size=%d, flip-flops (just script)=%d):\n\n",
                 ScriptBufferSize,
                 ScriptBufferSize * 8 // Converted to bits
    );

    for (size_t i = 0; i < ScriptBufferSize; i++)
    {
        ShowMessages("%02X ", (UINT8)ScriptBuffer[i]);
    }

    ShowMessages("\n");
}

/**
 * @brief Compress the script buffer
 *
 * @param InstanceInfo
 * @param ScriptBuffer
 * @param ScriptBufferSize
 * @param NumberOfStagesForScript
 * @param NewScriptBuffer
 * @param NewCompressedBufferSize
 * @param NumberOfBytesPerChunk
 *
 * @return BOOLEAN
 */
BOOLEAN
CommandHwClkCompressScriptBuffer(HWDBG_INSTANCE_INFORMATION * InstanceInfo,
                                 SYMBOL *                     ScriptBuffer,
                                 size_t                       ScriptBufferSize,
                                 UINT32                       NumberOfStagesForScript,
                                 HWDBG_SHORT_SYMBOL **        NewScriptBuffer,
                                 size_t *                     NewCompressedBufferSize,
                                 size_t *                     NumberOfBytesPerChunk)
{
    //
    // Now, converting the script based on supported script variable length
    //
    if (!g_HwdbgInstanceInfoIsValid)
    {
        //
        // The instance info is not valid
        //
        ShowMessages("err, the instance info is not valid\n");

        return FALSE;
    }

    //
    // Check if the variable length is valid
    //
    if (!(InstanceInfo->scriptVariableLength >= sizeof(BYTE) * 8))
    {
        //
        // The script variable length is not valid (at least 8 bit (1 byte)
        //
        ShowMessages("err, the script variable length should be at least 8 bits (1 byte)\n");
        return FALSE;
    }

    //
    // *** The script variable length is valid (at least 8 bit (1 byte) ***
    //

    //
    // Compress script buffer
    //
    if (HardwareScriptInterpreterConvertSymbolToHwdbgShortSymbolBuffer(InstanceInfo,
                                                                       ScriptBuffer,
                                                                       ScriptBufferSize,
                                                                       NumberOfStagesForScript,
                                                                       NewScriptBuffer,
                                                                       NewCompressedBufferSize) == FALSE)
    {
        ShowMessages("err, unable to convert the script buffer to short symbol buffer\n");
        return FALSE;
    }

    //
    // we put BRAM data width size here instead of script variable length (InstanceInfo.scriptVariableLength)
    // since we want it to read one symbol filed at a time
    //
    if (!HardwareScriptInterpreterCompressBuffer((UINT64 *)*NewScriptBuffer,
                                                 *NewCompressedBufferSize,
                                                 InstanceInfo->scriptVariableLength,
                                                 InstanceInfo->bramDataWidth,
                                                 NewCompressedBufferSize,
                                                 NumberOfBytesPerChunk))
    {
        //
        // Unable to compress the buffer
        //
        return FALSE;
    }

    //
    // The script buffer is compressed successfully
    //
    return TRUE;
}

/**
 * @brief Print the hwdbg script buffer
 *
 * @param InstanceInfo
 * @param NewCompressedBufferSize
 * @param NumberOfStagesForScript
 * @param NumberOfOperandsForScript
 * @param NewScriptBuffer
 * @param NumberOfNeededFlipFlopsInTargetDevice
 * @param NumberOfBytesPerChunk
 * @param NumberOfOperandsImplemented
 *
 * @return VOID
 */
VOID
CommandHwClkPrintHwdbgScriptBuffer(HWDBG_INSTANCE_INFORMATION * InstanceInfo,
                                   size_t                       NewCompressedBufferSize,
                                   UINT32                       NumberOfStagesForScript,
                                   UINT32                       NumberOfOperandsForScript,
                                   HWDBG_SHORT_SYMBOL *         NewScriptBuffer,
                                   size_t                       NumberOfNeededFlipFlopsInTargetDevice,
                                   size_t                       NumberOfBytesPerChunk,
                                   UINT32                       NumberOfOperandsImplemented)
{
    ShowMessages("\n---------------------------------------------------------\n");

    NumberOfNeededFlipFlopsInTargetDevice = HwdbgComputeNumberOfFlipFlopsNeeded(InstanceInfo, NumberOfStagesForScript);

    ShowMessages("hwdbg script buffer (buffer size=%d, stages=%d, operands needed: %d - operands used: %d (%.2f%%), total used flip-flops=%d, number of bytes per chunk: %d):\n\n",
                 NewCompressedBufferSize,
                 NumberOfStagesForScript,
                 NumberOfOperandsImplemented,
                 NumberOfOperandsForScript,
                 ((float)NumberOfOperandsForScript / (float)NumberOfOperandsImplemented) * 100,
                 NumberOfNeededFlipFlopsInTargetDevice,
                 NumberOfBytesPerChunk);

    for (size_t i = 0; i < NewCompressedBufferSize; i++)
    {
        ShowMessages("%02X ", (UINT8)((CHAR *)NewScriptBuffer)[i]);
    }
}

/**
 * @brief Write script configuration packet into a file
 *
 * @param InstanceInfo
 * @param FileName
 * @param NumberOfOperandsImplemented
 * @param NumberOfStagesForScript
 * @param NewScriptBuffer
 * @param NewCompressedBufferSize
 *
 * @return BOOLEAN
 */
BOOLEAN
CommandHwClkWriteScriptConfigurationPacketIntoFile(HWDBG_INSTANCE_INFORMATION * InstanceInfo,
                                                   const CHAR *                 FileName,
                                                   UINT32                       NumberOfStagesForScript,
                                                   UINT32                       NumberOfOperandsImplemented,
                                                   HWDBG_SHORT_SYMBOL *         NewScriptBuffer,
                                                   size_t                       NewCompressedBufferSize)
{
    TCHAR TestFilePath[MAX_PATH] = {0};

    //
    // *** Write script configuration packet into a file ***
    //

    ShowMessages("\n\nwriting script configuration packet into the file\n");

    if (SetupPathForFileName(FileName, TestFilePath, sizeof(TestFilePath), FALSE) &&
        HwdbgInterpreterSendScriptPacket(
            InstanceInfo,
            TestFilePath,
            NumberOfStagesForScript + NumberOfOperandsImplemented - 1, // Number of symbols = Number of stages + Number of operands - 1
            NewScriptBuffer,
            (UINT32)NewCompressedBufferSize))
    {
        ShowMessages("\n[*] script buffer successfully written into file: %s\n", TestFilePath);
        return TRUE;
    }
    else
    {
        ShowMessages("err, unable to write script buffer\n");
        return FALSE;
    }
}

/**
 * @brief Write test instance info request into a file
 *
 * @param InstanceInfo
 * @param FileName
 *
 * @return BOOLEAN
 */
BOOLEAN
CommandHwClkWriteTestInstanceInfoRequestIntoFile(HWDBG_INSTANCE_INFORMATION * InstanceInfo,
                                                 const CHAR *                 FileName)
{
    TCHAR TestFilePath[MAX_PATH] = {0};

    //
    // Write test instance info request into a file
    //
    if (SetupPathForFileName(FileName, TestFilePath, sizeof(TestFilePath), FALSE) &&
        HwdbgInterpreterSendPacketAndBufferToHwdbg(
            InstanceInfo,
            TestFilePath,
            DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_HARDWARE_LEVEL,
            hwdbgActionSendInstanceInfo,
            NULL,
            NULL_ZERO))
    {
        ShowMessages("[*] instance info successfully written into file: %s\n", TestFilePath);
        return TRUE;
    }

    //
    // Unable to write instance info request into a file
    //
    return FALSE;
}

/**
 * @brief Load the instance info
 *
 * @param InstanceFilePathToRead
 * @param InitialBramBufferSize
 *
 * @return BOOLEAN
 */
BOOLEAN
CommandHwClkLoadInstanceInfo(const TCHAR * InstanceFilePathToRead, UINT32 InitialBramBufferSize)
{
    UINT32 * MemoryBuffer = NULL;

    //
    // Allocate memory buffer to read the instance info
    //
    MemoryBuffer = (UINT32 *)malloc(InitialBramBufferSize * sizeof(UINT32));

    if (MemoryBuffer == NULL)
    {
        //
        // Memory allocation failed
        //
        ShowMessages("err, unable to allocate memory for the instance info packet of the debuggee");
        return FALSE;
    }

    //
    // *** Read the instance info from the file ***
    //
    if (CommandHwClkReadInstanceInfoFromFile(InstanceFilePathToRead, MemoryBuffer, InitialBramBufferSize))
    {
        ShowMessages("instance info read successfully\n");
    }
    else
    {
        ShowMessages("err, unable to read instance info packet of the debuggee");
        free(MemoryBuffer);
        return FALSE;
    }

    //
    // *** Interpret instance info packet ***
    //
    if (HwdbgInterpretPacket(MemoryBuffer, InitialBramBufferSize))
    {
        ShowMessages("instance info interpreted successfully\n");
        HwdbgShowIntanceInfo(&g_HwdbgInstanceInfo);
    }
    else
    {
        ShowMessages("err, unable to interpret instance info packet of the debuggee");
        free(MemoryBuffer);
        return FALSE;
    }

    //
    // The instance info is loaded successfully
    //
    free(MemoryBuffer);
    return TRUE;
}

/**
 * @brief Create hwdbg script
 * @param ScriptBuffer
 * @param ScriptBufferSize
 * @param HardwareScriptFilePathToSave
 *
 * @return BOOLEAN
 */
BOOLEAN
CommandHwClkCreateHwdbgScript(CHAR *        ScriptBuffer,
                              UINT32        ScriptBufferSize,
                              const TCHAR * HardwareScriptFilePathToSave)
{
    UINT32               NumberOfStagesForScript               = 0;
    UINT32               NumberOfOperandsImplemented           = 0;
    UINT32               NumberOfOperandsForScript             = 0;
    size_t               NewCompressedBufferSize               = 0;
    size_t               NumberOfNeededFlipFlopsInTargetDevice = 0;
    size_t               NumberOfBytesPerChunk                 = 0;
    HWDBG_SHORT_SYMBOL * NewScriptBuffer                       = NULL;

    //
    // *** Check the script capabilities with the generated script ***
    //
    if (!HardwareScriptInterpreterCheckScriptBufferWithScriptCapabilities(&g_HwdbgInstanceInfo,
                                                                          ScriptBuffer,
                                                                          ScriptBufferSize / sizeof(SYMBOL),
                                                                          &NumberOfStagesForScript,
                                                                          &NumberOfOperandsForScript,
                                                                          &NumberOfOperandsImplemented))
    {
        ShowMessages("\n[-] target script is NOT supported by this instance of hwdbg!\n");
        return FALSE;
    }
    else
    {
        ShowMessages("\n[+] target script is supported by this instance of hwdbg!\n");
    }

    //
    // *** Compress the script buffer based on the instance info ***
    //
    if (!CommandHwClkCompressScriptBuffer(&g_HwdbgInstanceInfo,
                                          (SYMBOL *)ScriptBuffer,
                                          ScriptBufferSize,
                                          NumberOfStagesForScript,
                                          &NewScriptBuffer,
                                          &NewCompressedBufferSize,
                                          &NumberOfBytesPerChunk))
    {
        ShowMessages("err, unable to compress the script buffer\n");
        return FALSE;
    }

    //
    // Print the hwdbg script buffer
    //
    CommandHwClkPrintHwdbgScriptBuffer(&g_HwdbgInstanceInfo,
                                       NewCompressedBufferSize,
                                       NumberOfStagesForScript,
                                       NumberOfOperandsForScript,
                                       NewScriptBuffer,
                                       NumberOfNeededFlipFlopsInTargetDevice,
                                       NumberOfBytesPerChunk,
                                       NumberOfOperandsImplemented);

    //
    // *** Write script configuration packet into a file ***
    //

    //
    // Write script configuration packet into a file
    //
    if (!CommandHwClkWriteScriptConfigurationPacketIntoFile(&g_HwdbgInstanceInfo,
                                                            HardwareScriptFilePathToSave,
                                                            NumberOfStagesForScript,
                                                            NumberOfOperandsImplemented,
                                                            NewScriptBuffer,
                                                            NewCompressedBufferSize))
    {
        ShowMessages("err, unable to write script buffer\n");
        return FALSE;
    }

    //
    // *** Free the allocated memory ***
    //

    //
    // Free the allocated memory for the short symbol buffer
    //
    if (NewScriptBuffer != NULL)
    {
        HardwareScriptInterpreterFreeHwdbgShortSymbolBuffer(NewScriptBuffer);
    }

    //
    // The script buffer is created successfully
    //
    return TRUE;
}

/** Get script buffer from raw string
 * @param ScriptBuffer
 * @param CodeBuffer
 * @param BufferAddress
 * @param BufferLength
 * @param Pointer
 *
 * @return BOOLEAN
 */
BOOLEAN
CommandHwClkGetScriptBufferFromRawString(string   ScriptString,
                                         PVOID *  CodeBuffer,
                                         UINT64 * BufferAddress,
                                         UINT32 * BufferLength,
                                         UINT32 * Pointer)
{
    PVOID ResultingCodeBuffer = NULL;

    //
    // Run script engine handler
    //
    ResultingCodeBuffer = ScriptEngineParseWrapper((char *)ScriptString.c_str(), TRUE);

    if (ResultingCodeBuffer == NULL)
    {
        //
        // return to show that this item contains an script error
        //
        return FALSE;
    }

    //
    // Print symbols (test)
    //
    // PrintSymbolBufferWrapper(ResultingCodeBuffer);

    //
    // Set the buffer and length
    //
    *BufferAddress = ScriptEngineWrapperGetHead(ResultingCodeBuffer);
    *BufferLength  = ScriptEngineWrapperGetSize(ResultingCodeBuffer);
    *Pointer       = ScriptEngineWrapperGetPointer(ResultingCodeBuffer);

    //
    // Set the code buffer
    //
    *CodeBuffer = ResultingCodeBuffer;

    //
    // The script buffer is copied successfully
    //
    return TRUE;
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
    if (!CommandHwClkLoadInstanceInfo(InstanceFilePathToRead, InitialBramBufferSize))
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
    CommandHwClkPrintScriptBuffer(ScriptBuffer, ActionScript->ScriptBufferSize);

    //
    // Create hwdbg script
    //
    if (!CommandHwClkCreateHwdbgScript(ScriptBuffer,
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
    if (!CommandHwClkWriteTestInstanceInfoRequestIntoFile(&g_HwdbgInstanceInfo,
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
