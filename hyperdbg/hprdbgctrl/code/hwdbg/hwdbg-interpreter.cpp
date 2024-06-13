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
 * @brief Shows the script capablities of the target debuggee
 *
 * @param InstanceInfo
 *
 * @return VOID
 */
VOID
HwdbgInterpreterShowScriptCapabilities(HWDBG_INSTANCE_INFORMATION * InstanceInfo)
{
    ShowMessages("\nThis debuggee supports the following operatiors:\n");
    ShowMessages("\tincrement: %s \n", InstanceInfo->scriptCapabilities.func_inc ? "supported" : "not supported");
    ShowMessages("\tdecrement: %s \n", InstanceInfo->scriptCapabilities.func_dec ? "supported" : "not supported");
    ShowMessages("\tor: %s \n", InstanceInfo->scriptCapabilities.func_or ? "supported" : "not supported");
    ShowMessages("\txor: %s \n", InstanceInfo->scriptCapabilities.func_xor ? "supported" : "not supported");
    ShowMessages("\tand: %s \n", InstanceInfo->scriptCapabilities.func_and ? "supported" : "not supported");
    ShowMessages("\tarithmetic shift right: %s \n", InstanceInfo->scriptCapabilities.func_asr ? "supported" : "not supported");
    ShowMessages("\tarithmetic shift left: %s \n", InstanceInfo->scriptCapabilities.func_asl ? "supported" : "not supported");
    ShowMessages("\taddition: %s \n", InstanceInfo->scriptCapabilities.func_add ? "supported" : "not supported");
    ShowMessages("\tsubtraction: %s \n", InstanceInfo->scriptCapabilities.func_sub ? "supported" : "not supported");
    ShowMessages("\tmultiplication: %s \n", InstanceInfo->scriptCapabilities.func_mul ? "supported" : "not supported");
    ShowMessages("\tdivision: %s \n", InstanceInfo->scriptCapabilities.func_div ? "supported" : "not supported");
    ShowMessages("\tmodulus: %s \n", InstanceInfo->scriptCapabilities.func_mod ? "supported" : "not supported");
    ShowMessages("\tgreater than: %s \n", InstanceInfo->scriptCapabilities.func_gt ? "supported" : "not supported");
    ShowMessages("\tless than: %s \n", InstanceInfo->scriptCapabilities.func_lt ? "supported" : "not supported");
    ShowMessages("\tgreater than or equal to: %s \n", InstanceInfo->scriptCapabilities.func_egt ? "supported" : "not supported");
    ShowMessages("\tless than or equal to: %s \n", InstanceInfo->scriptCapabilities.func_elt ? "supported" : "not supported");
    ShowMessages("\tequal: %s \n", InstanceInfo->scriptCapabilities.func_equal ? "supported" : "not supported");
    ShowMessages("\tnot equal: %s \n", InstanceInfo->scriptCapabilities.func_neq ? "supported" : "not supported");
    ShowMessages("\tjump: %s \n", InstanceInfo->scriptCapabilities.func_jmp ? "supported" : "not supported");
    ShowMessages("\tjump if zero: %s \n", InstanceInfo->scriptCapabilities.func_jz ? "supported" : "not supported");
    ShowMessages("\tjump if not zero: %s \n", InstanceInfo->scriptCapabilities.func_jnz ? "supported" : "not supported");
    ShowMessages("\tmove: %s \n", InstanceInfo->scriptCapabilities.func_mov ? "supported" : "not supported");
    ShowMessages("\tprint: %s \n", InstanceInfo->scriptCapabilities.func_printf ? "supported" : "not supported");
    ShowMessages("\n");
}

/**
 * @brief Check the script capablities with the target script buffer
 *
 * @param InstanceInfo
 * @param ScriptBuffer
 *
 * @return BOOLEAN TRUE if the script capablities support the script, otherwise FALSE
 */
BOOLEAN
HwdbgInterpreterCheckScriptBufferWithScriptCapabilities(HWDBG_INSTANCE_INFORMATION * InstanceInfo,
                                                        PVOID                        ScriptBuffer,
                                                        UINT32                       CountOfScriptSymbolChunks)
{
    BOOLEAN  NotSupported = FALSE;
    SYMBOL * SymbolArray  = (SYMBOL *)ScriptBuffer;

    for (size_t i = 0; i < CountOfScriptSymbolChunks; i++)
    {
        if (SymbolArray[i].Type != SYMBOL_SEMANTIC_RULE_TYPE)
        {
            ShowMessages("  \tfound a non-semnatic rule (operand) type: 0x%x, at: 0x%x\n", SymbolArray[i].Type, i);
            continue;
        }
        else
        {
            ShowMessages("- found a semnatic rule (operator) value: 0x%x, at: 0x%x\n", SymbolArray[i].Value, i);
        }

        switch (SymbolArray[i].Value)
        {
        case FUNC_INC:
            if (!InstanceInfo->scriptCapabilities.func_inc)
            {
                NotSupported = TRUE;
                ShowMessages("err, increment is not supported by the debuggee\n");
            }
            break;

        case FUNC_DEC:
            if (!InstanceInfo->scriptCapabilities.func_dec)
            {
                NotSupported = TRUE;
                ShowMessages("err, decrement is not supported by the debuggee\n");
            }
            break;

        case FUNC_OR:
            if (!InstanceInfo->scriptCapabilities.func_or)
            {
                NotSupported = TRUE;
                ShowMessages("err, OR is not supported by the debuggee\n");
            }
            break;

        case FUNC_XOR:
            if (!InstanceInfo->scriptCapabilities.func_xor)
            {
                NotSupported = TRUE;
                ShowMessages("err, XOR is not supported by the debuggee\n");
            }
            break;

        case FUNC_AND:
            if (!InstanceInfo->scriptCapabilities.func_and)
            {
                NotSupported = TRUE;
                ShowMessages("err, AND is not supported by the debuggee\n");
            }
            break;

        case FUNC_ASR:
            if (!InstanceInfo->scriptCapabilities.func_asr)
            {
                NotSupported = TRUE;
                ShowMessages("err, arithmetic shift right is not supported by the debuggee\n");
            }
            break;

        case FUNC_ASL:
            if (!InstanceInfo->scriptCapabilities.func_asl)
            {
                NotSupported = TRUE;
                ShowMessages("err, arithmetic shift left is not supported by the debuggee\n");
            }
            break;

        case FUNC_ADD:
            if (!InstanceInfo->scriptCapabilities.func_add)
            {
                NotSupported = TRUE;
                ShowMessages("err, addition is not supported by the debuggee\n");
            }
            break;

        case FUNC_SUB:
            if (!InstanceInfo->scriptCapabilities.func_sub)
            {
                NotSupported = TRUE;
                ShowMessages("err, subtraction is not supported by the debuggee\n");
            }
            break;

        case FUNC_MUL:
            if (!InstanceInfo->scriptCapabilities.func_mul)
            {
                NotSupported = TRUE;
                ShowMessages("err, multiplication is not supported by the debuggee\n");
            }
            break;

        case FUNC_DIV:
            if (!InstanceInfo->scriptCapabilities.func_div)
            {
                NotSupported = TRUE;
                ShowMessages("err, division is not supported by the debuggee\n");
            }
            break;

        case FUNC_MOD:
            if (!InstanceInfo->scriptCapabilities.func_mod)
            {
                NotSupported = TRUE;
                ShowMessages("err, modulus is not supported by the debuggee\n");
            }
            break;

        case FUNC_GT:
            if (!InstanceInfo->scriptCapabilities.func_gt)
            {
                NotSupported = TRUE;
                ShowMessages("err, greater than is not supported by the debuggee\n");
            }
            break;

        case FUNC_LT:
            if (!InstanceInfo->scriptCapabilities.func_lt)
            {
                NotSupported = TRUE;
                ShowMessages("err, less than is not supported by the debuggee\n");
            }
            break;

        case FUNC_EGT:
            if (!InstanceInfo->scriptCapabilities.func_egt)
            {
                NotSupported = TRUE;
                ShowMessages("err, greater than or equal to is not supported by the debuggee\n");
            }
            break;

        case FUNC_ELT:
            if (!InstanceInfo->scriptCapabilities.func_elt)
            {
                NotSupported = TRUE;
                ShowMessages("err, less than or equal to is not supported by the debuggee\n");
            }
            break;

        case FUNC_EQUAL:
            if (!InstanceInfo->scriptCapabilities.func_equal)
            {
                NotSupported = TRUE;
                ShowMessages("err, equal is not supported by the debuggee\n");
            }
            break;

        case FUNC_NEQ:
            if (!InstanceInfo->scriptCapabilities.func_neq)
            {
                NotSupported = TRUE;
                ShowMessages("err, not equal is not supported by the debuggee\n");
            }
            break;

        case FUNC_JMP:
            if (!InstanceInfo->scriptCapabilities.func_jmp)
            {
                NotSupported = TRUE;
                ShowMessages("err, jump is not supported by the debuggee\n");
            }
            break;

        case FUNC_JZ:
            if (!InstanceInfo->scriptCapabilities.func_jz)
            {
                NotSupported = TRUE;
                ShowMessages("err, jump if zero is not supported by the debuggee\n");
            }
            break;

        case FUNC_JNZ:
            if (!InstanceInfo->scriptCapabilities.func_jnz)
            {
                NotSupported = TRUE;
                ShowMessages("err, jump if not zero is not supported by the debuggee\n");
            }
            break;

        case FUNC_MOV:
            if (!InstanceInfo->scriptCapabilities.func_mov)
            {
                NotSupported = TRUE;
                ShowMessages("err, move is not supported by the debuggee\n");
            }
            break;

        case FUNC_PRINTF:
            if (!InstanceInfo->scriptCapabilities.func_printf)
            {
                NotSupported = TRUE;
                ShowMessages("err, printf is not supported by the debuggee\n");
            }
            break;

        default:

            NotSupported = TRUE;
            ShowMessages("err, undefined operator for hwdbg: %d (0x%x)\n",
                         SymbolArray[i].Type,
                         SymbolArray[i].Type);

            break;
        }
    }

    //
    // Script capabilities support this buffer
    //
    if (NotSupported)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
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

/**
 * @brief Function to compress the buffer
 *
 * @param Buffer
 * @param BufferLength
 * @param NewBufferSize
 *
 * @return BOOLEAN
 */
BOOLEAN
HwdbgInterpreterConvertSymbolToHwdbgShortSymbolBuffer(SYMBOL * SymbolBuffer,
                                                      size_t   SymbolBufferLength,
                                                      size_t * NewBufferSize)

{
    SIZE_T NumberOfSymbols = SymbolBufferLength / sizeof(SymbolBuffer[0]);

    *NewBufferSize = NumberOfSymbols * sizeof(HWDBG_SHORT_SYMBOL);

    //
    // Create a temporary buffer to hold the compressed data
    //
    HWDBG_SHORT_SYMBOL * HwdbgShortSymbolBuffer = (HWDBG_SHORT_SYMBOL *)malloc(*NewBufferSize);

    if (!HwdbgShortSymbolBuffer)
    {
        ShowMessages("err, could not allocate compression buffer\n");
        return FALSE;
    }

    //
    // Zeroing the short symbol buffer
    //
    RtlZeroMemory(HwdbgShortSymbolBuffer, *NewBufferSize);

    //
    // Filling the short symbol buffer from original buffer
    //
    for (size_t i = 0; i < NumberOfSymbols; i++)
    {
        //
        // Move the symbol buffer into a short symbol buffer
        //
        HwdbgShortSymbolBuffer[i].Type  = SymbolBuffer[i].Type;
        HwdbgShortSymbolBuffer[i].Value = SymbolBuffer[i].Value;
    }

    //
    // Copy the compressed data back to the original buffer
    //
    RtlZeroMemory(SymbolBuffer, SymbolBufferLength);
    memcpy(SymbolBuffer, HwdbgShortSymbolBuffer, *NewBufferSize);

    //
    // Free the temporary buffer
    //
    free(HwdbgShortSymbolBuffer);

    return TRUE;
}