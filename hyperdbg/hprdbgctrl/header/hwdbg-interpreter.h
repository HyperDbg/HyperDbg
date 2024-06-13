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
#define HWDBG_TEST_INSTANCE_INFO_PATH "..\\..\\..\\..\\hwdbg\\sim\\hwdbg\\DebuggerModuleTestingBRAM\\bram_instance_info.txt"

/**
 * @brief Path to write the sample of the script buffer
 *
 */
#define HWDBG_TEST_SCRIPT_BUFFER_PATH "..\\..\\..\\..\\hwdbg\\src\\test\\bram\\script_buffer.hex.txt"

//////////////////////////////////////////////////
//				    Functions                   //
//////////////////////////////////////////////////

BOOLEAN
HwdbgInterpretPacket(PVOID BufferReceived, UINT32 LengthReceived);

BOOLEAN
HwdbgInterpreterFillFileFromMemory(const TCHAR * FileName, UINT32 * MemoryBuffer, size_t BufferSize);

BOOLEAN
HwdbgInterpreterFillMemoryFromFile(const TCHAR * FileName, UINT32 * MemoryBuffer, size_t BufferSize);

BOOLEAN
HwdbgInterpreterConvertSymbolToHwdbgShortSymbolBuffer(SYMBOL * SymbolBuffer,
                                                      size_t   SymbolBufferLength,
                                                      size_t * NewBufferSize);

BOOLEAN
HwdbgInterpreterCompressBuffer(UINT64 * Buffer,
                               size_t   BufferLength,
                               int      CompressBitSize,
                               size_t * NewBufferSize,
                               size_t * NumberOfBytesPerChunk);

VOID
HwdbgInterpreterShowScriptCapabilities(HWDBG_INSTANCE_INFORMATION * InstanceInfo);

BOOLEAN
HwdbgInterpreterCheckScriptBufferWithScriptCapabilities(HWDBG_INSTANCE_INFORMATION * InstanceInfo,
                                                        PVOID                        ScriptBuffer,
                                                        UINT32                       CountOfScriptSymbolChunks);