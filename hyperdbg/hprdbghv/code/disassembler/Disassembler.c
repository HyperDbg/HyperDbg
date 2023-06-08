/**
 * @file Diassembler.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Disassembler functions in kernel-mode
 * @details
 *
 * @version 0.3
 * @date 2023-06-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Disassembler show the instructions
 * @details This function should not be called from VMX-root mode
 *
 * @param Address
 * @param Length
 * @param Is32Bit
 *
 * @return BOOLEAN
 */
BOOLEAN
DisassemblerShowInstructionsInVmxNonRootMode(PVOID Address, UINT32 Length, BOOLEAN Is32Bit)
{
    ZydisDecoder            decoder;
    ZydisFormatter          formatter;
    SIZE_T                  readOffset = 0;
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand     operands[ZYDIS_MAX_OPERAND_COUNT];
    ZyanStatus              status;
    CHAR                    printBuffer[128];

    PAGED_CODE();

    if (ZydisGetVersion() != ZYDIS_VERSION)
    {
        LogError("Err, invalid zydis version");
        return FALSE;
    }

    //
    // Initialize Zydis decoder
    //
    if (Is32Bit)
    {
        if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32)))
        {
            return FALSE;
        }
    }
    else
    {
        if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
        {
            return FALSE;
        }
    }

    //
    // Initialize Zydis formatter
    //
    if (!ZYAN_SUCCESS(ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL)))
    {
        return FALSE;
    }

    //
    // Start the decode loop
    //
    while ((status = ZydisDecoderDecodeFull(&decoder,
                                            (PVOID)((UINT64)Address + readOffset),
                                            Length - readOffset,
                                            &instruction,
                                            operands)) != ZYDIS_STATUS_NO_MORE_DATA)
    {
        NT_ASSERT(ZYAN_SUCCESS(status));
        if (!ZYAN_SUCCESS(status))
        {
            readOffset++;
            continue;
        }

        // Format and print the instruction
        const ZyanU64 instrAddress = (ZyanU64)((UINT64)Address + readOffset);
        ZydisFormatterFormatInstruction(
            &formatter,
            &instruction,
            operands,
            instruction.operand_count_visible,
            printBuffer,
            sizeof(printBuffer),
            instrAddress,
            NULL);

        LogInfo("+%-4X 0x%-16llX\t\t%hs\n", (ULONG)readOffset, instrAddress, printBuffer);

        readOffset += instruction.length;
    }

    //
    // return an error status so that the driver does not have to be unloaded after running
    //
    return TRUE;
}

/**
 * @brief Disassembler show only one instruction
 * @details This function should not be directly called from VMX-root mode
 * if the caller is sure that the target buffer is safe to be access, then it's okay
 *
 * @param Address
 * @param Is32Bit
 *
 * @return BOOLEAN
 */
BOOLEAN
DisassemblerShowOneInstructionInVmxNonRootMode(PVOID Address, BOOLEAN Is32Bit)
{
    ZydisDecoder            decoder;
    ZydisFormatter          formatter;
    SIZE_T                  readOffset = 0;
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand     operands[ZYDIS_MAX_OPERAND_COUNT];
    ZyanStatus              status;
    CHAR                    printBuffer[128];

    if (ZydisGetVersion() != ZYDIS_VERSION)
    {
        LogError("Err, invalid zydis version");
        return FALSE;
    }

    //
    // Initialize Zydis decoder
    //
    if (Is32Bit)
    {
        if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32)))
        {
            return FALSE;
        }
    }
    else
    {
        if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
        {
            return FALSE;
        }
    }

    //
    // Initialize Zydis formatter
    //
    if (!ZYAN_SUCCESS(ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL)))
    {
        return FALSE;
    }

    //
    // Start the decode loop
    //
    while ((status = ZydisDecoderDecodeFull(&decoder,
                                            (PVOID)((UINT64)Address + readOffset),
                                            MAXIMUM_INSTR_SIZE - readOffset,
                                            &instruction,
                                            operands)) != ZYDIS_STATUS_NO_MORE_DATA)
    {
        NT_ASSERT(ZYAN_SUCCESS(status));
        if (!ZYAN_SUCCESS(status))
        {
            readOffset++;
            continue;
        }

        // Format and print the instruction
        const ZyanU64 instrAddress = (ZyanU64)((UINT64)Address + readOffset);
        ZydisFormatterFormatInstruction(
            &formatter,
            &instruction,
            operands,
            instruction.operand_count_visible,
            printBuffer,
            sizeof(printBuffer),
            instrAddress,
            NULL);

        // LogInfo("+%-4X 0x%-16llX\t\t%hs\n", (ULONG)readOffset, instrAddress, printBuffer);
        Log("%llx\t\t%hs\n", Address, printBuffer);

        // readOffset += instruction.length;

        //
        // Only one instruction is enough
        //
        break;
    }

    //
    // return an error status so that the driver does not have to be unloaded after running
    //
    return TRUE;
}

/**
 * @brief Disassembler length disassemble engine
 * @details if you want to call it directly, shouldn't not be in VMX-root mode, otherwise, you can
 * call DisassemblerLengthDisassembleEngineInVmxRootOnTargetProcess to access memory safely
 *
 * @param Address
 * @param Is32Bit
 *
 * @return UINT32
 */
UINT32
DisassemblerLengthDisassembleEngine(PVOID Address, BOOLEAN Is32Bit)
{
    ZydisDecoder            decoder;
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand     operands[ZYDIS_MAX_OPERAND_COUNT];
    ZyanStatus              status;
    CHAR                    printBuffer[128];

    if (ZydisGetVersion() != ZYDIS_VERSION)
    {
        LogError("Err, invalid zydis version");
        return NULL;
    }

    //
    // Initialize Zydis decoder
    //
    if (Is32Bit)
    {
        if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32)))
        {
            return NULL;
        }
    }
    else
    {
        if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
        {
            return NULL;
        }
    }

    //
    // Start the decode loop
    //
    while ((status = ZydisDecoderDecodeFull(&decoder,
                                            (PVOID)((UINT64)Address),
                                            MAXIMUM_INSTR_SIZE,
                                            &instruction,
                                            operands)) != ZYDIS_STATUS_NO_MORE_DATA)
    {
        NT_ASSERT(ZYAN_SUCCESS(status));

        if (!ZYAN_SUCCESS(status))
        {
            //
            // Probably invalid instruction
            //
            return NULL;
        }

        //
        // This is a length disassembler, so we just return the length
        //
        return instruction.length;
    }

    //
    // return an error status
    //
    return NULL;
}

/**
 * @brief Disassembler length disassembler engine
 * @details Should be called in VMX-root mode
 *
 * @param Address
 * @param Is32Bit
 *
 * @return UINT32
 */
UINT32
DisassemblerLengthDisassembleEngineInVmxRootOnTargetProcess(PVOID Address, BOOLEAN Is32Bit)
{
    BYTE   SafeMemoryToRead[MAXIMUM_INSTR_SIZE] = {0};
    UINT64 SizeOfSafeBufferToRead               = 0;

    //
    // Read the maximum number of instruction that is valid to be read in the
    // target address
    //
    SizeOfSafeBufferToRead = CheckAddressMaximumInstructionLength(Address);

    //
    // Find the current instruction
    //
    MemoryMapperReadMemorySafeOnTargetProcess(Address,
                                              SafeMemoryToRead,
                                              SizeOfSafeBufferToRead);

    return DisassemblerLengthDisassembleEngine(SafeMemoryToRead, Is32Bit);
}

/**
 * @brief Shows the disassembly of only one instruction
 * @details Should be called in VMX-root mode
 *
 * @param Address
 * @param Is32Bit
 *
 * @return UINT32
 */
UINT32
DisassemblerShowOneInstructionInVmxRootMode(PVOID Address, BOOLEAN Is32Bit)
{
    BYTE   SafeMemoryToRead[MAXIMUM_INSTR_SIZE] = {0};
    UINT64 SizeOfSafeBufferToRead               = 0;

    //
    // Read the maximum number of instruction that is valid to be read in the
    // target address
    //
    SizeOfSafeBufferToRead = CheckAddressMaximumInstructionLength(Address);

    //
    // Find the current instruction
    //
    MemoryMapperReadMemorySafeOnTargetProcess(Address,
                                              SafeMemoryToRead,
                                              SizeOfSafeBufferToRead);

    return DisassemblerShowOneInstructionInVmxNonRootMode(SafeMemoryToRead, Is32Bit);
}
