/***************************************************************************************************
  Zyan Disassembler Library (Zydis)
  Original Author : Florian Bernd
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
***************************************************************************************************/

/**
 * @file
 * @brief   Demonstrates basic hooking functionality of the `ZydisFormatter`
 * class by implementing a custom symbol-resolver.
 */

#include "pch.h"

#include "Zycore/Format.h"
#include "Zycore/LibC.h"
#include "Zydis/Zydis.h"

#pragma comment(lib, "Zydis.lib")
#pragma comment(lib, "Zycore.lib")

//
// Global Variables
//
extern UINT32                                       g_DisassemblerSyntax;
extern std::map<UINT64, LOCAL_FUNCTION_DESCRIPTION> g_DisassemblerSymbolMap;
extern BOOLEAN                                      g_AddressConversion;

/**
 * @brief Defines the `ZydisSymbol` struct.
 */
typedef struct ZydisSymbol_
{
    /**
     * @brief The symbol address.
     */
    ZyanU64 address;
    /**
     * @brief The symbol name.
     */
    const char * name;
} ZydisSymbol;

ZydisFormatterFunc default_print_address_absolute;

/**
 * @brief Print addresses
 *
 * @param formatter
 * @param buffer
 * @param context
 * @return ZyanStatus
 */
static ZyanStatus
ZydisFormatterPrintAddressAbsolute(const ZydisFormatter *  formatter,
                                   ZydisFormatterBuffer *  buffer,
                                   ZydisFormatterContext * context)
{
    ZyanU64                                                address;
    std::map<UINT64, LOCAL_FUNCTION_DESCRIPTION>::iterator Iterate;

    ZYAN_CHECK(ZydisCalcAbsoluteAddress(context->instruction, context->operand, context->runtime_address, &address));

    //
    // Apply addressconversion of settings here
    //
    if (g_AddressConversion)
    {
        //
        // Check to find the symbol of address
        //
        Iterate = g_DisassemblerSymbolMap.find(address);

        if (Iterate != g_DisassemblerSymbolMap.end())
        {
            ZYAN_CHECK(ZydisFormatterBufferAppend(buffer, ZYDIS_TOKEN_SYMBOL));
            ZyanString * string;
            ZYAN_CHECK(ZydisFormatterBufferGetString(buffer, &string));
            return ZyanStringAppendFormat(string,
                                          "<%s (%s)>",
                                          Iterate->second.ObjectName.c_str(),
                                          SeparateTo64BitValue(Iterate->first).c_str());
        }
    }

    return default_print_address_absolute(formatter, buffer, context);
}

/**
 * @brief Disassemble a user-mode buffer
 *
 * @param decoder
 * @param runtime_address
 * @param data
 * @param length
 * @param maximum_instr
 * @param is_x86_64
 * @param show_of_branch_is_taken
 * @param rflags just used in the case show_of_branch_is_taken is true
 */
VOID
DisassembleBuffer(ZydisDecoder * decoder,
                  ZyanU64        runtime_address,
                  ZyanU8 *       data,
                  ZyanUSize      length,
                  uint32_t       maximum_instr,
                  BOOLEAN        is_x86_64,
                  BOOLEAN        show_of_branch_is_taken,
                  PRFLAGS        rflags)
{
    ZydisFormatter formatter;
    int            instr_decoded   = 0;
    UINT64         UsedBaseAddress = NULL;

    if (g_DisassemblerSyntax == 1)
    {
        ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);
    }
    else if (g_DisassemblerSyntax == 2)
    {
        ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_ATT);
    }
    else if (g_DisassemblerSyntax == 3)
    {
        ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL_MASM);
    }
    else
    {
        ShowMessages("err, in selecting disassembler syntax\n");
        return;
    }

    ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_FORCE_SEGMENT, ZYAN_TRUE);
    ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_FORCE_SIZE, ZYAN_TRUE);

    //
    // Replace the `ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS` function that formats
    // the absolute addresses
    //
    default_print_address_absolute =
        (ZydisFormatterFunc)&ZydisFormatterPrintAddressAbsolute;
    ZydisFormatterSetHook(&formatter, ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS, (const void **)&default_print_address_absolute);

    ZydisDecodedOperand     operands[ZYDIS_MAX_OPERAND_COUNT];
    ZydisDecodedInstruction instruction;
    char                    buffer[256];

    while (ZYAN_SUCCESS(ZydisDecoderDecodeFull(decoder, data, length, &instruction, operands)))
    {
        //
        // Apply addressconversion of settings here
        //
        if (g_AddressConversion)
        {
            //
            // Showing function names here
            //
            if (SymbolShowFunctionNameBasedOnAddress(runtime_address, &UsedBaseAddress))
            {
                //
                // The symbol address is showed
                //
                ShowMessages(":\n");
            }
        }

        // ZYAN_PRINTF("%016" PRIX64 "  ", runtime_address);
        ShowMessages("%s   ", SeparateTo64BitValue(runtime_address).c_str());
        //
        // We have to pass a `runtime_address` different to
        // `ZYDIS_RUNTIME_ADDRESS_NONE` to enable printing of absolute addresses
        //
        ZydisFormatterFormatInstruction(&formatter, &instruction, operands, instruction.operand_count_visible, &buffer[0], sizeof(buffer), runtime_address, ZYAN_NULL);

        //
        // Show the memory for this instruction
        //
        for (size_t i = 0; i < instruction.length; i++)
        {
            ZyanU8 MemoryContent = data[i];
            ShowMessages(" %02X", MemoryContent);
        }

        ShowMessages("\n\ninstruction.operand_width: %d\n\n", instruction.operand_width);

        //
        // Add padding (we assume that each instruction should be at least 10 bytes)
        //
#define PaddingLength 12
        if (instruction.length < PaddingLength)
        {
            for (size_t i = 0; i < PaddingLength - instruction.length; i++)
            {
                ShowMessages("   ");
            }
        }

        //
        // Check whether we should show the result of conditional branches or not
        //
        if (show_of_branch_is_taken)
        {
            //
            // Get the result of conditional jump, we re-format the instruction
            // here because the user might have changed the configuration of Zydis
            // using the "settings" command so it's better to re-format with default
            // configuration
            //
            RFLAGS TempRflags = {0};
            TempRflags.AsUInt = rflags->AsUInt;
            DEBUGGER_CONDITIONAL_JUMP_STATUS ResultOfCondJmp =
                HyperDbgIsConditionalJumpTaken(data, length, TempRflags, is_x86_64);

            if (ResultOfCondJmp == DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN)
            {
                ShowMessages(" %s [taken]\n", &buffer[0]);
            }
            else if (ResultOfCondJmp ==
                     DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN)
            {
                ShowMessages(" %s [not taken]\n", &buffer[0]);
            }
            else
            {
                //
                // It's either not a conditional jump or an error occurred
                //
                ShowMessages(" %s\n", &buffer[0]);
            }
        }
        else
        {
            //
            // Show regular instruction
            //
            ShowMessages(" %s\n", &buffer[0]);
        }

        data += instruction.length;
        length -= instruction.length;
        runtime_address += instruction.length;
        instr_decoded++;

        if (instr_decoded == maximum_instr)
        {
            return;
        }
    }
}

/**
 * @brief Zydis test
 *
 * @return int
 */
int
ZydisTest()
{
    if (ZydisGetVersion() != ZYDIS_VERSION)
    {
        fputs("Invalid Zydis version\n", ZYAN_STDERR);
        return EXIT_FAILURE;
    }

    ZyanU8 data[] = {
        0x48,
        0x8B,
        0x05,
        0x39,
        0x00,
        0x13,
        0x00, // mov rax, qword ptr ds:[<SomeModule.SomeData>]
        0x50, // push rax
        0xFF,
        0x15,
        0xF2,
        0x10,
        0x00,
        0x00, // call qword ptr ds:[<SomeModule.SomeFunction>]
        0x85,
        0xC0, // test eax, eax
        0x0F,
        0x84,
        0x00,
        0x00,
        0x00,
        0x00, // jz 0x007FFFFFFF400016
        0xE9,
        0xE5,
        0x0F,
        0x00,
        0x00 // jmp <SomeModule.EntryPoint>
    };

    ZydisDecoder decoder;
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

    DisassembleBuffer(&decoder, 0x007FFFFFFF400000, &data[0], sizeof(data), 0xffffffff, TRUE, FALSE, NULL);

    return 0;
}

/**
 * @brief Disassemble x64 assemblies
 *
 * @param BufferToDisassemble buffer to disassemble
 * @param BaseAddress the base address of assembly
 * @param Size size of buffer
 * @param MaximumInstrDecoded maximum instructions to decode, 0 means all
 * possible
 * @param ShowBranchIsTakenOrNot on conditional jumps shows whether jumps is
 * taken or not
 * @param Rflags in the case ShowBranchIsTakenOrNot is true, we use this
 * variable to show the result of jump
 *
 * @return int
 */
int
HyperDbgDisassembler64(unsigned char * BufferToDisassemble,
                       UINT64          BaseAddress,
                       UINT64          Size,
                       UINT32          MaximumInstrDecoded,
                       BOOLEAN         ShowBranchIsTakenOrNot,
                       PRFLAGS         Rflags)
{
    if (ZydisGetVersion() != ZYDIS_VERSION)
    {
        fputs("Invalid Zydis version\n", ZYAN_STDERR);
        return EXIT_FAILURE;
    }

    ZydisDecoder decoder;
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

    //
    // Disassembling buffer
    //
    DisassembleBuffer(&decoder, BaseAddress, &BufferToDisassemble[0], Size, MaximumInstrDecoded, TRUE, ShowBranchIsTakenOrNot, Rflags);

    return 0;
}

/**
 * @brief Disassemble 32 bit assemblies
 *
 * @param BufferToDisassemble buffer to disassemble
 * @param BaseAddress the base address of assembly
 * @param Size size of buffer
 * @param MaximumInstrDecoded maximum instructions to decode, 0 means all
 * possible
 * @param ShowBranchIsTakenOrNot on conditional jumps shows whether jumps is
 * taken or not
 * @param Rflags in the case ShowBranchIsTakenOrNot is true, we use this
 * variable to show the result of jump
 *
 * @return int
 */
int
HyperDbgDisassembler32(unsigned char * BufferToDisassemble,
                       UINT64          BaseAddress,
                       UINT64          Size,
                       UINT32          MaximumInstrDecoded,
                       BOOLEAN         ShowBranchIsTakenOrNot,
                       PRFLAGS         Rflags)
{
    if (ZydisGetVersion() != ZYDIS_VERSION)
    {
        fputs("Invalid Zydis version\n", ZYAN_STDERR);
        return EXIT_FAILURE;
    }

    ZydisDecoder decoder;
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32);

    //
    // Disassembling buffer
    //
    DisassembleBuffer(&decoder, (UINT32)BaseAddress, &BufferToDisassemble[0], Size, MaximumInstrDecoded, FALSE, ShowBranchIsTakenOrNot, Rflags);

    return 0;
}

/**
 * @brief Check whether the jump is taken or not taken (in debugger)
 * @details the implementation of this function derived from the
 * table in this site : http://www.unixwiz.net/techtips/x86-jumps.html
 *
 * @param BufferToDisassemble Current Bytes of assembly
 * @param BuffLength Length of buffer
 * @param Rflags The kernel's current RFLAG
 * @param Isx86_64 Whether it's an x86 or x64
 *
 * @return DEBUGGER_NEXT_INSTRUCTION_FINDER_STATUS
 */
DEBUGGER_CONDITIONAL_JUMP_STATUS
HyperDbgIsConditionalJumpTaken(unsigned char * BufferToDisassemble,
                               UINT64          BuffLength,
                               RFLAGS          Rflags,
                               BOOLEAN         Isx86_64)
{
    ZydisDecoder            decoder;
    ZydisFormatter          formatter;
    ZydisDecodedOperand     operands[ZYDIS_MAX_OPERAND_COUNT];
    UINT64                  CurrentRip    = 0;
    int                     instr_decoded = 0;
    ZydisDecodedInstruction instruction;
    UINT32                  MaximumInstrDecoded = 1;

    if (ZydisGetVersion() != ZYDIS_VERSION)
    {
        ShowMessages("invalid Zydis version\n");
        return DEBUGGER_CONDITIONAL_JUMP_STATUS_ERROR;
    }

    if (Isx86_64)
    {
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
    }
    else
    {
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32);
    }

    ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

    ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_FORCE_SEGMENT, ZYAN_TRUE);
    ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_FORCE_SIZE, ZYAN_TRUE);

    //
    // Replace the `ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS` function that
    // formats the absolute addresses
    //
    default_print_address_absolute = (ZydisFormatterFunc)&ZydisFormatterPrintAddressAbsolute;
    ZydisFormatterSetHook(&formatter, ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS, (const void **)&default_print_address_absolute);

    while (ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, BufferToDisassemble, BuffLength, &instruction, operands)))
    {
        //
        // We have to pass a `runtime_address` different to
        // `ZYDIS_RUNTIME_ADDRESS_NONE` to enable printing of absolute addresses
        //

        // ZydisFormatterFormatInstruction(&formatter, &instruction, operands, instruction.operand_count_visible, &buffer[0], sizeof(buffer), (ZyanU64)CurrentRip, ZYAN_NULL);

        switch (instruction.mnemonic)
        {
        case ZydisMnemonic::ZYDIS_MNEMONIC_JO:

            //
            // Jump if overflow (jo)
            //
            if (Rflags.OverflowFlag)
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN;
            else
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN;

            break;

        case ZydisMnemonic::ZYDIS_MNEMONIC_JNO:

            //
            // Jump if not overflow (jno)
            //
            if (!Rflags.OverflowFlag)
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN;
            else
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN;

            break;

        case ZydisMnemonic::ZYDIS_MNEMONIC_JS:

            //
            // Jump if sign
            //
            if (Rflags.SignFlag)
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN;
            else
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN;

            break;

        case ZydisMnemonic::ZYDIS_MNEMONIC_JNS:

            //
            // Jump if not sign
            //
            if (!Rflags.SignFlag)
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN;
            else
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN;

            break;

        case ZydisMnemonic::ZYDIS_MNEMONIC_JZ:

            //
            // Jump if equal (je),
            // Jump if zero (jz)
            //
            if (Rflags.ZeroFlag)
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN;
            else
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN;

            break;

        case ZydisMnemonic::ZYDIS_MNEMONIC_JNZ:

            //
            // Jump if not equal (jne),
            // Jump if not zero (jnz)
            //
            if (!Rflags.ZeroFlag)
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN;
            else
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN;

            break;

        case ZydisMnemonic::ZYDIS_MNEMONIC_JB:

            //
            // Jump if below (jb),
            // Jump if not above or equal (jnae),
            // Jump if carry (jc)
            //

            //
            // This jump is unsigned
            //

            if (Rflags.CarryFlag)
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN;
            else
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN;

            break;

        case ZydisMnemonic::ZYDIS_MNEMONIC_JNB:

            //
            // Jump if not below (jnb),
            // Jump if above or equal (jae),
            // Jump if not carry (jnc)
            //

            //
            // This jump is unsigned
            //

            if (!Rflags.CarryFlag)
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN;
            else
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN;

            break;

        case ZydisMnemonic::ZYDIS_MNEMONIC_JBE:

            //
            // Jump if below or equal (jbe),
            // Jump if not above (jna)
            //

            //
            // This jump is unsigned
            //

            if (Rflags.CarryFlag || Rflags.ZeroFlag)
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN;
            else
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN;

            break;

        case ZydisMnemonic::ZYDIS_MNEMONIC_JNBE:

            //
            // Jump if above (ja),
            // Jump if not below or equal (jnbe)
            //

            //
            // This jump is unsigned
            //

            if (!Rflags.CarryFlag && !Rflags.ZeroFlag)
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN;
            else
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN;

            break;

        case ZydisMnemonic::ZYDIS_MNEMONIC_JL:

            //
            // Jump if less (jl),
            // Jump if not greater or equal (jnge)
            //

            //
            // This jump is signed
            //

            if (Rflags.SignFlag != Rflags.OverflowFlag)
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN;
            else
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN;

            break;

        case ZydisMnemonic::ZYDIS_MNEMONIC_JNL:

            //
            // Jump if greater or equal (jge),
            // Jump if not less (jnl)
            //

            //
            // This jump is signed
            //

            if (Rflags.SignFlag == Rflags.OverflowFlag)
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN;
            else
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN;

            break;

        case ZydisMnemonic::ZYDIS_MNEMONIC_JLE:

            //
            // Jump if less or equal (jle),
            // Jump if not greater (jng)
            //

            //
            // This jump is signed
            //

            if (Rflags.ZeroFlag || Rflags.SignFlag != Rflags.OverflowFlag)
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN;
            else
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN;

            break;

        case ZydisMnemonic::ZYDIS_MNEMONIC_JNLE:

            //
            // Jump if greater (jg),
            // Jump if not less or equal (jnle)
            //

            //
            // This jump is signed
            //

            if (!Rflags.ZeroFlag && Rflags.SignFlag == Rflags.OverflowFlag)
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN;
            else
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN;

            break;

        case ZydisMnemonic::ZYDIS_MNEMONIC_JP:

            //
            // Jump if parity (jp),
            // Jump if parity even (jpe)
            //

            if (Rflags.ParityFlag)
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN;
            else
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN;

            break;

        case ZydisMnemonic::ZYDIS_MNEMONIC_JNP:

            //
            // Jump if not parity (jnp),
            // Jump if parity odd (jpo)
            //

            if (!Rflags.ParityFlag)
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_TAKEN;
            else
                return DEBUGGER_CONDITIONAL_JUMP_STATUS_JUMP_IS_NOT_TAKEN;

            break;

        case ZydisMnemonic::ZYDIS_MNEMONIC_JCXZ:
        case ZydisMnemonic::ZYDIS_MNEMONIC_JECXZ:

            //
            // Jump if %CX register is 0 (jcxz),
            // Jump if% ECX register is 0 (jecxz)
            //

            //
            // Actually this instruction are rarely used
            // but if we want to support these instructions then we
            // should read ecx and cx each time in the debuggee,
            // so it's better to just ignore it as a non-conditional
            // jump
            //
            return DEBUGGER_CONDITIONAL_JUMP_STATUS_NOT_CONDITIONAL_JUMP;

        default:

            //
            // It's not a jump
            //
            return DEBUGGER_CONDITIONAL_JUMP_STATUS_NOT_CONDITIONAL_JUMP;
            break;
        }

        return DEBUGGER_CONDITIONAL_JUMP_STATUS_ERROR;
    }

    //
    // Should not reach here
    //
    return DEBUGGER_CONDITIONAL_JUMP_STATUS_ERROR;
}

/**
 * @brief Check whether the current instruction is a 'call' or not
 *
 * @param BufferToDisassemble Current Bytes of assembly
 * @param BuffLength Length of buffer
 * @param Isx86_64 Whether it's an x86 or x64
 * @param CallLength Length of call (if return value is TRUE)
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgCheckWhetherTheCurrentInstructionIsCall(
    unsigned char * BufferToDisassemble,
    UINT64          BuffLength,
    BOOLEAN         Isx86_64,
    PUINT32         CallLength)
{
    ZydisDecoder            decoder;
    ZydisFormatter          formatter;
    ZydisDecodedOperand     operands[ZYDIS_MAX_OPERAND_COUNT];
    UINT64                  CurrentRip    = 0;
    int                     instr_decoded = 0;
    ZydisDecodedInstruction instruction;
    char                    buffer[256];
    UINT32                  MaximumInstrDecoded = 1;

    //
    // Default length
    //
    *CallLength = 0;

    if (ZydisGetVersion() != ZYDIS_VERSION)
    {
        ShowMessages("invalid zydis version\n");
        return DEBUGGER_CONDITIONAL_JUMP_STATUS_ERROR;
    }

    if (Isx86_64)
    {
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
    }
    else
    {
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32);
    }

    ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

    ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_FORCE_SEGMENT, ZYAN_TRUE);
    ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_FORCE_SIZE, ZYAN_TRUE);

    //
    // Replace the `ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS` function that
    // formats the absolute addresses
    //
    default_print_address_absolute =
        (ZydisFormatterFunc)&ZydisFormatterPrintAddressAbsolute;
    ZydisFormatterSetHook(&formatter, ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS, (const void **)&default_print_address_absolute);

    while (ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, BufferToDisassemble, BuffLength, &instruction, operands)))
    {
        //
        // We have to pass a `runtime_address` different to
        // `ZYDIS_RUNTIME_ADDRESS_NONE` to enable printing of absolute addresses
        //

        ZydisFormatterFormatInstruction(&formatter, &instruction, operands, instruction.operand_count_visible, &buffer[0], sizeof(buffer), (ZyanU64)CurrentRip, ZYAN_NULL);

        if (instruction.mnemonic == ZydisMnemonic::ZYDIS_MNEMONIC_CALL)
        {
            //
            // It's a call
            //

            //
            // Log call
            //
            // ShowMessages("call length : 0x%x\n", instruction.length);

            //
            // Set the length
            //
            *CallLength = instruction.length;

            return TRUE;
        }
        else
        {
            //
            // It's not call
            //
            return FALSE;
        }
    }

    //
    // Should not reach here
    //
    return FALSE;
}

/**
 * @brief Length Disassembler engine based on Zydis
 *
 * @param BufferToDisassemble Current Bytes of assembly
 * @param BuffLength Length of buffer
 * @param Isx86_64 Whether it's an x86 or x64
 * @param Length of call (if return value is TRUE)
 *
 * @return UINT32
 */
UINT32
HyperDbgLengthDisassemblerEngine(
    unsigned char * BufferToDisassemble,
    UINT64          BuffLength,
    BOOLEAN         Isx86_64)
{
    ZydisDecoder            decoder;
    ZydisFormatter          formatter;
    ZydisDecodedOperand     operands[ZYDIS_MAX_OPERAND_COUNT];
    UINT64                  CurrentRip    = 0;
    int                     instr_decoded = 0;
    ZydisDecodedInstruction instruction;
    UINT32                  MaximumInstrDecoded = 1;

    if (ZydisGetVersion() != ZYDIS_VERSION)
    {
        ShowMessages("invalid Zydis version\n");
        return DEBUGGER_CONDITIONAL_JUMP_STATUS_ERROR;
    }

    if (Isx86_64)
    {
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
    }
    else
    {
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32);
    }

    ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

    ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_FORCE_SEGMENT, ZYAN_TRUE);
    ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_FORCE_SIZE, ZYAN_TRUE);

    //
    // Replace the `ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS` function that
    // formats the absolute addresses
    //
    default_print_address_absolute =
        (ZydisFormatterFunc)&ZydisFormatterPrintAddressAbsolute;
    ZydisFormatterSetHook(&formatter, ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS, (const void **)&default_print_address_absolute);

    while (ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, BufferToDisassemble, BuffLength, &instruction, operands)))
    {
        //
        // We have to pass a `runtime_address` different to
        // `ZYDIS_RUNTIME_ADDRESS_NONE` to enable printing of absolute addresses
        //
        // ZydisFormatterFormatInstruction(&formatter, &instruction, operands, instruction.operand_count_visible, &buffer[0], sizeof(buffer), (ZyanU64)CurrentRip, ZYAN_NULL);

        //
        // Return len of buffer
        //
        return instruction.length;
    }

    //
    // Error in disassembling buffer
    //
    return 0;
}

/**
 * @brief Print addresses
 *
 * @param formatter
 * @param buffer
 * @param context
 * @return ZyanStatus
 */
static ZyanStatus
ZydisFormatterPrintAddressAbsoluteForTrackingInstructions(const ZydisFormatter *  formatter,
                                                          ZydisFormatterBuffer *  buffer,
                                                          ZydisFormatterContext * context)
{
    ZyanU64                                                address;
    std::map<UINT64, LOCAL_FUNCTION_DESCRIPTION>::iterator Iterate;

    ZYAN_CHECK(ZydisCalcAbsoluteAddress(context->instruction, context->operand, context->runtime_address, &address));

    //
    // Apply addressconversion of settings here
    //
    if (g_AddressConversion)
    {
        //
        // Check to find the symbol of address
        //
        Iterate = g_DisassemblerSymbolMap.find(address);

        if (Iterate != g_DisassemblerSymbolMap.end())
        {
            ZYAN_CHECK(ZydisFormatterBufferAppend(buffer, ZYDIS_TOKEN_SYMBOL));
            ZyanString * string;
            ZYAN_CHECK(ZydisFormatterBufferGetString(buffer, &string));

            //
            // Call the tracker callback (with function name)
            //
            CommandTrackHandleReceivedCallInstructions(Iterate->second.ObjectName.c_str(), Iterate->first);

            return ZyanStringAppendFormat(string,
                                          "<%s (%s)>",
                                          Iterate->second.ObjectName.c_str(),
                                          SeparateTo64BitValue(Iterate->first).c_str());
        }
    }

    //
    // Call the tracker callback (without function name)
    //
    CommandTrackHandleReceivedCallInstructions(NULL, address);

    return default_print_address_absolute(formatter, buffer, context);
}

/**
 * @brief Check whether the current instruction is a 'call' or 'ret' or not
 *
 * @param BufferToDisassemble Current Bytes of assembly
 * @param CurrentRip Address of current RIP
 * @param BuffLength Length of buffer
 * @param Isx86_64 Whether it's an x86 or x64
 * @param IsRet Whether it's a 'ret' or not
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgCheckWhetherTheCurrentInstructionIsCallOrRet(
    unsigned char * BufferToDisassemble,
    UINT64          CurrentRip,
    UINT32          BuffLength,
    BOOLEAN         Isx86_64,
    PBOOLEAN        IsRet)
{
    ZydisDecoder            decoder;
    ZydisFormatter          formatter;
    ZydisDecodedOperand     operands[ZYDIS_MAX_OPERAND_COUNT];
    int                     instr_decoded = 0;
    ZydisDecodedInstruction instruction;
    char                    buffer[256];
    UINT32                  MaximumInstrDecoded = 1;

    if (ZydisGetVersion() != ZYDIS_VERSION)
    {
        ShowMessages("invalid zydis version\n");
        return DEBUGGER_CONDITIONAL_JUMP_STATUS_ERROR;
    }

    if (!Isx86_64)
    {
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
    }
    else
    {
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32);
    }

    ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

    ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_FORCE_SEGMENT, ZYAN_TRUE);
    ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_FORCE_SIZE, ZYAN_TRUE);

    //
    // Replace the `ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS` function that
    // formats the absolute addresses
    //
    default_print_address_absolute =
        (ZydisFormatterFunc)&ZydisFormatterPrintAddressAbsoluteForTrackingInstructions;
    ZydisFormatterSetHook(&formatter, ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS, (const void **)&default_print_address_absolute);

    while (ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, BufferToDisassemble, BuffLength, &instruction, operands)))
    {
        if (instruction.mnemonic == ZydisMnemonic::ZYDIS_MNEMONIC_CALL)
        {
            //
            // It's a 'call' instruction
            //

            //
            // Log call
            //
            // ShowMessages("call length : 0x%x\n", instruction.length);

            //
            // We have to pass a `runtime_address` different to
            // `ZYDIS_RUNTIME_ADDRESS_NONE` to enable printing of absolute addresses
            //

            ZydisFormatterFormatInstruction(&formatter, &instruction, operands, instruction.operand_count_visible, &buffer[0], sizeof(buffer), (ZyanU64)CurrentRip, ZYAN_NULL);

            *IsRet = FALSE;

            return TRUE;
        }
        else if (instruction.mnemonic == ZydisMnemonic::ZYDIS_MNEMONIC_RET)
        {
            //
            // It's a 'ret' instruction
            //

            //
            // Log ret
            //
            // ShowMessages("ret length : 0x%x\n", instruction.length);

            //
            // Call the tracker callback
            //
            CommandTrackHandleReceivedRetInstructions(CurrentRip);

            *IsRet = TRUE;

            return TRUE;
        }
        else
        {
            //
            // It's not call
            //
            return FALSE;
        }
    }

    //
    // Should not reach here
    //
    return FALSE;
}

/**
 * @brief Check whether the current instruction is a 'ret' or not
 *
 * @param BufferToDisassemble Current Bytes of assembly
 * @param BuffLength Length of buffer
 * @param Isx86_64 Whether it's an x86 or x64
 * @param RetLength Length of ret (if return value is TRUE)
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgCheckWhetherTheCurrentInstructionIsRet(
    unsigned char * BufferToDisassemble,
    UINT64          BuffLength,
    BOOLEAN         Isx86_64)
{
    ZydisDecoder            decoder;
    ZydisFormatter          formatter;
    ZydisDecodedOperand     operands[ZYDIS_MAX_OPERAND_COUNT];
    UINT64                  CurrentRip    = 0;
    int                     instr_decoded = 0;
    ZydisDecodedInstruction instruction;
    char                    buffer[256];
    UINT32                  MaximumInstrDecoded = 1;

    if (ZydisGetVersion() != ZYDIS_VERSION)
    {
        ShowMessages("invalid zydis version\n");
        return DEBUGGER_CONDITIONAL_JUMP_STATUS_ERROR;
    }

    if (Isx86_64)
    {
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
    }
    else
    {
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32);
    }

    ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

    ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_FORCE_SEGMENT, ZYAN_TRUE);
    ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_FORCE_SIZE, ZYAN_TRUE);

    //
    // Replace the `ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS` function that
    // formats the absolute addresses
    //
    default_print_address_absolute =
        (ZydisFormatterFunc)&ZydisFormatterPrintAddressAbsolute;
    ZydisFormatterSetHook(&formatter, ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS, (const void **)&default_print_address_absolute);

    while (ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, BufferToDisassemble, BuffLength, &instruction, operands)))
    {
        //
        // We have to pass a `runtime_address` different to
        // `ZYDIS_RUNTIME_ADDRESS_NONE` to enable printing of absolute addresses
        //

        ZydisFormatterFormatInstruction(&formatter, &instruction, operands, instruction.operand_count_visible, &buffer[0], sizeof(buffer), (ZyanU64)CurrentRip, ZYAN_NULL);

        if (instruction.mnemonic == ZydisMnemonic::ZYDIS_MNEMONIC_RET)
        {
            //
            // It's a ret
            //

            //
            // Log ret
            //
            // ShowMessages("ret length : 0x%x\n", instruction.length);

            return TRUE;
        }
        else
        {
            //
            // It's not ret
            //
            return FALSE;
        }
    }

    //
    // Should not reach here
    //
    return FALSE;
}
