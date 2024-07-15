/**
 * @file a.cpp
 * @author Abbas Masoumi
 * @brief a command
 * @details
 * @version 0.10
 * @date 2024-07-14
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

extern BOOLEAN                  g_IsSerialConnectedToRemoteDebuggee;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

struct Instruction
{
    std::string              mnemonic;
    std::vector<std::string> operands;
};

struct AssembleData
{
    std::string              AsmRaw {};
    std::string              AsmFixed {};
    std::vector<Instruction> instructions;
    size_t                   StatementCount {};
    size_t                   BytesCount {};
    unsigned char *          EncodedBytes {};
    vector<UINT64>           EncBytesIntVec {};
    ks_err                   ks_err {};
};

/**
 * @brief help of a and !a command
 *
 * @return VOID
 */
VOID
CommandAssembleHelp()
{
    ShowMessages("a !a : assembles snippet at specific address. symbols are supported.\n");
    ShowMessages("\nIf you want to assemble to physical (address) memory then add '!' "
                 "at the start of the command\n");

    ShowMessages("syntax : \ta [Address (hex)] {AsmCmd1; AsmCmd2} [pid ProcessId (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : a fffff8077356f010 {nop; nop; nop} \n");
    ShowMessages("\t\te.g : a nt!ExAllocatePoolWithTag+10+@rcx {jmp nt!ExAllocatePoolWithTag+10} \n");
    ShowMessages("\t\te.g : a nt!ExAllocatePoolWithTag {add DWORD PTR [nt!ExAllocatePoolWithTag+10+@rax], 99} \n");
    ShowMessages("\t\te.g : !a abc01d {add DWORD PTR [nt!ExAllocatePoolWithTag+10+@rax], 99} \n");
}

/**
 * @brief tries to solve the symbol issue with Keystone, which apparently originates from LLVM-MC.
 *
 * @return VOID
 */
void
parseAssemblyLine(AssembleData & assembleData)
{
    std::string RawAsm = assembleData.AsmRaw;
    std::regex  instructionRegex(R"(\s*(\w+)\s+(.+))");
    std::smatch match;

    // split assembly line by ';'
    std::vector<std::string> assemblyInstructions;
    size_t                   pos       = 0;
    std::string              delimiter = ";";
    while ((pos = RawAsm.find(delimiter)) != std::string::npos)
    {
        std::string token = RawAsm.substr(0, pos);
        if (!token.empty())
        {
            assemblyInstructions.push_back(token);
        }
        RawAsm.erase(0, pos + delimiter.length());
    }
    if (!RawAsm.empty())
    {
        assemblyInstructions.push_back(RawAsm);
    }

    // process each assembly instruction
    for (const auto & instructionLine : assemblyInstructions)
    {
        if (std::regex_match(instructionLine, match, instructionRegex))
        {
            Instruction instr;
            instr.mnemonic = match[1].str();

            // split operands by ','
            std::string operandStr = match[2].str();
            size_t      start = 0, end = 0;
            while ((end = operandStr.find(',', start)) != std::string::npos)
            {
                instr.operands.push_back(operandStr.substr(start, end - start));
                start = end + 1;
            }
            instr.operands.push_back(operandStr.substr(start));

            // remove '[' and ']' from operands if present
            for (auto & operand : instr.operands)
            {
                // a simple check to see if we're dealing with symbols or not
                bool containsExclamation = (operand.find('!') != std::string::npos);
                bool containsAt          = (operand.find('@') != std::string::npos);

                if (!containsExclamation && !containsAt)
                {
                    continue;
                }

                std::string expr;
                UINT64      expr_addr;
                // find '[' and ']' to replace their content
                auto leftBracketPos  = operand.find('[');
                auto rightBracketPos = operand.find(']');
                if (leftBracketPos != std::string::npos && rightBracketPos != std::string::npos &&
                    rightBracketPos > leftBracketPos + 1)
                {
                    expr = operand.substr(leftBracketPos + 1, rightBracketPos - leftBracketPos - 1);

                    if (!SymbolConvertNameOrExprToAddress(expr, &expr_addr))
                    {
                        // not resolved
                        continue;
                    }

                    // replace symbol with its equivalent address
                    std::ostringstream oss;
                    oss << std::hex << std::showbase << expr_addr;
                    operand.replace(leftBracketPos + 1, rightBracketPos - leftBracketPos - 1, oss.str());
                }
                else
                {
                    expr = operand.substr(leftBracketPos + 1, rightBracketPos - leftBracketPos - 1);

                    if (!SymbolConvertNameOrExprToAddress(expr, &expr_addr))
                    {
                        // not resolved
                        continue;
                    }

                    // replace symbol with its equivalent address
                    std::ostringstream oss;
                    oss << std::hex << std::showbase << expr_addr;
                    operand.replace(leftBracketPos + 1, rightBracketPos - leftBracketPos - 1, oss.str());
                }
            }

            assembleData.instructions.push_back(instr);
        }
        else
        {
            // most probably a unary instruction
            // std::cerr << "Error: Failed to parse instruction: " << instructionLine << std::endl;
            Instruction instr;
            instr.mnemonic = instructionLine;
            assembleData.instructions.push_back(instr);
        }
    }

    // serialize the fixed asm
    std::ostringstream oss;
    for (const auto & instr : assembleData.instructions)
    {
        oss << instr.mnemonic;
        for (size_t i = 0; i < instr.operands.size(); ++i)
        {
            if (i == 0)
            {
                oss << " ";
            }
            else
            {
                oss << ",";
            }
            oss << instr.operands[i];
        }
        oss << ";";
    }

    assembleData.AsmFixed = oss.str();
}

std::vector<std::string>
ParseUserCmd(const std::string & command)
{
    std::vector<std::string> result;
    auto                     it  = command.begin();
    auto                     end = command.end();
    while (it != end)
    {
        // skip leading whitespace
        it = std::find_if_not(it, end, ::isspace);
        if (it == end)
            break;
        if (*it == '{')
        {
            // find matching closing brace
            auto closeIt = std::find(it + 1, end, '}');
            if (closeIt != end)
            {
                std::string RawAsm(it + 1, closeIt); // remove "{}"
                result.emplace_back(RawAsm);
                it = closeIt + 1;
            }
            else
            {
                // no matching brace
                ShowMessages("Assembly snippet is not closed.");
            }
        }
        else
        {
            // normal text, find next space
            auto spaceIt = std::find_if(it, end, ::isspace);
            result.emplace_back(it, spaceIt);
            it = spaceIt;
        }
    }
    return result;
}

static bool
sym_resolver(const char * symbol, uint64_t * value)
{
    std::string sym(symbol);

    if (!SymbolConvertNameOrExprToAddress(sym, value))
    {
        ShowMessages("err, couldn't resolve Address at '%s'\n\n",
                     symbol);
        CommandAssembleHelp();
        return false;
    }
    return true;
}

static int
Assemble(ks_arch arch, int mode, uint64_t start_addr, int syntax, AssembleData & asmbDat)
{
    ks_engine * ks;

    asmbDat.ks_err = ks_open(arch, mode, &ks);
    if (asmbDat.ks_err != KS_ERR_OK)
    {
        ShowMessages("err, failed on ks_open().");
        return -1;
    }

    if (syntax)
    {
        asmbDat.ks_err = ks_option(ks, KS_OPT_SYNTAX, syntax);
        if (asmbDat.ks_err != KS_ERR_OK)
        {
            ShowMessages("err, failed on ks_option() with error code = %u\n", asmbDat.ks_err);
        }
    }

    // Setting symbol resolver callback
    asmbDat.ks_err = ks_option(ks, KS_OPT_SYM_RESOLVER, (size_t)sym_resolver);
    if (asmbDat.ks_err != KS_ERR_OK)
    {
        ShowMessages("err, failed on ks_option() with error code = %u\n", asmbDat.ks_err);
    }

    if (ks_asm(ks, asmbDat.AsmFixed.c_str(), start_addr, &asmbDat.EncodedBytes, &asmbDat.BytesCount, &asmbDat.StatementCount))
    {
        asmbDat.ks_err = ks_errno(ks);
        ShowMessages("err, failed on ks_asm() with count = %lu, error code = %u\n", (int)asmbDat.StatementCount, asmbDat.ks_err);
    }
    else
    {
        if (asmbDat.BytesCount == 0)
        {
            ShowMessages("err, the assemble operation returned no bytes, most likely due to incorrect formatting.\n");
        }
        else
        {
            ShowMessages("generated assembly: %lu bytes, %lu statements ==>> ", (int)asmbDat.BytesCount, (int)asmbDat.StatementCount);

            size_t i;
            ShowMessages("%s = ", asmbDat.AsmRaw.c_str());
            for (i = 0; i < asmbDat.BytesCount; i++)
            {
                ShowMessages("%02x ", asmbDat.EncodedBytes[i]);
                asmbDat.EncBytesIntVec.push_back(static_cast<UINT64>(asmbDat.EncodedBytes[i]));
            }
            ShowMessages("\n");

            ks_close(ks);
            return 0;
        }
    }
    ks_close(ks);
    return -1;
}

/**
 * @brief a and !a commands handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandAssemble(vector<string> SplitCommand, string Command)
{
    UINT64                    Address {};
    vector<UINT64>            ValuesToEdit {};
    DEBUGGER_EDIT_MEMORY      EditMemoryRequest {};
    UINT64                    Value {};
    UINT32                    ProcId {};
    DEBUGGER_EDIT_MEMORY_TYPE MemoryType;

    if (g_ActiveProcessDebuggingState.IsActive)
    {
        ProcId = g_ActiveProcessDebuggingState.ProcessId;
    }

    std::vector<std::string> CmdVec = ParseUserCmd(Command);

    if (CmdVec.size() < 3)
    {
        ShowMessages("incorrect use of the 'a'\n\n");
        CommandAssembleHelp();
        return;
    }
    else if (CmdVec[0] == "a")
    {
        MemoryType = EDIT_VIRTUAL_MEMORY;
    }
    else if (CmdVec[0] == "!a")
    {
        MemoryType = EDIT_PHYSICAL_MEMORY;
    }
    else
    {
        ShowMessages("incorrect use of the 'a'\n\n");
        CommandAssembleHelp();
        return;
    }

    // check if there is any "pid"
    auto it = std::find_if(CmdVec.begin(), CmdVec.end(), [](const std::string & s) { return s.find("pid") != std::string::npos; });
    if (it != CmdVec.end())
    {
        if (g_IsSerialConnectedToRemoteDebuggee)
        {
            ShowMessages(ASSERT_MESSAGE_CANNOT_SPECIFY_PID);
            return;
        }

        if (std::next(it) != CmdVec.end()) // is there anything after "pid"?
        {
            if (!ConvertStringToUInt32(std::string(*(++it)), &ProcId))
            {
                ShowMessages("please specify a correct hex process id\n\n");
                CommandAssembleHelp();
                return;
            }
        }
    }
    else
    {
        // pid Not found
    }

    // fetching start_address i.e. address to assemble to
    if (!SymbolConvertNameOrExprToAddress(CmdVec.at(1), &Address))
    {
        ShowMessages("err, couldn't resolve Address at '%s'\n\n",
                     CmdVec.at(1).c_str());
        CommandAssembleHelp();
        return;
    }

    AssembleData AssembleData;
    AssembleData.AsmRaw = CmdVec.at(2);
    parseAssemblyLine(AssembleData);

    if (Assemble(KS_ARCH_X86, KS_MODE_64, Address, KS_OPT_SYNTAX_INTEL, AssembleData))
    {
        ShowMessages("err, code: '%u'\n\n", AssembleData.ks_err);
        CommandAssembleHelp();
        return;
    }

    if (ProcId == 0)
    {
        ProcId = GetCurrentProcessId();
    }

    if (HyperDbgWriteMemory(
            (PVOID)Address,
            MemoryType,
            ProcId,
            (PVOID)AssembleData.EncodedBytes,
            (UINT32)AssembleData.BytesCount))
    {
        ShowMessages("successfully assembled at 0x%016llx address.\n", static_cast<uint64_t>(Address));
    }
    else
    {
        ShowMessages("failed to write generated assembly to memory.");
    }
}
