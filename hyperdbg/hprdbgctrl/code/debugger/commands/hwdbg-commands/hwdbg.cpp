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
 * @brief Function to parse a single line of the memory content
 *
 * @param Line
 * @return VOID
 */
std::vector<UINT32>
ParseLine(const std::string & Line)
{
    std::vector<UINT32> Values;
    std::stringstream   Ss(Line);
    std::string         Token;

    // Skip the memory address part
    std::getline(Ss, Token, ':');

    // Read the hex value
    while (std::getline(Ss, Token, ' '))
    {
        if (Token.length() == 8 && std::all_of(Token.begin(), Token.end(), ::isxdigit))
        {
            Values.push_back(static_cast<UINT32>(std::stoul(Token, nullptr, 16)));
        }
    }

    return Values;
}

/**
 * @brief Function to read the file and fill the memory buffer
 *
 * @param FileName
 * @param MemoryBuffer
 * @param BufferSize
 * @return BOOLEAN
 */
BOOLEAN
FillMemoryFromFile(const std::string & FileName, UINT32 * MemoryBuffer, size_t BufferSize)
{
    std::ifstream File(FileName);
    std::string   Line;
    BOOLEAN       Result = TRUE;
    size_t        Index  = 0;

    if (!File.is_open())
    {
        ShowMessages("err, unable to open file %s\n", FileName.c_str());
        return FALSE;
    }

    while (getline(File, Line))
    {
        if (Index >= BufferSize)
        {
            Result = FALSE;
            ShowMessages("err, buffer overflow, file contains more data than buffer can hold\n");
            break;
        }

        vector<UINT32> Values = ParseLine(Line);

        for (UINT32 Value : Values)
        {
            if (Index < BufferSize)
            {
                MemoryBuffer[Index++] = Value;
            }
            else
            {
                ShowMessages("err, buffer overflow, file contains more data than buffer can hold\n");
                File.close();
                return FALSE;
            }
        }
    }

    File.close();
    return Result;
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
    vector<string>                     SplitCommandCaseSensitive {Split(Command, ' ')};
    DEBUGGER_EVENT_PARSING_ERROR_CAUSE EventParsingErrorCause;

    if (SplitCommand.size() == 2 && !SplitCommand.at(1).compare("test"))
    {
        TCHAR        TestFilePath[MAX_PATH] = {0};
        const SIZE_T BufferSize             = 256; // Adjust based on the number of memory entries of the file
        UINT32       PortNum                = 0;
        UINT32       MemoryBuffer[BufferSize];

        if (SetupPathForFileName(HWDBG_TEST_INSTANCE_INFO_PATH, TestFilePath, sizeof(TestFilePath)) &&
            FillMemoryFromFile(TestFilePath, MemoryBuffer, BufferSize))
        {
            // Print the content of MemoryBuffer for verification
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
            ShowMessages("Debuggee Number Of Pins: 0x%x\n", g_HwdbgInstanceInfo.numberOfPins);
            ShowMessages("Debuggee Number Of Ports: 0x%x\n", g_HwdbgInstanceInfo.numberOfPorts);

            for (auto item : g_HwdbgPortConfiguration)
            {
                ShowMessages("Port number %d ($hw_port%d): 0x%x\n", PortNum, PortNum, item);
                PortNum++;
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
        ShowMessages("hwdbg script buffer (size=%d, stages=%d, flip-flops=%d):\n",
                     ActionScript->ScriptBufferSize,
                     ActionScript->ScriptBufferSize / 32,
                     ActionScript->ScriptBufferSize * 8);
        CHAR * ScriptBuffer = (CHAR *)((UINT64)ActionScript + sizeof(DEBUGGER_GENERAL_ACTION));

        for (size_t i = 0; i < ActionScript->ScriptBufferSize; i++)
        {
            ShowMessages("%02x ", ScriptBuffer[i]);
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
