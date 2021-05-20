/**
 * @file symbol.h
 * @author Alee Amini (aleeaminiz@gmail.com)
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief symbol parser header
 * @details
 * @version 0.1
 * @date 2021-05-20
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

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
