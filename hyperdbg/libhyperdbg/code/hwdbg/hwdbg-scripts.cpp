/**
 * @file hwdbg-scripts.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Hardware scripts for hwdbg
 * @details
 * @version 0.11
 * @date 2024-09-30
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

/**
 * @brief Print the actual script
 *
 * @param ScriptBuffer
 * @param ScriptBufferSize
 *
 * @return VOID
 */
VOID
HwdbgScriptPrintScriptBuffer(CHAR * ScriptBuffer, UINT32 ScriptBufferSize)
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
HwdbgScriptCompressScriptBuffer(HWDBG_INSTANCE_INFORMATION * InstanceInfo,
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
 * @brief Print the hwdbg script buffer and hardware details
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
HwdbgScriptPrintFinalScriptBufferAndHardwareDetails(HWDBG_INSTANCE_INFORMATION * InstanceInfo,
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
HwdbgScriptWriteScriptConfigurationPacketIntoFile(HWDBG_INSTANCE_INFORMATION * InstanceInfo,
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
        HwdbgScriptSendScriptPacket(
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
 * @brief Create hwdbg script
 * @param ScriptBuffer
 * @param ScriptBufferSize
 * @param HardwareScriptFilePathToSave
 *
 * @return BOOLEAN
 */
BOOLEAN
HwdbgScriptCreateHwdbgScript(CHAR *        ScriptBuffer,
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
    if (!HwdbgScriptCompressScriptBuffer(&g_HwdbgInstanceInfo,
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
    HwdbgScriptPrintFinalScriptBufferAndHardwareDetails(&g_HwdbgInstanceInfo,
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
    if (!HwdbgScriptWriteScriptConfigurationPacketIntoFile(&g_HwdbgInstanceInfo,
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
HwdbgScriptGetScriptBufferFromRawString(string   ScriptString,
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
 * @brief Sends a HyperDbg (hwdbg) script packet to the hwdbg
 *
 * @param InstanceInfo
 * @param FileName
 * @param Buffer
 * @param BufferLength
 *
 * @return BOOLEAN
 */
BOOLEAN
HwdbgScriptSendScriptPacket(HWDBG_INSTANCE_INFORMATION * InstanceInfo,
                            const TCHAR *                FileName,
                            UINT32                       NumberOfSymbols,
                            HWDBG_SHORT_SYMBOL *         Buffer,
                            UINT32                       BufferLength)
{
    HWDBG_SCRIPT_BUFFER ScriptBuffer = {0};
    BOOLEAN             Result       = FALSE;

    //
    // Make the packet's structure
    //
    ScriptBuffer.scriptNumberOfSymbols = NumberOfSymbols;

    //
    // Allocate a buffer for storing the header packet + buffer (if not empty)
    //
    CHAR * FinalBuffer = (CHAR *)malloc(BufferLength + sizeof(HWDBG_SCRIPT_BUFFER));

    if (!FinalBuffer)
    {
        return FALSE;
    }

    RtlZeroMemory(FinalBuffer, BufferLength + sizeof(HWDBG_SCRIPT_BUFFER));

    //
    // Copy the packet into the FinalBuffer
    //
    memcpy(FinalBuffer, &ScriptBuffer, sizeof(HWDBG_SCRIPT_BUFFER));

    //
    // Copy the buffer (if available) into the FinalBuffer
    //
    if (Buffer != NULL)
    {
        memcpy(FinalBuffer + sizeof(HWDBG_SCRIPT_BUFFER), Buffer, BufferLength);
    }

    //
    // Here we would send FinalBuffer to the hardware debugger
    //
    Result = HwdbgInterpreterSendPacketAndBufferToHwdbg(
        InstanceInfo,
        FileName,
        DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_HARDWARE_LEVEL,
        hwdbgActionConfigureScriptBuffer,
        FinalBuffer,
        BufferLength + sizeof(HWDBG_SCRIPT_BUFFER));

    //
    // Free the allocated memory after use
    //
    free(FinalBuffer);

    return Result;
}
