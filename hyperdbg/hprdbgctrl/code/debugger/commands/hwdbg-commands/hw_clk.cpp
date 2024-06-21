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
 * @brief !hw_clk command handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandHwClk(vector<string> SplitCommand, string Command)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL     Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionScript          = NULL;
    UINT32                             EventLength;
    UINT64                             SpecialTarget                         = 0;
    UINT32                             ActionBreakToDebuggerLength           = 0;
    UINT32                             ActionCustomCodeLength                = 0;
    UINT32                             ActionScriptLength                    = 0;
    UINT32                             NumberOfStagesForScript               = 0;
    UINT32                             NumberOfOperandsForScript             = 0;
    size_t                             NewCompressedBufferSize               = 0;
    size_t                             NumberOfNeededFlipFlopsInTargetDevice = 0;
    size_t                             NumberOfBytesPerChunk                 = 0;
    vector<string>                     SplitCommandCaseSensitive {Split(Command, ' ')};
    DEBUGGER_EVENT_PARSING_ERROR_CAUSE EventParsingErrorCause;
    HWDBG_SHORT_SYMBOL *               NewScriptBuffer = NULL;

    if (SplitCommand.size() >= 2 && !SplitCommand.at(1).compare("test"))
    {
        TCHAR        TestFilePath[MAX_PATH] = {0};
        const SIZE_T BufferSize             = 256; // Adjust based on the number of memory entries of the file
        UINT32       PortNum                = 0;
        UINT32       MemoryBuffer[BufferSize];

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
        }

        //
        // Interpret packet
        //
        if (HwdbgInterpretPacket(MemoryBuffer, BufferSize))
        {
            ShowMessages("instance info interpreted successfully\n");

            ShowMessages("Debuggee Version: 0x%x\n", g_HwdbgInstanceInfo.version);
            ShowMessages("Debuggee Maximum Number Of Stages: 0x%x\n", g_HwdbgInstanceInfo.maximumNumberOfStages);
            ShowMessages("Debuggee Script Variable Length: 0x%x\n", g_HwdbgInstanceInfo.scriptVariableLength);
            ShowMessages("Debuggee Maximum Number Of Supported GET Script Operators: 0x%x\n", g_HwdbgInstanceInfo.maximumNumberOfSupportedGetScriptOperators);
            ShowMessages("Debuggee Maximum Number Of Supported SET Script Operators: 0x%x\n", g_HwdbgInstanceInfo.maximumNumberOfSupportedSetScriptOperators);
            ShowMessages("Debuggee Shared Memory Size: 0x%x\n", g_HwdbgInstanceInfo.sharedMemorySize);
            ShowMessages("Debuggee Debugger Area Offset: 0x%x\n", g_HwdbgInstanceInfo.debuggerAreaOffset);
            ShowMessages("Debuggee Debuggee Area Offset: 0x%x\n", g_HwdbgInstanceInfo.debuggeeAreaOffset);
            ShowMessages("Debuggee Script Capabilities Mask: 0x%llx\n", g_HwdbgInstanceInfo.scriptCapabilities);

            //
            // Show script capabilities
            //
            HwdbgInterpreterShowScriptCapabilities(&g_HwdbgInstanceInfo);

            ShowMessages("Debuggee Number Of Pins: 0x%x\n", g_HwdbgInstanceInfo.numberOfPins);
            ShowMessages("Debuggee Number Of Ports: 0x%x\n", g_HwdbgInstanceInfo.numberOfPorts);

            ShowMessages("Debuggee BRAM Address Width: 0x%x\n", g_HwdbgInstanceInfo.bramAddrWidth);
            ShowMessages("Debuggee BRAM Data Width: 0x%x (%d bit)\n", g_HwdbgInstanceInfo.bramDataWidth, g_HwdbgInstanceInfo.bramDataWidth);

            for (auto item : g_HwdbgPortConfiguration)
            {
                ShowMessages("Port number %d ($hw_port%d): 0x%x\n", PortNum, PortNum, item);
                PortNum++;
            }
        }
        else
        {
            ShowMessages("err, unable to interpret instance info packet of the debuggee");
            return;
        }

        //
        // Interpret and fill the general event and action fields
        //
        //
        if (!InterpretGeneralEventAndActionsFields(
                &SplitCommand,
                &SplitCommandCaseSensitive,
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
            return;
        }

        //
        // Print the actual script
        //
        ShowMessages("\nHyperDbg (general) script buffer (size=%d, flip-flops=%d):\n\n",
                     ActionScript->ScriptBufferSize,
                     ActionScript->ScriptBufferSize * 8 // Converted to bits
        );

        CHAR * ScriptBuffer = (CHAR *)((UINT64)ActionScript + sizeof(DEBUGGER_GENERAL_ACTION));

        for (size_t i = 0; i < ActionScript->ScriptBufferSize; i++)
        {
            ShowMessages("%02X ", (UINT8)ScriptBuffer[i]);
        }

        ShowMessages("\n");

        //
        // Check the script capabilities with the generated script
        //
        if (HwdbgInterpreterCheckScriptBufferWithScriptCapabilities(&g_HwdbgInstanceInfo,
                                                                    ScriptBuffer,
                                                                    ActionScript->ScriptBufferSize / sizeof(SYMBOL),
                                                                    &NumberOfStagesForScript,
                                                                    &NumberOfOperandsForScript))
        {
            ShowMessages("\n[+] target script is supported by this instance of hwdbg!\n");

            //
            // Now, converting the script based on supported script variable length
            //
            if (g_HwdbgInstanceInfoIsValid)
            {
                //
                // Conversion needed
                //
                if (g_HwdbgInstanceInfo.scriptVariableLength >= sizeof(BYTE) * 8)
                {
                    //
                    // The script variable length is valid (at least 8 bit (1 byte)
                    //

                    //
                    // Compress script buffer
                    //
                    if (HwdbgInterpreterConvertSymbolToHwdbgShortSymbolBuffer(&g_HwdbgInstanceInfo,
                                                                              (SYMBOL *)ScriptBuffer,
                                                                              ActionScript->ScriptBufferSize,
                                                                              NumberOfStagesForScript,
                                                                              &NewScriptBuffer,
                                                                              &NewCompressedBufferSize) == TRUE &&

                        //
                        // we put bram data width size here instead of script variable length (g_HwdbgInstanceInfo.scriptVariableLength)
                        // since we want it to read one symbol filed at a time
                        //
                        HwdbgInterpreterCompressBuffer((UINT64 *)NewScriptBuffer,
                                                       NewCompressedBufferSize,
                                                       g_HwdbgInstanceInfo.scriptVariableLength,
                                                       g_HwdbgInstanceInfo.bramDataWidth,
                                                       &NewCompressedBufferSize,
                                                       &NumberOfBytesPerChunk) == TRUE)
                    {
                        ShowMessages("\n---------------------------------------------------------\n");

                        UINT32 NumberOfOperandsImplemented = NumberOfStagesForScript * (g_HwdbgInstanceInfo.maximumNumberOfSupportedGetScriptOperators + g_HwdbgInstanceInfo.maximumNumberOfSupportedSetScriptOperators);

                        //
                        // Calculate the number of flip-flops needed in the target device
                        // + operator symbol itself which only contains value (type is always equal to SYMBOL_SEMANTIC_RULE_TYPE)
                        // so, it is not counted as a flip-flop
                        //
                        NumberOfNeededFlipFlopsInTargetDevice = (NumberOfStagesForScript *
                                                                 (g_HwdbgInstanceInfo.maximumNumberOfSupportedGetScriptOperators + g_HwdbgInstanceInfo.maximumNumberOfSupportedSetScriptOperators) *
                                                                 g_HwdbgInstanceInfo.scriptVariableLength *
                                                                 sizeof(HWDBG_SHORT_SYMBOL) / sizeof(UINT64)) +
                                                                (NumberOfStagesForScript *
                                                                 g_HwdbgInstanceInfo.scriptVariableLength *
                                                                 (sizeof(HWDBG_SHORT_SYMBOL) / sizeof(UINT64)) / 2);

                        ShowMessages("hwdbg script buffer (buffer size=%d, stages=%d, operands needed: %d - operands used: %d (%.2f%%), flip-flops=%d, number of bytes per chunk: %d):\n\n",
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

                        ShowMessages("\n\nwriting script configuration packet into the file\n");

                        //
                        // *** Write script configuration packet into a file ***
                        //
                        if (SetupPathForFileName(HWDBG_TEST_WRITE_SCRIPT_BUFFER_PATH, TestFilePath, sizeof(TestFilePath), FALSE) &&
                            HwdbgInterpreterSendScriptPacket(&g_HwdbgInstanceInfo,
                                                             TestFilePath,
                                                             NumberOfStagesForScript + NumberOfOperandsImplemented - 1, // Number of symbols = Number of stages + Number of operands - 1
                                                             NewScriptBuffer,
                                                             (UINT32)NewCompressedBufferSize))
                        {
                            ShowMessages("\n[*] script buffer successfully written into file: %s\n", TestFilePath);
                        }
                        else
                        {
                            ShowMessages("err, unable to write script buffer\n");
                        }

                        //
                        // *** Write test instance info request into a file ***
                        //
                        if (SetupPathForFileName(HWDBG_TEST_WRITE_INSTANCE_INFO_PATH, TestFilePath, sizeof(TestFilePath), FALSE) &&
                            HwdbgInterpreterSendPacketAndBufferToHwdbg(
                                &g_HwdbgInstanceInfo,
                                TestFilePath,
                                DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_HARDWARE_LEVEL,
                                hwdbgActionSendInstanceInfo,
                                NULL,
                                NULL_ZERO))
                        {
                            ShowMessages("[*] instance info successfully written into file: %s\n", TestFilePath);
                        }
                    }
                }
                else
                {
                    //
                    // The script variable length is not valid (at least 8 bit (1 byte)
                    //
                    ShowMessages("err, the script variable length should be at least 8 bits (1 byte)\n");
                }
            }
        }
        else
        {
            ShowMessages("\n[-] target script is NOT supported by this instance of hwdbg!\n");
        }

        //
        // Free the allocated memory
        //
        if (NewScriptBuffer != NULL)
        {
            free(NewScriptBuffer);
        }
        FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
    }
    else
    {
        ShowMessages("incorrect use of the '%s'\n\n", SplitCommand.at(0).c_str());
        CommandHwClkHelp();
        return;
    }
}
