/**
 * @file symbol-linux.cpp
 * @author Max Raulea (max.raulea@gmail.com)
 * @brief Linux stub implementations of the symbol subsystem
 * @details The Windows implementation uses DbgHelp + PDB files (symbol-parser/).
 *          Linux uses ELF/DWARF which requires a separate implementation.
 *          These stubs allow the library to compile and link on Linux while
 *          keeping all call sites intact.
 *
 *          TODO: implement a real ELF/DWARF symbol parser for Linux
 *                (libdw / libelf / libbfd) and replace these stubs.
 *
 * @version 0.1
 * @date 2026-06-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#ifdef __linux__

VOID
SymbolBuildAndShowSymbolTable()
{
    ShowMessages("err, symbol table is not supported on Linux yet\n");
}

BOOLEAN
SymbolShowFunctionNameBasedOnAddress(UINT64 Address, PUINT64 UsedBaseAddress)
{
    return FALSE;
}

BOOLEAN
SymbolLoadOrDownloadSymbols(BOOLEAN IsDownload, BOOLEAN SilentLoad)
{
    if (!SilentLoad)
        ShowMessages("err, symbol loading is not supported on Linux yet\n");
    return FALSE;
}

/**
 * @brief Attempt to resolve a name or expression to an address.
 *
 * On Linux, full symbol resolution (ELF/DWARF) is not yet implemented.
 * This stub handles the common case of a plain hex/decimal literal so that
 * numeric addresses still work everywhere in the debugger.
 */
BOOLEAN
SymbolConvertNameOrExprToAddress(const string & TextToConvert, PUINT64 Result)
{
    try
    {
        *Result = std::stoull(TextToConvert, nullptr, 0);
        return TRUE;
    }
    catch (...)
    {
        return FALSE;
    }
}

BOOLEAN
SymbolDeleteSymTable()
{
    return TRUE;
}

BOOLEAN
SymbolBuildSymbolTable(PMODULE_SYMBOL_DETAIL * BufferToStoreDetails,
                       PUINT32                 StoredLength,
                       UINT32                  UserProcessId,
                       BOOLEAN                 SendOverSerial)
{
    return FALSE;
}

BOOLEAN
SymbolBuildAndUpdateSymbolTable(PMODULE_SYMBOL_DETAIL SymbolDetail)
{
    return FALSE;
}

VOID
SymbolInitialReload()
{
}

BOOLEAN
SymbolLocalReload(UINT32 UserProcessId)
{
    return FALSE;
}

VOID
SymbolPrepareDebuggerWithSymbolInfo(UINT32 UserProcessId)
{
}

BOOLEAN
SymbolReloadSymbolTableInDebuggerMode(UINT32 ProcessId)
{
    return FALSE;
}

#endif // __linux__
