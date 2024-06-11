/**
 * @file hwdbg-interpreter.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Interpreter of hwdbg packets and requests
 * @details
 * @version 1.0
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
;

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
        case hwdbgResponseInvalidPacketOrError:

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

            break;

        case hwdbgResponseScriptBufferConfigurationResult:

            Result = TRUE;

            //
            // Todo: implement it
            //

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
HwdbgInterpreterFillMemoryFromFile(const TCHAR * FileName, UINT32 * MemoryBuffer, size_t BufferSize)
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
 * @brief Function to write the memory buffer to a file in the specified format
 *
 * @param FileName
 * @param MemoryBuffer
 * @param BufferSize
 * @return BOOLEAN
 */
BOOLEAN
HwdbgInterpreterFillFileFromMemory(const TCHAR * FileName, UINT32 * MemoryBuffer, size_t BufferSize)
{
    std::ofstream File(FileName);

    if (!File.is_open())
    {
        printf("err, unable to open file %s\n", FileName);
        return FALSE;
    }

    size_t Address = 0;
    for (size_t I = 0; I < BufferSize; ++I)
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
            File << "   | TypeOfThePacket - DEBUGGER_TO_DEBUGGEE_HARDWARE_LEVEL (0x4)";
        }
        else if (I == 5)
        {
            File << "   | RequestedActionOfThePacket - hwdbgActionSendInstanceInfo (0x1)";
        }
        else if (I == 6)
        {
            File << "   | Start of Optional Data";
        }

        File << "\n";
        Address += 4;
    }

    File.close();

    return TRUE;
}

/**
 * @brief Function to compress the buffer
 *
 * @param Buffer
 * @param BufferLength
 * @param CompressBitSize
 * @param NewBufferSize
 * @param NumberOfBytesPerChunk
 *
 * @return BOOLEAN
 */
BOOLEAN
HwdbgInterpreterCompressBuffer(UINT64 * Buffer,
                               size_t   BufferLength,
                               int      CompressBitSize,
                               size_t * NewBufferSize,
                               size_t * NumberOfBytesPerChunk)
{
    if (CompressBitSize <= 0 || CompressBitSize > 64)
    {
        ShowMessages("err, invalid bit size, it should be between 1 and 64\n");
        return FALSE;
    }

    //
    // Calculate the number of 64-bit chunks
    //
    size_t NumberOfChunks = BufferLength / sizeof(UINT64);

    //
    // Calculate the number of bytes needed for the new compressed buffer
    //
    size_t NewBytesPerChunk = (CompressBitSize + 7) / 8; // ceil(CompressBitSize / 8)
    *NumberOfBytesPerChunk  = NewBytesPerChunk;
    *NewBufferSize          = NumberOfChunks * NewBytesPerChunk;

    //
    // Create a temporary buffer to hold the compressed data
    //
    UINT8 * TempBuffer = (UINT8 *)malloc(*NewBufferSize);

    if (TempBuffer == NULL)
    {
        ShowMessages("err, memory allocation failed\n");
        return FALSE;
    }

    //
    // Compress each chunk and store it in the temporary buffer
    //
    for (size_t i = 0; i < NumberOfChunks; ++i)
    {
        uint64_t Chunk = Buffer[i];
        for (size_t j = 0; j < NewBytesPerChunk; ++j)
        {
            TempBuffer[i * NewBytesPerChunk + j] = (uint8_t)((Chunk >> (j * 8)) & 0xFF);
        }
    }

    //
    // Copy the compressed data back to the original buffer
    //
    RtlZeroMemory(Buffer, BufferLength);
    memcpy(Buffer, TempBuffer, *NewBufferSize);

    //
    // Free the temporary buffer
    //
    free(TempBuffer);

    return TRUE;
}
