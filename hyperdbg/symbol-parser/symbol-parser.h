/**
 * @file symbol-parser.h
 * @author Sina Karvandi (sina@rayanfam.com)
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
    UINT64  BaseAddress;
    DWORD64 ModuleBase;
    char    ModuleName[_MAX_FNAME];

} SYMBOL_LOADED_MODULE_DETAILS, *PSYMBOL_LOADED_MODULE_DETAILS;

//////////////////////////////////////////////////
//					Exports                     //
//////////////////////////////////////////////////
extern "C" {
__declspec(dllexport) UINT32 SymLoadFileSymbol(UINT64 BaseAddress, const char * PdbFileName);
__declspec(dllexport) UINT32 SymUnloadAllSymbols();
__declspec(dllexport) UINT32 SymUnloadModuleSymbol(char * ModuleName);
__declspec(dllexport) UINT32 SymSearchSymbolForMask(const char * SearchMask);
__declspec(dllexport) UINT64 SymConvertNameToAddress(const char * FunctionOrVariableName, PBOOLEAN WasFound);
__declspec(dllexport) BOOLEAN SymConvertFileToPdbPath(const char * LocalFilePath, char * ResultPath);
__declspec(dllexport) BOOLEAN SymConvertFileToPdbFileAndGuidAndAgeDetails(const char * LocalFilePath, char * PdbFilePath, char * GuidAndAgeDetails);
__declspec(dllexport) BOOLEAN SymbolInitLoad(PMODULE_SYMBOL_DETAIL BufferToStoreDetails, UINT32 StoredLength, BOOLEAN DownloadIfAvailable, const char * SymbolPath);
}

//////////////////////////////////////////////////
//					Functions                   //
//////////////////////////////////////////////////

BOOL
SymGetFileParams(const char * FileName, DWORD & FileSize);

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

BOOLEAN
SymbolPDBDownload(std::string SymName, std::string GUID, std::string SymPath);
