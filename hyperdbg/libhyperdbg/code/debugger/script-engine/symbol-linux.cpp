/**
 * @file symbol-linux.cpp
 * @author Max Raulea (max.raulea@hyperdbg.org)
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
 * @brief check and convert string to a 64 bit unsigned integer and also
 *  check for symbol object names and evaluate expressions
 * @details Same as the Windows implementation in symbol.cpp. Symbol names
 *          aren't resolved on Linux yet (the name lookup always reports not
 *          found), but numbers and expressions (e.g., @rax, @rsp+10) are
 *
 * @param TextToConvert the target string
 * @param Result result will be save to the pointer
 *
 * @return BOOLEAN shows whether the conversion was successful or not
 */
BOOLEAN
SymbolConvertNameOrExprToAddress(const string & TextToConvert, PUINT64 Result)
{
    BOOLEAN IsFound  = FALSE;
    BOOLEAN HasError = FALSE;
    UINT64  Address  = 0;

    if (ConvertStringToUInt64(TextToConvert, &Address))
    {
        //
        // It's a hex number
        //
        IsFound = TRUE;
    }
    else
    {
        //
        // Check for symbol object names
        //
        Address = ScriptEngineConvertNameToAddressWrapper(TextToConvert.c_str(), &IsFound);

        if (!IsFound)
        {
            //
            // As the last resort, test whether it's an expression; in the
            // Debugger Mode it's sent to the debuggee to be evaluated
            //
            Address = ScriptEngineEvalSingleExpression(TextToConvert, &HasError);
            IsFound = !HasError;
        }
    }

    if (IsFound)
    {
        *Result = Address;
    }

    return IsFound;
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
