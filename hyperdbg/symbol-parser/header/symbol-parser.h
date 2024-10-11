/**
 * @file symbol-parser.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief symbol parser headers
 * @details
 * @version 0.1
 * @date 2021-05-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Configs                     //
//////////////////////////////////////////////////

#define DoNotShowDetailedResult TRUE

//////////////////////////////////////////////////
//					Structures                  //
//////////////////////////////////////////////////

/**
 * @brief Hold detail about the loaded modules
 *
 */
typedef struct _SYMBOL_LOADED_MODULE_DETAILS
{
    UINT64 BaseAddress;
    UINT64 ModuleBase;
    char   ModuleName[_MAX_FNAME];
    char   ModuleAlternativeName[_MAX_FNAME];
    char   PdbFilePath[MAX_PATH];

} SYMBOL_LOADED_MODULE_DETAILS, *PSYMBOL_LOADED_MODULE_DETAILS;

//////////////////////////////////////////////////
//				Exports & Imports               //
//////////////////////////////////////////////////
extern "C" {

//
// Imports
//
__declspec(dllimport) int
pdbex_export(int argc, char ** argv, bool is_struct, void * buffer_address);
__declspec(dllimport) void
pdbex_set_logging_method_export(PVOID handler);
}

//////////////////////////////////////////////////
//					Functions                   //
//////////////////////////////////////////////////

BOOL
SymGetFileParams(const char * FileName, DWORD & FileSize);

BOOL
SymGetFileSize(const char * FileName, DWORD & FileSize);

VOID
SymShowSymbolInfo(UINT64 ModBase);

BOOL CALLBACK
SymDisplayMaskSymbolsCallback(SYMBOL_INFO * SymInfo, ULONG SymbolSize, PVOID UserContext);

BOOL CALLBACK
SymDeliverDisassemblerSymbolMapCallback(SYMBOL_INFO * SymInfo, ULONG SymbolSize, PVOID UserContext);

VOID
SymShowSymbolDetails(SYMBOL_INFO & SymInfo);

const char *
SymTagStr(ULONG Tag);

BOOLEAN
SymbolPdbDownload(std::string SymName, const std::string & GUID, const std::string & SymPath, BOOLEAN IsSilentLoad);
