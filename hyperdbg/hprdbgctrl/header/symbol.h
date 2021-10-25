/**
 * @file symbol.h
 * @author Sina Karvandi (sina@rayanfam.com)
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
//			 For symbol (pdb) parsing		    //
//////////////////////////////////////////////////

VOID
SymbolBuildAndShowSymbolTable(BOOLEAN BuildLocalSymTable);

BOOLEAN
SymbolShowFunctionNameBasedOnAddress(UINT64 Address, PUINT64 UsedBaseAddress);

BOOLEAN
SymbolLoadOrDownloadSymbols(BOOLEAN IsDownload, BOOLEAN SilentLoad);

BOOLEAN
SymbolConvertNameOrExprToAddress(string TextToConvert, PUINT64 Result);

BOOLEAN
SymbolBuildSymbolTable(PMODULE_SYMBOL_DETAIL * BufferToStoreDetails,
                       PUINT32                 StoredLength,
                       BOOLEAN                 SendOverSerial);

BOOLEAN
SymbolBuildAndUpdateSymbolTable(PMODULE_SYMBOL_DETAIL SymbolDetail);

VOID
SymbolInitialReload();

BOOLEAN
SymbolLocalReload();

VOID
SymbolPrepareDebuggerWithSymbolInfo();

BOOLEAN
SymbolReloadSymbolTableInDebuggerMode();
