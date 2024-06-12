/**
 * @file hwdbg.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !hwdbg command
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
;

/**
 * @brief help of the !hwdbg command
 *
 * @return VOID
 */
VOID
CommandHwdbgHelp()
{
    ShowMessages("!hwdbg : performs actions related to hwdbg hardware debugging.\n\n");

    ShowMessages("syntax : \t!hwdbg  [script { Script (string) }]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !hwdbg script { @hw_pin1 = 0; }\n");
}

/**
 * @brief !hwdbg command handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandHwdbg(vector<string> SplitCommand, string Command)
{
    PDEBUGGER_GENERAL_EVENT_DETAIL     Event                 = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionBreakToDebugger = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionCustomCode      = NULL;
    PDEBUGGER_GENERAL_ACTION           ActionScript          = NULL;
    UINT32                             EventLength;
    UINT64                             SpecialTarget               = 0;
    UINT32                             ActionBreakToDebuggerLength = 0;
    UINT32                             ActionCustomCodeLength      = 0;
    UINT32                             ActionScriptLength          = 0;
    size_t                             NewCompressedBufferSize     = 0;
    size_t                             NumberOfBytesPerChunk       = 0;
    vector<string>                     SplitCommandCaseSensitive {Split(Command, ' ')};
    DEBUGGER_EVENT_PARSING_ERROR_CAUSE EventParsingErrorCause;

    if (SplitCommand.size() >= 2 && !SplitCommand.at(1).compare("test"))
    {
        TCHAR        TestFilePath[MAX_PATH] = {0};
        const SIZE_T BufferSize             = 256; // Adjust based on the number of memory entries of the file
        UINT32       PortNum                = 0;
        UINT32       MemoryBuffer[BufferSize];

        if (SetupPathForFileName(HWDBG_TEST_INSTANCE_INFO_PATH, TestFilePath, sizeof(TestFilePath), TRUE) &&
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
            ShowMessages("Debuggee Maximum Number Of Supported Script Operators: 0x%x\n", g_HwdbgInstanceInfo.maximumNumberOfSupportedScriptOperators);
            ShowMessages("Debuggee Debugger Area Offset: 0x%x\n", g_HwdbgInstanceInfo.debuggerAreaOffset);
            ShowMessages("Debuggee Debuggee Area Offset: 0x%x\n", g_HwdbgInstanceInfo.debuggeeAreaOffset);
            ShowMessages("Debuggee Script Capabilities Mask: 0x%llx\n", g_HwdbgInstanceInfo.scriptCapabilities);

            //
            // Show script capabilities
            //
            HwdbgInterpreterShowScriptCapabilities(&g_HwdbgInstanceInfo);

            ShowMessages("Debuggee Number Of Pins: 0x%x\n", g_HwdbgInstanceInfo.numberOfPins);
            ShowMessages("Debuggee Number Of Ports: 0x%x\n", g_HwdbgInstanceInfo.numberOfPorts);

            for (auto item : g_HwdbgPortConfiguration)
            {
                ShowMessages("Port number %d ($hw_port%d): 0x%x\n", PortNum, PortNum, item);
                PortNum++;
            }

            //
            // Write content of the memory into a file
            //
            if (SetupPathForFileName(HWDBG_TEST_SCRIPT_BUFFER_PATH, TestFilePath, sizeof(TestFilePath), FALSE) &&
                HwdbgInterpreterFillFileFromMemory(TestFilePath, MemoryBuffer, BufferSize))
            {
                ShowMessages("Script buffer successfully written into file: %s\n", TestFilePath);
            }
        }
        else
        {
            ShowMessages("err, unable to interpret instance info packet of the debuggee");
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
        ShowMessages("hwdbg script buffer (size=%d, stages=%d, flip-flops=%d):\n\n",
                     ActionScript->ScriptBufferSize,
                     ActionScript->ScriptBufferSize / sizeof(SYMBOL), // by default four 8 bytes which is equal to 32
                     ActionScript->ScriptBufferSize * 8               // Converted to bits
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
                                                                    ActionScript->ScriptBufferSize / sizeof(SYMBOL)))
        {
            ShowMessages("\n[+] target script is supported by this instance of hwdbg!\n");

            //
            // Now, converting the script based on supported script variable length
            //
            if (g_HwdbgInstanceInfoIsValid)
            {
                if (g_HwdbgInstanceInfo.scriptVariableLength == sizeof(UINT64) * 8)
                {
                    NewCompressedBufferSize = ActionScript->ScriptBufferSize;
                    ShowMessages("the script variable length is same as default length; thus, does not need conversion\n");
                }
                else
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
                        if (HwdbgInterpreterCompressBuffer((UINT64 *)ScriptBuffer,
                                                           ActionScript->ScriptBufferSize,
                                                           g_HwdbgInstanceInfo.scriptVariableLength,
                                                           &NewCompressedBufferSize,
                                                           &NumberOfBytesPerChunk) == TRUE)
                        {
                            ShowMessages("\n---------------------------------------------------------\n");
                            ShowMessages("compressed script buffer size: 0x%x\n", g_HwdbgInstanceInfo.scriptVariableLength);

                            ShowMessages("hwdbg script buffer (size=%d, stages=%d, flip-flops=%d, number of bytes per chunk: %d):\n\n",
                                         NewCompressedBufferSize,
                                         NewCompressedBufferSize / (NumberOfBytesPerChunk * 4), // Multiplied by 4 because there are 4 fields in SYMBOL structure
                                         NewCompressedBufferSize * 8,                           // Converted to bits
                                         NumberOfBytesPerChunk);

                            for (size_t i = 0; i < NewCompressedBufferSize; i++)
                            {
                                ShowMessages("%02X ", (UINT8)ScriptBuffer[i]);
                            }

                            ShowMessages("\n");
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
        }
        else
        {
            ShowMessages("\n[-] target script is NOT supported by this instance of hwdbg!\n");
        }

        //
        // Free the allocated memory
        //
        FreeEventsAndActionsMemory(Event, ActionBreakToDebugger, ActionCustomCode, ActionScript);
    }
    else
    {
        ShowMessages("incorrect use of the '%s'\n\n", SplitCommand.at(0).c_str());
        CommandHwdbgHelp();
        return;
    }
}
