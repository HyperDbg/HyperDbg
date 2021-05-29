/**
 * @file symbol-parser.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief symbok parser headers
 * @details 
 * @version 0.1
 * @date 2021-05-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Exports                    //
//////////////////////////////////////////////////
extern "C" {
__declspec(dllexport) UINT64 SymConvertNameToAddress(const char * FunctionName, PBOOLEAN WasFound);
}

//////////////////////////////////////////////////
//					Functions                   //
//////////////////////////////////////////////////
BOOL
SymGetFileParams(const char * FileName, DWORD64 & BaseAddr, DWORD & FileSize);

BOOL
SymGetFileSize(const char * FileName, DWORD & FileSize);

VOID
SymShowSymbolInfo(DWORD64 ModBase);

BOOL CALLBACK
SymEnumSymbolsCallback(SYMBOL_INFO * SymInfo, ULONG SymbolSize, PVOID UserContext);

VOID
SymShowSymbolDetails(SYMBOL_INFO & SymInfo);

const char *
SymTagStr(ULONG Tag);
