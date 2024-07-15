
/**
 * @file a.cpp
 * @author Abbas Masoumi Gorji
 * @brief a command
 * @details
 * @version 0.10
 * @date 2024-07-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"
#include "keystone\keystone.h"

extern BOOLEAN                  g_IsSerialConnectedToRemoteDebuggee;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

struct AssembleData
{
public:
    std::string     AsmRaw {};
    std::string     AsmFixed {};
    size_t          StatementCount {};
    size_t          BytesCount {};
    unsigned char * EncodedBytes {};
    vector<UINT64>  EncBytesIntVec {};
    ks_err          ks_err {};

    AssembleData() = default;

    /**
     * @brief tries to solve the symbol issue with Keystone, which apparently originates from LLVM-MC.
     *
     * @return VOID
     */
    void
    parseAssemblyData()
    {
        std::string RawAsm = AsmRaw;

        // remove all "\n" instances
        RawAsm.erase(std::remove(RawAsm.begin(), RawAsm.end(), '\n'), RawAsm.end());

        // remove multiple spaces
        std::regex multipleSpaces(" +");
        RawAsm = std::regex_replace(RawAsm, multipleSpaces, " ");

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
        for (auto & instructionLine : assemblyInstructions)
        {
            std::string expr {};
            UINT64      expr_addr {};
            size_t      start {};

            while ((start = instructionLine.find('<', start)) != std::string::npos)
            {
                size_t end = instructionLine.find('>', start);
                if (end != std::string::npos)
                {
                    std::string expr = instructionLine.substr(start + 1, end - start - 1);
                    if (!SymbolConvertNameOrExprToAddress(expr, &expr_addr) && expr_addr == 0)
                    {
                        ShowMessages("err, failed to resolve the symbol [ %s ].\n", expr.c_str());
                        start += expr.size() + 2;
                        continue;
                    }

                    std::ostringstream oss;
                    oss << std::hex << std::showbase << expr_addr;
                    instructionLine.replace(start, end - start + 1, oss.str());
                }
                else
                {
                    // No matching '>' found, break the loop
                    break;
                }
                start += expr.size() + 2;
            }
        }

        // Append ";" between two std::strings
        auto apndSemCln = [](std::string a, std::string b) {
            return std::move(a) + ';' + std::move(b);
        };
        // Concatenate each assembly line
        AsmFixed = std::accumulate(std::next(assemblyInstructions.begin()), assemblyInstructions.end(), assemblyInstructions.at(0), apndSemCln);

        if (!AsmFixed.empty() && AsmFixed.back() == ';')
        {
            // remove the last ";" for it will be counted as a statement by Keystone and a wrong number would be printed
            AsmFixed.pop_back();
        }

        while (!AsmFixed.empty() && AsmFixed.back() == ';')
        {
            AsmFixed.pop_back();
        }
    }
};

class _CMD
{
public:
    std::string Command_str;
    std::string Address_str;
    std::string AsmSnippet;
    std::string Start_Address_str;
    std::string ProcId_str;

    _CMD() = default;

    bool isEmpty() const
    {
        return Command_str.empty() &&
               Address_str.empty() &&
               AsmSnippet.empty() &&
               Start_Address_str.empty() &&
               ProcId_str.empty();
    }
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
    ShowMessages("\t\te.g : a nt!ExAllocatePoolWithTag+10+@rcx {jmp <nt!ExAllocatePoolWithTag+10>} \n");
    ShowMessages("\t\te.g : a nt!ExAllocatePoolWithTag {add DWORD PTR [<nt!ExAllocatePoolWithTag+10+@rax>], 99} \n");
    ShowMessages("\t\t      note that within assembly snippet, symbols must be enclosed by \" <> \"\n");
    ShowMessages("\t\t      note that type specifier must be capital\n");

    ShowMessages("\n");
    ShowMessages("\tto merely view the byte code of an assembly snippet:\n");
    ShowMessages("\t\ta {jmp <nt!ExAllocatePoolWithTag+10>} [start_address]\n");
    ShowMessages("\t\t\"start_address\" is useful when dealing with relative instructions like \" jmp \" ");
}

_CMD
ParseUserCmd(const std::string & command)
{
    std::vector<std::string> CmdVec;
    _CMD                     CMD {};
    _CMD                     CmdEmpty {};

    auto Cmd_it  = command.begin();
    auto Cmd_end = command.end();

    while (Cmd_it != Cmd_end)
    {
        // skip leading whitespace
        Cmd_it = std::find_if_not(Cmd_it, Cmd_end, ::isspace);
        if (Cmd_it == Cmd_end)
            break;
        if (*Cmd_it == '{')
        {
            // find matching closing brace
            auto closeIt = std::find(Cmd_it + 1, Cmd_end, '}');
            if (closeIt != Cmd_end)
            {
                if (CmdVec.size() < 2)
                {
                    // if yes, asm snippet was provided right after "a" command (2nd index)
                    // i.e. no address were provided to assembling to
                    CmdVec.emplace_back(""); // emtpty addres string
                }
                std::string RawAsm(Cmd_it + 1, closeIt); // remove "{}"
                CmdVec.emplace_back(RawAsm);
                Cmd_it = closeIt + 1;
            }
            else
            {
                // no matching brace
                ShowMessages("err, assembly snippet is not closed.");
            }
        }
        else
        {
            // normal text, find next space
            auto spaceIt = std::find_if(Cmd_it, Cmd_end, ::isspace);
            CmdVec.emplace_back(Cmd_it, spaceIt);
            Cmd_it = spaceIt;
        }
    }

    // check if there is any "pid"
    auto Pid_it = std::find_if(CmdVec.begin(), CmdVec.end(), [](const std::string & s) { return s.find("pid") != std::string::npos; });
    if (Pid_it != CmdVec.end())
    {
        bool isLast       = (Pid_it == std::prev(CmdVec.end()));
        bool isSecondLast = (Pid_it == std::prev(CmdVec.end(), 2));
        if (!isLast && !isSecondLast)
        {
            ShowMessages("pid must be the last argument.");
            return CmdEmpty;
        }
        if (g_IsSerialConnectedToRemoteDebuggee)
        {
            ShowMessages(ASSERT_MESSAGE_CANNOT_SPECIFY_PID);
            return CmdEmpty;
        }

        if (std::next(Pid_it) != CmdVec.end()) // is there anything after "pid"?
        {
            CMD.ProcId_str = std::string(*(++Pid_it));
        }
        else
        {
            ShowMessages("no hex number was provided as process id.\n\n");
            return CmdEmpty;
        }
    }
    else
    {
        // user didn't provide pid at all. ignoring.
    }

    // fill the CMD object. will be optimized later.
    if (CmdVec.size() > 0)
        CMD.Command_str = CmdVec.at(0);
    if (CmdVec.size() > 1)
        CMD.Address_str = CmdVec.at(1);
    if (CmdVec.size() > 2)
        CMD.AsmSnippet = CmdVec.at(2);
    if (CmdVec.size() > 3)
        CMD.Start_Address_str = CmdVec.at(3);
    // if (CmdVec.size() > 4) CMD.ProcId_str        = CmdVec.at(5);  4 is the "pid" string

    return CMD;
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
            ShowMessages("err, the assemble operation returned no bytes, most likely due to incorrect formatting of assembly snippet.\n");
        }
        else
        {
            ShowMessages("generated assembly: %lu bytes, %lu statements ==>> ", (int)asmbDat.BytesCount, (int)asmbDat.StatementCount);

            size_t i;
            ShowMessages("%s = ", asmbDat.AsmFixed.c_str());
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
    DEBUGGER_EDIT_MEMORY_TYPE MemoryType {};
    _CMD                      CMD {};
    UINT64                    Address {};
    UINT64                    Start_Address {};
    UINT32                    ProcId = 0;

    CMD = ParseUserCmd(Command);

    if (CMD.isEmpty())
    {
        // error messeges are already printed
        return;
    }

    if (!CMD.ProcId_str.empty())
    {
        if (!ConvertStringToUInt32(CMD.ProcId_str, &ProcId))
        {
            ShowMessages("please specify a correct hex process id\n\n");
            CommandAssembleHelp();
            return;
        }
    }

    if (g_ActiveProcessDebuggingState.IsActive)
    {
        ProcId = g_ActiveProcessDebuggingState.ProcessId;
    }

    if (CMD.AsmSnippet.empty())
    {
        ShowMessages("no assembly snippet provided\n");
        CommandAssembleHelp();
        return;
    }
    else if (CMD.Command_str == "a")
    {
        MemoryType = EDIT_VIRTUAL_MEMORY;
    }
    else if (CMD.Command_str == "!a")
    {
        MemoryType = EDIT_PHYSICAL_MEMORY;
    }
    else
    {
        ShowMessages("unknown assemble command.\n\n");
        CommandAssembleHelp();
        return;
    }

    // fetching start_address i.e. address to assemble to
    if (!CMD.Address_str.empty()) // was any address provided to assemble to?
    {
        if (!SymbolConvertNameOrExprToAddress(CMD.Address_str, &Address))
        {
            ShowMessages("err, couldn't resolve Address at '%s'\n\n",
                         CMD.Address_str.c_str());
            CommandAssembleHelp();
            return;
        }
    }
    else if (!CMD.Start_Address_str.empty()) // was a custom start_address provided?
    {
        if (!SymbolConvertNameOrExprToAddress(CMD.Start_Address_str, &Start_Address))
        {
            ShowMessages("err, couldn't resolve Address at '%s'\n\n",
                         CMD.Start_Address_str.c_str());
            CommandAssembleHelp();
            return;
        }
        Address = Start_Address;
    }
    else
    {
        ShowMessages("warn, no start_address provided to calculate relative asm commands.\n\n");
    }

    AssembleData AssembleData;
    AssembleData.AsmRaw = CMD.AsmSnippet; // third element
    AssembleData.parseAssemblyData();

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

    if (Address) // was the user only trying to get the bytes?
    {
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
}
