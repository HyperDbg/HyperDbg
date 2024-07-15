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
    VOID
    ParseAssemblyData()
    {
        std::string RawAsm = AsmRaw;

        //
        // remove all "\n" instances
        //
        RawAsm.erase(std::remove(RawAsm.begin(), RawAsm.end(), '\n'), RawAsm.end());

        //
        // remove multiple spaces
        //
        std::regex MultipleSpaces(" +");
        RawAsm = std::regex_replace(RawAsm, MultipleSpaces, " ");

        //
        // split assembly line by ';'
        //
        std::vector<std::string> AssemblyInstructions;
        size_t                   Pos       = 0;
        std::string              Delimiter = ";";
        while ((Pos = RawAsm.find(Delimiter)) != std::string::npos)
        {
            std::string Token = RawAsm.substr(0, Pos);
            if (!Token.empty())
            {
                AssemblyInstructions.push_back(Token);
            }
            RawAsm.erase(0, Pos + Delimiter.length());
        }
        if (!RawAsm.empty())
        {
            AssemblyInstructions.push_back(RawAsm);
        }

        //
        // process each assembly instruction
        //
        for (auto & InstructionLine : AssemblyInstructions)
        {
            std::string Expr {};
            UINT64      ExprAddr {};
            size_t      Start {};

            while ((Start = InstructionLine.find('<', Start)) != std::string::npos)
            {
                size_t End = InstructionLine.find('>', Start);
                if (End != std::string::npos)
                {
                    std::string Expr = InstructionLine.substr(Start + 1, End - Start - 1);
                    if (!SymbolConvertNameOrExprToAddress(Expr, &ExprAddr) && ExprAddr == 0)
                    {
                        ShowMessages("err, failed to resolve the symbol [ %s ].\n", Expr.c_str());
                        Start += Expr.size() + 2;
                        continue;
                    }

                    std::ostringstream Oss;
                    Oss << std::hex << std::showbase << ExprAddr;
                    InstructionLine.replace(Start, End - Start + 1, Oss.str());
                }
                else
                {
                    //
                    // No matching '>' found, break the loop
                    //
                    break;
                }
                Start += Expr.size() + 2;
            }
        }

        //
        // Append ";" between two std::strings
        //
        auto ApndSemCln = [](std::string a, std::string b) {
            return std::move(a) + ';' + std::move(b);
        };

        //
        // Concatenate each assembly line
        //
        AsmFixed = std::accumulate(std::next(AssemblyInstructions.begin()), AssemblyInstructions.end(), AssemblyInstructions.at(0), ApndSemCln);

        if (!AsmFixed.empty() && AsmFixed.back() == ';')
        {
            //
            // remove the last ";" for it will be counted as a statement by Keystone and a wrong number would be printed
            //
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
    std::string CommandStr;
    std::string AddressStr;
    std::string AsmSnippet;
    std::string StartAddressStr;
    std::string ProcIdStr;

    _CMD() = default;

    bool isEmpty() const
    {
        return CommandStr.empty() &&
               AddressStr.empty() &&
               AsmSnippet.empty() &&
               StartAddressStr.empty() &&
               ProcIdStr.empty();
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

    ShowMessages("syntax : \ta [Address (hex)] [asm {AsmCmd1; AsmCmd2}] [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \t!a [Address (hex)] [asm {AsmCmd1; AsmCmd2}]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : a fffff8077356f010 {nop; nop; nop} \n");
    ShowMessages("\t\te.g : a nt!ExAllocatePoolWithTag+10+@rcx {jmp <nt!ExAllocatePoolWithTag+10>} \n");
    ShowMessages("\t\te.g : a nt!ExAllocatePoolWithTag {add DWORD PTR [<nt!ExAllocatePoolWithTag+10+@rax>], 99} \n");
    ShowMessages("\t\t      note that within assembly snippet, symbols must be enclosed by \" <> \"\n");
    ShowMessages("\t\t      note that type specifier must be capital\n");

    ShowMessages("\n");
    ShowMessages("\tto merely view the byte code of an assembly snippet:\n");
    ShowMessages("\t\ta {jmp <nt!ExAllocatePoolWithTag+10>} [StartAddress]\n");
    ShowMessages("\t\t\"StartAddress\" is useful when dealing with relative instructions like \" jmp \" ");
}

_CMD
ParseUserCmd(const std::string & command)
{
    std::vector<std::string> CmdVec;
    _CMD                     CMD {};
    _CMD                     CmdEmpty {};

    auto CmdIt  = command.begin();
    auto CmdEnd = command.end();

    while (CmdIt != CmdEnd)
    {
        //
        // skip leading whitespace
        //
        CmdIt = std::find_if_not(CmdIt, CmdEnd, ::isspace);
        if (CmdIt == CmdEnd)
            break;
        if (*CmdIt == '{')
        {
            //
            // find matching closing brace
            //
            auto CloseIt = std::find(CmdIt + 1, CmdEnd, '}');
            if (CloseIt != CmdEnd)
            {
                if (CmdVec.size() < 2)
                {
                    //
                    // if yes, asm snippet was provided right after "a" command (2nd index)
                    // i.e. no address were provided to assembling to
                    //
                    CmdVec.emplace_back(""); // emtpty addres string
                }
                std::string RawAsm(CmdIt + 1, CloseIt); // remove "{}"
                CmdVec.emplace_back(RawAsm);
                CmdIt = CloseIt + 1;
            }
            else
            {
                //
                // no matching brace
                //
                ShowMessages("err, assembly snippet is not closed.");
            }
        }
        else
        {
            //
            // normal text, find next space
            //
            auto SpaceIt = std::find_if(CmdIt, CmdEnd, ::isspace);
            CmdVec.emplace_back(CmdIt, SpaceIt);
            CmdIt = SpaceIt;
        }
    }

    //
    // Check if there is any "pid"
    //
    auto PidIt = std::find_if(CmdVec.begin(), CmdVec.end(), [](const std::string & s) { return s.find("pid") != std::string::npos; });
    if (PidIt != CmdVec.end())
    {
        bool IsLast       = (PidIt == std::prev(CmdVec.end()));
        bool IsSecondLast = (PidIt == std::prev(CmdVec.end(), 2));
        if (!IsLast && !IsSecondLast)
        {
            ShowMessages("pid must be the last argument");
            return CmdEmpty;
        }
        if (g_IsSerialConnectedToRemoteDebuggee)
        {
            ShowMessages(ASSERT_MESSAGE_CANNOT_SPECIFY_PID);
            return CmdEmpty;
        }

        if (std::next(PidIt) != CmdVec.end()) // is there anything after "pid"?
        {
            CMD.ProcIdStr = std::string(*(++PidIt));
        }
        else
        {
            ShowMessages("no hex number was provided as process id\n\n");
            return CmdEmpty;
        }
    }
    else
    {
        //
        // user didn't provide pid at all. ignoring.
        //
    }

    //
    // fill the CMD object. will be optimized later
    //
    if (CmdVec.size() > 0)
        CMD.CommandStr = CmdVec.at(0);
    if (CmdVec.size() > 1)
        CMD.AddressStr = CmdVec.at(1);
    if (CmdVec.size() > 2)
        CMD.AsmSnippet = CmdVec.at(2);
    if (CmdVec.size() > 3)
        CMD.StartAddressStr = CmdVec.at(3);
    // if (CmdVec.size() > 4) CMD.ProcId_str        = CmdVec.at(5);  4 is the "pid" string

    return CMD;
}

static bool
SymResolver(const char * Symbol, UINT64 * Value)
{
    std::string Sym(Symbol);

    if (!SymbolConvertNameOrExprToAddress(Sym, Value))
    {
        ShowMessages("err, couldn't resolve Address at '%s'\n\n",
                     Symbol);
        CommandAssembleHelp();
        return false;
    }
    return true;
}

INT
Assemble(ks_arch Arch, INT Mode, UINT64 StartAddr, INT Syntax, AssembleData & AsmbDat)
{
    ks_engine * Ks;

    AsmbDat.ks_err = ks_open(Arch, Mode, &Ks);
    if (AsmbDat.ks_err != KS_ERR_OK)
    {
        ShowMessages("err, failed on ks_open()");
        return -1;
    }

    if (Syntax)
    {
        AsmbDat.ks_err = ks_option(Ks, KS_OPT_SYNTAX, Syntax);
        if (AsmbDat.ks_err != KS_ERR_OK)
        {
            ShowMessages("err, failed on ks_option() with error code = %u\n", AsmbDat.ks_err);
        }
    }

    //
    // Setting symbol resolver callback
    //
    AsmbDat.ks_err = ks_option(Ks, KS_OPT_SYM_RESOLVER, (size_t)SymResolver);
    if (AsmbDat.ks_err != KS_ERR_OK)
    {
        ShowMessages("err, failed on ks_option() with error code = %u\n", AsmbDat.ks_err);
    }

    if (ks_asm(Ks, AsmbDat.AsmFixed.c_str(), StartAddr, &AsmbDat.EncodedBytes, &AsmbDat.BytesCount, &AsmbDat.StatementCount))
    {
        AsmbDat.ks_err = ks_errno(Ks);
        ShowMessages("err, failed on ks_asm() with count = %lu, error code = %u\n", (int)AsmbDat.StatementCount, AsmbDat.ks_err);
    }
    else
    {
        if (AsmbDat.BytesCount == 0)
        {
            ShowMessages("err, the assemble operation returned no bytes, most likely due to incorrect formatting of assembly snippet\n");
        }
        else
        {
            ShowMessages("generated assembly: %lu bytes, %lu statements ==>> ", (int)AsmbDat.BytesCount, (int)AsmbDat.StatementCount);

            size_t i;
            ShowMessages("%s = ", AsmbDat.AsmFixed.c_str());
            for (i = 0; i < AsmbDat.BytesCount; i++)
            {
                ShowMessages("%02x ", AsmbDat.EncodedBytes[i]);
                AsmbDat.EncBytesIntVec.push_back(static_cast<UINT64>(AsmbDat.EncodedBytes[i]));
            }
            ShowMessages("\n");

            ks_close(Ks);
            return 0;
        }
    }
    ks_close(Ks);
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
    UINT64                    StartAddress {};
    UINT32                    ProcId = 0;

    CMD = ParseUserCmd(Command);

    if (CMD.isEmpty())
    {
        //
        // Error messeges are already printed
        //
        return;
    }

    if (!CMD.ProcIdStr.empty())
    {
        if (!ConvertStringToUInt32(CMD.ProcIdStr, &ProcId))
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
    else if (CMD.CommandStr == "a")
    {
        MemoryType = EDIT_VIRTUAL_MEMORY;
    }
    else if (CMD.CommandStr == "!a")
    {
        MemoryType = EDIT_PHYSICAL_MEMORY;
    }
    else
    {
        ShowMessages("unknown assemble command\n\n");
        CommandAssembleHelp();
        return;
    }

    //
    // Fetching start_address i.e. address to assemble to
    //
    if (!CMD.AddressStr.empty()) // was any address provided to assemble to?
    {
        if (!SymbolConvertNameOrExprToAddress(CMD.AddressStr, &Address))
        {
            ShowMessages("err, couldn't resolve Address at '%s'\n\n",
                         CMD.AddressStr.c_str());
            CommandAssembleHelp();
            return;
        }
    }
    else if (!CMD.StartAddressStr.empty()) // was a custom start_address provided?
    {
        if (!SymbolConvertNameOrExprToAddress(CMD.StartAddressStr, &StartAddress))
        {
            ShowMessages("err, couldn't resolve Address at '%s'\n\n",
                         CMD.StartAddressStr.c_str());
            CommandAssembleHelp();
            return;
        }
        Address = StartAddress;
    }
    else
    {
        ShowMessages("warning, no start address provided to calculate relative asm commands\n\n");
    }

    AssembleData AssembleData;
    AssembleData.AsmRaw = CMD.AsmSnippet; // third element
    AssembleData.ParseAssemblyData();

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
            ShowMessages("successfully assembled at 0x%016llx address\n", static_cast<UINT64>(Address));
        }
        else
        {
            ShowMessages("failed to write generated assembly to memory\n");
        }
    }
}
