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
//		        	 Structures	        	    //
//////////////////////////////////////////////////

/**
 * @brief structures for sending and saving details
 * about each module and symbols details
 *
 */
typedef struct _MODULE_SYMBOL_DETAIL
{
    BOOLEAN IsSymbolDetailsFound; // TRUE if the details of symbols found, FALSE if not found
    BOOLEAN IsRealSymbolPath;     // TRUE if the ModuleSymbolPath is a real path
                                  // and FALSE if ModuleSymbolPath is just a module name
    UINT64 BaseAddress;
    char   FilePath[MAX_PATH];
    char   ModuleSymbolPath[MAX_PATH];
    char   ModuleSymbolGuidAndAge[MAXIMUM_GUID_AND_AGE_SIZE];

} MODULE_SYMBOL_DETAIL, *PMODULE_SYMBOL_DETAIL;

//////////////////////////////////////////////////
//			 For symbol (pdb) parsing		    //
//////////////////////////////////////////////////

BOOLEAN
SymbolConvertNameToAddress(string TextToConvert, PUINT64 Result);

BOOLEAN
SymbolLoadNtoskrnlSymbol(UINT64 BaseAddress);

BOOLEAN
SymbolBuildSymbolTable(PMODULE_SYMBOL_DETAIL * BufferToStoreDetails, PUINT32 StoredLength);
