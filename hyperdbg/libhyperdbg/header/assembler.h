/**
* 
* @author Abbas Masoumi Gorji
* @date 2024-7-16
* 
**/

#pragma once

#include "pch.h"


class AssembleData
{
public:
    std::string     AsmRaw {};
    std::string     AsmFixed {};
    size_t          StatementCount {};
    size_t          BytesCount {};
    unsigned char * EncodedBytes {};
    vector<UINT64>  EncBytesIntVec {};
    ks_err          KsErr {};

    AssembleData() = default;

    VOID
    ParseAssemblyData();

    INT
    Assemble(UINT64 StartAddr, ks_arch Arch = KS_ARCH_X86, INT Mode = KS_MODE_64, INT Syntax = KS_OPT_SYNTAX_INTEL);
};