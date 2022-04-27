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
    char   PdbFilePath[MAX_PATH];

} SYMBOL_LOADED_MODULE_DETAILS, *PSYMBOL_LOADED_MODULE_DETAILS;

//////////////////////////////////////////////////
//				Exports & Imports               //
//////////////////////////////////////////////////
extern "C" {

//
// Imports
//
__declspec(dllimport) int pdbex_export(int argc, char ** argv, bool is_struct, void * buffer_address);
__declspec(dllimport) void pdbex_set_logging_method_export(PVOID handler);

//
// Exports
//
__declspec(dllexport) VOID SymSetTextMessageCallback(PVOID handler);
__declspec(dllexport) UINT32 SymLoadFileSymbol(UINT64 BaseAddress, const char * PdbFileName);
__declspec(dllexport) UINT32 SymUnloadAllSymbols();
__declspec(dllexport) UINT32 SymUnloadModuleSymbol(char * ModuleName);
__declspec(dllexport) UINT32 SymSearchSymbolForMask(const char * SearchMask);
__declspec(dllexport) BOOLEAN SymGetFieldOffset(CHAR * TypeName, CHAR * FieldName, UINT32 * FieldOffset);
__declspec(dllexport) BOOLEAN SymGetDataTypeSize(CHAR * TypeName, UINT64 * TypeSize);
__declspec(dllexport) BOOLEAN SymCreateSymbolTableForDisassembler(void * CallbackFunction);
__declspec(dllexport) UINT64 SymConvertNameToAddress(const char * FunctionOrVariableName, PBOOLEAN WasFound);
__declspec(dllexport) BOOLEAN SymConvertFileToPdbPath(const char * LocalFilePath, char * ResultPath);
__declspec(dllexport) BOOLEAN SymConvertFileToPdbFileAndGuidAndAgeDetails(const char * LocalFilePath, char * PdbFilePath, char * GuidAndAgeDetails);
__declspec(dllexport) BOOLEAN SymbolInitLoad(PVOID BufferToStoreDetails, UINT32 StoredLength, BOOLEAN DownloadIfAvailable, const char * SymbolPath, BOOLEAN IsSilentLoad);
__declspec(dllexport) BOOLEAN SymShowDataBasedOnSymbolTypes(const char * TypeName, UINT64 Address, BOOLEAN IsStruct, PVOID BufferAddress, const char * AdditionalParameters);
__declspec(dllexport) VOID SymbolAbortLoading();
__declspec(dllexport) BOOLEAN SymQuerySizeof(_In_ const char * StructNameOrTypeName, _Out_ UINT32 * SizeOfField);
__declspec(dllexport) BOOLEAN SymCastingQueryForFiledsAndTypes(_In_ const char * StructName, _In_ const char * FiledOfStructName, _Out_ PBOOLEAN IsStructNamePointerOrNot, _Out_ PBOOLEAN IsFiledOfStructNamePointerOrNot, _Out_ char ** NewStructOrTypeName, _Out_ UINT32 * OffsetOfFieldFromTop, _Out_ UINT32 * SizeOfField);
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
SymbolPDBDownload(std::string SymName, const std::string & GUID, const std::string & SymPath, BOOLEAN IsSilentLoad);

VOID
SymbolAbortLoading();
