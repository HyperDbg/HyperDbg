/**
 * @file hwdbg-interpreter.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Interpreter of hwdbg packets and requests
 * @details
 * @version 0.10
 * @date 2024-06-11
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

/**
 * @brief Interpret packets of hwdbg
 *
 * @param BufferReceived
 * @param LengthReceived
 * @return BOOLEAN
 */
BOOLEAN
HwdbgInterpretPacket(PVOID BufferReceived, UINT32 LengthReceived)
{
    PHWDBG_INSTANCE_INFORMATION InstanceInfoPacket;
    PUINT32                     InstanceInfoPorts;
    DEBUGGER_REMOTE_PACKET *    TheActualPacket = NULL;
    BOOLEAN                     Result          = FALSE;

    //
    // Apply the initial offset
    //
    if (g_HwdbgInstanceInfoIsValid)
    {
        //
        // Use the debuggee's preferred offset (area) since the instance info
        // already received and interpreted
        //
        TheActualPacket = (DEBUGGER_REMOTE_PACKET *)(((CHAR *)BufferReceived) + g_HwdbgInstanceInfo.debuggeeAreaOffset);
    }
    else
    {
        //
        // Use default initial offset as there is no information (instance info)
        // from debuggee
        //
        TheActualPacket = (DEBUGGER_REMOTE_PACKET *)(((CHAR *)BufferReceived) + DEFAULT_INITIAL_DEBUGGEE_TO_DEBUGGER_OFFSET);
    }

    if (TheActualPacket->Indicator == INDICATOR_OF_HYPERDBG_PACKET)
    {
        //
        // Check checksum (for hwdbg, checksum is ignored)
        //
        // if (KdComputeDataChecksum((PVOID)&TheActualPacket->Indicator,
        //                           LengthReceived - sizeof(BYTE)) != TheActualPacket->Checksum)
        // {
        //     ShowMessages("err, checksum is invalid\n");
        //     return FALSE;
        // }

        //
        // Check if the packet type is correct
        //
        if (TheActualPacket->TypeOfThePacket != DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER_HARDWARE_LEVEL)
        {
            //
            // sth wrong happened, the packet is not belonging to use
            // for hwdbg interpreter
            //
            ShowMessages("err, unknown packet received from the debuggee\n");
            return FALSE;
        }

        //
        // It's a HyperDbg packet (for hwdbg)
        //
        switch (TheActualPacket->RequestedActionOfThePacket)
        {
        case hwdbgResponseSuccessOrErrorMessage:

            Result = TRUE;

            //
            // Todo: implement it
            //

            break;

        case hwdbgResponseInstanceInfo:

            Result             = TRUE;
            InstanceInfoPacket = (HWDBG_INSTANCE_INFORMATION *)(((CHAR *)TheActualPacket) + sizeof(DEBUGGER_REMOTE_PACKET));
            InstanceInfoPorts  = (UINT32 *)(((CHAR *)InstanceInfoPacket) + sizeof(HWDBG_INSTANCE_INFORMATION));

            //
            // Copy the instance info into the global hwdbg instance info
            //
            RtlCopyMemory(&g_HwdbgInstanceInfo, InstanceInfoPacket, sizeof(HWDBG_INSTANCE_INFORMATION));

            //
            // Reset previous port configurations (if any)
            //
            g_HwdbgPortConfiguration.clear();

            //
            // Instance info is valid from now
            //
            g_HwdbgInstanceInfoIsValid = TRUE;

            //
            // Read port arrangements
            //
            for (size_t i = 0; i < g_HwdbgInstanceInfo.numberOfPorts; i++)
            {
                g_HwdbgPortConfiguration.push_back(InstanceInfoPorts[i]);
            }

            //
            // Infom the script engine about the instance info
            //
            ScriptEngineSetHwdbgInstanceInfo(&g_HwdbgInstanceInfo);

            break;

        default:

            Result = FALSE;
            ShowMessages("err, unknown packet request received from the debuggee\n");

            break;
        }
    }

    //
    // Packet handled successfully
    //
    return Result;
}

/**
 * @brief Function to parse a single line of the memory content
 *
 * @param Line
 * @return VOID
 */
std::vector<UINT32>
HwdbgParseStringMemoryLine(const std::string & Line)
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
HwdbgInterpreterFillMemoryFromFile(
    const TCHAR * FileName,
    UINT32 *      MemoryBuffer,
    size_t        BufferSize)
{
    std::ifstream File(FileName);
    std::string   Line;
    BOOLEAN       Result = TRUE;
    size_t        Index  = 0;

    if (!File.is_open())
    {
        ShowMessages("err, unable to open file %s\n", FileName);
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

        vector<UINT32> Values = HwdbgParseStringMemoryLine(Line);

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
 * @brief Function to write the memory buffer to a file in the specified format
 *
 * @param InstanceInfo
 * @param FileName
 * @param MemoryBuffer
 * @param BufferSize
 * @param RequestedAction
 *
 * @return BOOLEAN
 */
BOOLEAN
HwdbgInterpreterFillFileFromMemory(
    HWDBG_INSTANCE_INFORMATION * InstanceInfo,
    const TCHAR *                FileName,
    UINT32 *                     MemoryBuffer,
    size_t                       BufferSize,
    HWDBG_ACTION_ENUMS           RequestedAction)
{
    std::ofstream File(FileName);

    if (!File.is_open())
    {
        printf("err, unable to open file %s\n", FileName);
        return FALSE;
    }

    size_t Address = 0;
    for (size_t I = 0; I < BufferSize / sizeof(UINT32); ++I)
    {
        File << std::hex << std::setw(8) << std::setfill('0') << MemoryBuffer[I];
        File << " ; +0x" << std::hex << std::setw(1) << std::setfill('0') << Address;

        if (I == 0)
        {
            File << "   | Checksum";
        }
        else if (I == 1)
        {
            File << "   | Checksum";
        }
        else if (I == 2)
        {
            File << "   | Indicator";
        }
        else if (I == 3)
        {
            File << "   | Indicator";
        }
        else if (I == 4)
        {
            File << "  | TypeOfThePacket - DEBUGGER_TO_DEBUGGEE_HARDWARE_LEVEL (0x4)";
        }
        else if (I == 5)
        {
            File << "  | RequestedActionOfThePacket - Value" << " (0x" << std::hex << std::setw(1) << std::setfill('0') << RequestedAction << ")";
        }
        else if (I == 6)
        {
            File << "  | Start of Optional Data";
        }

        File << "\n";
        Address += 4;
    }

    //
    // Add zeros to the end of the file to fill the shared memory
    //
    if (g_HwdbgInstanceInfoIsValid)
    {
        while (Address < InstanceInfo->sharedMemorySize)
        {
            File << "00000000 ; +0x" << std::hex << std::setw(1) << std::setfill('0') << Address;
            Address += 4;

            if (Address < InstanceInfo->sharedMemorySize)
            {
                File << "\n";
            }
        }
    }

    //
    // Close the file
    //
    File.close();

    return TRUE;
}

/**
 * @brief Function to compute number of flip-flops needed in the target device
 *
 * @param InstanceInfo
 * @param NumberOfStages
 *
 * @return SIZE_T
 */
SIZE_T
HwdbgComputeNumberOfFlipFlopsNeeded(
    HWDBG_INSTANCE_INFORMATION * InstanceInfo,
    UINT32                       NumberOfStages)
{
    //
    // Calculate the number of flip-flops needed in the target device
    // + operator symbol itself which only contains value (type is always equal to SYMBOL_SEMANTIC_RULE_TYPE)
    // so, it is not counted as a flip-flop
    //
    SIZE_T NumberOfNeededFlipFlopsInTargetDevice = 0;

    //
    // size of operator (GET and SET)
    //
    NumberOfNeededFlipFlopsInTargetDevice += (NumberOfStages *
                                              (g_HwdbgInstanceInfo.maximumNumberOfSupportedGetScriptOperators + g_HwdbgInstanceInfo.maximumNumberOfSupportedSetScriptOperators) *
                                              g_HwdbgInstanceInfo.scriptVariableLength *
                                              sizeof(HWDBG_SHORT_SYMBOL) / sizeof(UINT64));

    //
    // size of main operator (/ 2 is becasue Type is not inffered)
    //
    NumberOfNeededFlipFlopsInTargetDevice += (NumberOfStages * g_HwdbgInstanceInfo.scriptVariableLength * (sizeof(HWDBG_SHORT_SYMBOL) / sizeof(UINT64)) / 2);

    //
    // size of local (and global) variables
    //
    NumberOfNeededFlipFlopsInTargetDevice += (NumberOfStages * g_HwdbgInstanceInfo.numberOfSupportedLocalAndGlobalVariables * g_HwdbgInstanceInfo.scriptVariableLength);

    //
    // size of temporary variables
    //
    NumberOfNeededFlipFlopsInTargetDevice += (NumberOfStages * g_HwdbgInstanceInfo.numberOfSupportedTemporaryVariables * g_HwdbgInstanceInfo.scriptVariableLength);

    //
    // size of stage index register + targetStage (* 2)
    //
    NumberOfNeededFlipFlopsInTargetDevice += (NumberOfStages * Log2Ceil(g_HwdbgInstanceInfo.maximumNumberOfStages * (g_HwdbgInstanceInfo.maximumNumberOfSupportedGetScriptOperators + g_HwdbgInstanceInfo.maximumNumberOfSupportedSetScriptOperators + 1)) * 2);

    //
    // stage enable flip-flop
    //
    NumberOfNeededFlipFlopsInTargetDevice += (NumberOfStages);

    //
    // input => output flip-flop
    //
    NumberOfNeededFlipFlopsInTargetDevice += (NumberOfStages * g_HwdbgInstanceInfo.numberOfPins);

    //
    // return the number of flip-flops needed in the target device
    //
    return NumberOfNeededFlipFlopsInTargetDevice;
}

/**
 * @brief Sends a HyperDbg packet + a buffer to the hwdbg
 *
 * @param InstanceInfo
 * @param FileName
 * @param PacketType
 * @param RequestedAction
 * @param Buffer
 * @param BufferLength
 *
 * @return BOOLEAN
 */
BOOLEAN
HwdbgInterpreterSendPacketAndBufferToHwdbg(HWDBG_INSTANCE_INFORMATION * InstanceInfo,
                                           const TCHAR *                FileName,
                                           DEBUGGER_REMOTE_PACKET_TYPE  PacketType,
                                           HWDBG_ACTION_ENUMS           RequestedAction,
                                           CHAR *                       Buffer,
                                           UINT32                       BufferLength)
{
    DEBUGGER_REMOTE_PACKET Packet          = {0};
    SIZE_T                 CommandMaxSize  = 0;
    SIZE_T                 FinalBufferSize = 0;

    if (g_HwdbgInstanceInfoIsValid)
    {
        CommandMaxSize = InstanceInfo->debuggeeAreaOffset - InstanceInfo->debuggerAreaOffset;
    }
    else
    {
        //
        // Use default limitation
        //
        CommandMaxSize = DEFAULT_INITIAL_DEBUGGEE_TO_DEBUGGER_OFFSET - DEFAULT_INITIAL_DEBUGGER_TO_DEBUGGEE_OFFSET;
    }

    //
    // If buffer is not available, then the length is zero
    //
    if (Buffer == NULL)
    {
        BufferLength = 0;
    }

    //
    // Compute the final buffer size
    //
    FinalBufferSize = sizeof(DEBUGGER_REMOTE_PACKET) + BufferLength;

    //
    // Check if buffer not pass the boundary
    //
    if (FinalBufferSize > CommandMaxSize)
    {
        ShowMessages("err, buffer is above the maximum buffer size that can be sent to hwdbg (%d > %d)\n",
                     FinalBufferSize,
                     CommandMaxSize);

        return FALSE;
    }

    //
    // Make the packet's structure
    //
    Packet.Indicator       = INDICATOR_OF_HYPERDBG_PACKET;
    Packet.TypeOfThePacket = PacketType;

    //
    // Set the requested action
    //
    Packet.RequestedActionOfThePacket = (DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION)RequestedAction;

    //
    // calculate checksum of the packet
    //
    Packet.Checksum =
        KdComputeDataChecksum((PVOID)((UINT64)&Packet + 1),
                              sizeof(DEBUGGER_REMOTE_PACKET) - sizeof(BYTE));

    if (Buffer != NULL)
    {
        Packet.Checksum += KdComputeDataChecksum((PVOID)Buffer, BufferLength);
    }

    //
    // If there is an offset for debugger to debuggee command, we'll apply it here
    //
    if (g_HwdbgInstanceInfoIsValid)
    {
        FinalBufferSize += InstanceInfo->debuggerAreaOffset;
    }
    else
    {
        FinalBufferSize += DEFAULT_INITIAL_DEBUGGER_TO_DEBUGGEE_OFFSET;
    }

    //
    // Allocate a buffer for storing the header packet + buffer (if not empty)
    //
    CHAR * FinalBuffer = (CHAR *)malloc(FinalBufferSize);

    if (!FinalBuffer)
    {
        return FALSE;
    }

    RtlZeroMemory(FinalBuffer, FinalBufferSize);

    //
    // Leave the offset
    //
    SIZE_T Offset = g_HwdbgInstanceInfoIsValid ? InstanceInfo->debuggerAreaOffset : DEFAULT_INITIAL_DEBUGGER_TO_DEBUGGEE_OFFSET;

    //
    // Copy the packet into the FinalBuffer
    //
    memcpy(FinalBuffer + Offset, &Packet, sizeof(DEBUGGER_REMOTE_PACKET));

    //
    // Copy the buffer (if available) into the FinalBuffer
    //
    if (Buffer != NULL)
    {
        memcpy(FinalBuffer + Offset + sizeof(DEBUGGER_REMOTE_PACKET), Buffer, BufferLength);
    }

    //
    // Here you would send FinalBuffer to the hardware debugger
    //
    HwdbgInterpreterFillFileFromMemory(InstanceInfo, FileName, (UINT32 *)FinalBuffer, FinalBufferSize, RequestedAction);

    //
    // Free the allocated memory after use
    //
    free(FinalBuffer);

    return TRUE;
}

/**
 * @brief Show instance info details
 *
 * @param InstanceInfo
 *
 * @return VOID
 */
VOID
HwdbgShowIntanceInfo(HWDBG_INSTANCE_INFORMATION * InstanceInfo)
{
    UINT32 PortNum = 0;

    ShowMessages("Debuggee Version: 0x%x\n", InstanceInfo->version);
    ShowMessages("Debuggee Maximum Number Of Stages: 0x%x\n", InstanceInfo->maximumNumberOfStages);
    ShowMessages("Debuggee Script Variable Length: 0x%x\n", InstanceInfo->scriptVariableLength);
    ShowMessages("Debuggee Number of Supported Local (and global) Variables: 0x%x\n", InstanceInfo->numberOfSupportedLocalAndGlobalVariables);
    ShowMessages("Debuggee Number of Supported Temporary Variables: 0x%x\n", InstanceInfo->numberOfSupportedTemporaryVariables);
    ShowMessages("Debuggee Maximum Number Of Supported GET Script Operators: 0x%x\n", InstanceInfo->maximumNumberOfSupportedGetScriptOperators);
    ShowMessages("Debuggee Maximum Number Of Supported SET Script Operators: 0x%x\n", InstanceInfo->maximumNumberOfSupportedSetScriptOperators);
    ShowMessages("Debuggee Shared Memory Size: 0x%x\n", InstanceInfo->sharedMemorySize);
    ShowMessages("Debuggee Debugger Area Offset: 0x%x\n", InstanceInfo->debuggerAreaOffset);
    ShowMessages("Debuggee Debuggee Area Offset: 0x%x\n", InstanceInfo->debuggeeAreaOffset);
    ShowMessages("Debuggee Script Capabilities Mask: 0x%llx\n", InstanceInfo->scriptCapabilities);

    //
    // Show script capabilities
    //
    HardwareScriptInterpreterShowScriptCapabilities(&g_HwdbgInstanceInfo);

    ShowMessages("Debuggee Number Of Pins: 0x%x\n", InstanceInfo->numberOfPins);
    ShowMessages("Debuggee Number Of Ports: 0x%x\n", InstanceInfo->numberOfPorts);

    ShowMessages("Debuggee BRAM Address Width: 0x%x\n", InstanceInfo->bramAddrWidth);
    ShowMessages("Debuggee BRAM Data Width: 0x%x (%d bit)\n", InstanceInfo->bramDataWidth, InstanceInfo->bramDataWidth);

    for (auto item : g_HwdbgPortConfiguration)
    {
        ShowMessages("Port number %d ($hw_port%d): 0x%x\n", PortNum, PortNum, item);
        PortNum++;
    }
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
HwdbgReadInstanceInfoFromFile(const TCHAR * FileName, UINT32 * MemoryBuffer, size_t BufferSize)
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
 * @brief Write test instance info request into a file
 *
 * @param InstanceInfo
 * @param FileName
 *
 * @return BOOLEAN
 */
BOOLEAN
HwdbgWriteTestInstanceInfoRequestIntoFile(HWDBG_INSTANCE_INFORMATION * InstanceInfo,
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
HwdbgLoadInstanceInfo(const TCHAR * InstanceFilePathToRead, UINT32 InitialBramBufferSize)
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
    if (HwdbgReadInstanceInfoFromFile(InstanceFilePathToRead, MemoryBuffer, InitialBramBufferSize))
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
