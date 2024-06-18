/**
 * @file hwdbg-interpreter.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for the interpreter of hwdbg packets and requests
 * @details
 * @version 1.0
 * @date 2024-06-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				   Definitions                  //
//////////////////////////////////////////////////

/**
 * @brief Path to read the sample of the instance info
 *
 */
#define HWDBG_TEST_READ_INSTANCE_INFO_PATH "..\\..\\..\\..\\hwdbg\\sim\\hwdbg\\DebuggerModuleTestingBRAM\\bram_instance_info.txt"

/**
 * @brief Path to write the sample of the script buffer
 *
 */
#define HWDBG_TEST_WRITE_SCRIPT_BUFFER_PATH "..\\..\\..\\..\\hwdbg\\src\\test\\bram\\script_buffer.hex.txt"

/**
 * @brief Path to write the sample of the instance info requests
 *
 */
#define HWDBG_TEST_WRITE_INSTANCE_INFO_PATH "..\\..\\..\\..\\hwdbg\\src\\test\\bram\\instance_info.hex.txt"

//////////////////////////////////////////////////
//				    Functions                   //
//////////////////////////////////////////////////

BOOLEAN
HwdbgInterpretPacket(PVOID BufferReceived, UINT32 LengthReceived);

BOOLEAN
HwdbgInterpreterFillFileFromMemory(
    HWDBG_INSTANCE_INFORMATION * InstanceInfo,
    const TCHAR *                FileName,
    UINT32 *                     MemoryBuffer,
    size_t                       BufferSize,
    HWDBG_ACTION_ENUMS           RequestedAction);

BOOLEAN
HwdbgInterpreterFillMemoryFromFile(const TCHAR * FileName, UINT32 * MemoryBuffer, size_t BufferSize);

BOOLEAN
HwdbgInterpreterConvertSymbolToHwdbgShortSymbolBuffer(
    HWDBG_INSTANCE_INFORMATION * InstanceInfo,
    SYMBOL *                     SymbolBuffer,
    size_t                       SymbolBufferLength,
    UINT32                       NumberOfStages,
    HWDBG_SHORT_SYMBOL **        NewShortSymbolBuffer,
    size_t *                     NewBufferSize);

BOOLEAN
HwdbgInterpreterCompressBuffer(UINT64 * Buffer,
                               size_t   BufferLength,
                               UINT32   ScriptVariableLength,
                               UINT32   BramDataWidth,
                               size_t * NewBufferSize,
                               size_t * NumberOfBytesPerChunk);

VOID
HwdbgInterpreterShowScriptCapabilities(HWDBG_INSTANCE_INFORMATION * InstanceInfo);

BOOLEAN
HwdbgInterpreterCheckScriptBufferWithScriptCapabilities(HWDBG_INSTANCE_INFORMATION * InstanceInfo,
                                                        PVOID                        ScriptBuffer,
                                                        UINT32                       CountOfScriptSymbolChunks,
                                                        UINT32 *                     NumberOfStages,
                                                        UINT32 *                     NumberOfOperands);

BOOLEAN
HwdbgInterpreterSendPacketAndBufferToHwdbg(HWDBG_INSTANCE_INFORMATION * InstanceInfo,
                                           const TCHAR *                FileName,
                                           DEBUGGER_REMOTE_PACKET_TYPE  PacketType,
                                           HWDBG_ACTION_ENUMS           RequestedAction,
                                           CHAR *                       Buffer,
                                           UINT32                       BufferLength);

BOOLEAN
HwdbgInterpreterSendScriptPacket(HWDBG_INSTANCE_INFORMATION * InstanceInfo,
                                 const TCHAR *                FileName,
                                 UINT32                       NumberOfSymbols,
                                 HWDBG_SHORT_SYMBOL *         Buffer,
                                 UINT32                       BufferLength);
