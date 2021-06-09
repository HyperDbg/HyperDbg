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
//			 For symbol (pdb) parsing		    //
//////////////////////////////////////////////////

BOOLEAN
SymbolConvertNameToAddress(string TextToConvert, PUINT64 Result);

BOOLEAN
SymbolLoadNtoskrnlSymbol(UINT64 BaseAddress);

BOOLEAN
SymbolReloadLoadedModulesInformation(PVOID BufferToStoreDetails, PUINT StoredLength);
