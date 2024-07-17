/**
 * @file assembler.cpp
 * @author Abbas Masoumi Gorji
 * @brief turns asm code into byte
 * @details
 * @version 0.1
 * @date 2024-7-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#include "pch.h"

/**
 * @brief tries to solve the symbol issue with Keystone, which apparently originates from LLVM-MC.
 *
 * @return VOID
 */
VOID
AssembleData::ParseAssemblyData()
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

INT
AssembleData::Assemble(UINT64 StartAddr, ks_arch Arch, INT Mode, INT Syntax)
{
    ks_engine * Ks;

    KsErr = ks_open(Arch, Mode, &Ks);
    if (KsErr != KS_ERR_OK)
    {
        ShowMessages("err, failed on ks_open()");
        return -1;
    }

    if (Syntax)
    {
        KsErr = ks_option(Ks, KS_OPT_SYNTAX, Syntax);
        if (KsErr != KS_ERR_OK)
        {
            ShowMessages("err, failed on ks_option() with error code = %u\n", KsErr);
        }
    }

    //
    // as long as symbols are parsed before passing asm code, SymResolver is not needed, for now.
    //
    KsErr = ks_option(Ks, KS_OPT_SYM_RESOLVER, 0); // null SymResolver
    if (KsErr != KS_ERR_OK)
    {
        ShowMessages("err, failed on ks_option() with error code = %u\n", KsErr);
    }

    if (ks_asm(Ks, AsmFixed.c_str(), StartAddr, &EncodedBytes, &BytesCount, &StatementCount))
    {
        KsErr = ks_errno(Ks);
        ShowMessages("err, failed on ks_asm() with count = %lu, error code = %u\n", (int)StatementCount, KsErr);
    }
    else
    {
        if (BytesCount == 0)
        {
            ShowMessages("err, the assemble operation returned no bytes, most likely due to incorrect formatting of assembly snippet\n");
        }
        else
        {
            //
            // inserting zero termination for compatibility with C functions
            //
            EncodedBytes[BytesCount] = '\0'; 

            ShowMessages("generated assembly: %lu bytes, %lu statements ==>> ", (int)BytesCount, (int)StatementCount);

            size_t i;
            ShowMessages("%s = ", AsmFixed.c_str());
            for (i = 0; i < BytesCount; i++)
            {
                ShowMessages("%02x ", EncodedBytes[i]);
                EncBytesIntVec.push_back(static_cast<UINT64>(EncodedBytes[i]));
            }
            ShowMessages("\n");

            ks_close(Ks);
            return 0;
        }
    }
    ks_close(Ks);
    return -1;
}

AssembleData *
create_AssembleData()
{
    return new AssembleData;
}