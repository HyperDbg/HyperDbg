/**
 * @file symbol.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Symbol related functions header
 * @details
 * @version 0.1
 * @date 2021-06-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//			        Structures		            //
//////////////////////////////////////////////////

/**
 * @brief Save the local function symbols' description
 *
 */
typedef struct _LOCAL_FUNCTION_DESCRIPTION
{
    std::string ObjectName;
    UINT32      ObjectSize;

} LOCAL_FUNCTION_DESCRIPTION, *PLOCAL_FUNCTION_DESCRIPTION;

//////////////////////////////////////////////////
//			    	    Pdbex                   //
//////////////////////////////////////////////////

#define PDBEX_DEFAULT_CONFIGURATION "-j- -k- -e n -i"

//////////////////////////////////////////////////
//			 For symbol (pdb) parsing		    //
//////////////////////////////////////////////////

VOID
SymbolBuildAndShowSymbolTable();

BOOLEAN
SymbolShowFunctionNameBasedOnAddress(UINT64 Address, PUINT64 UsedBaseAddress);

BOOLEAN
SymbolLoadOrDownloadSymbols(BOOLEAN IsDownload, BOOLEAN SilentLoad);

BOOLEAN
SymbolConvertNameOrExprToAddress(const string & TextToConvert, PUINT64 Result);

BOOLEAN
SymbolDeleteSymTable();

BOOLEAN
SymbolBuildSymbolTable(PMODULE_SYMBOL_DETAIL * BufferToStoreDetails,
                       PUINT32                 StoredLength,
                       UINT32                  UserProcessId,
                       BOOLEAN                 SendOverSerial);

BOOLEAN
SymbolBuildAndUpdateSymbolTable(PMODULE_SYMBOL_DETAIL SymbolDetail);

VOID
SymbolInitialReload();

BOOLEAN
SymbolLocalReload(UINT32 UserProcessId);

VOID
SymbolPrepareDebuggerWithSymbolInfo(UINT32 UserProcessId);

BOOLEAN
SymbolReloadSymbolTableInDebuggerMode(UINT32 ProcessId);
