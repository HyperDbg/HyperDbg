#pragma once

//
// Header file of symbol-parser
// Imports
//
#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllimport) VOID
    SymSetTextMessageCallback(PVOID Handler);
__declspec(dllimport) UINT64
    SymConvertNameToAddress(const char * FunctionOrVariableName, PBOOLEAN WasFound);
__declspec(dllimport) UINT32
    SymLoadFileSymbol(UINT64 BaseAddress, const char * PdbFileName);
__declspec(dllimport) UINT32
    SymUnloadAllSymbols();
__declspec(dllimport) UINT32
    SymUnloadModuleSymbol(char * ModuleName);
__declspec(dllimport) UINT32
    SymSearchSymbolForMask(const char * SearchMask);
__declspec(dllimport) BOOLEAN
    SymGetFieldOffset(CHAR * TypeName, CHAR * FieldName, UINT32 * FieldOffset);
__declspec(dllimport) BOOLEAN
    SymGetDataTypeSize(CHAR * TypeName, UINT64 * TypeSize);
__declspec(dllimport) BOOLEAN
    SymCreateSymbolTableForDisassembler(void * CallbackFunction);
__declspec(dllimport) BOOLEAN
    SymConvertFileToPdbPath(const char * LocalFilePath, char * ResultPath);
__declspec(dllimport) BOOLEAN
    SymConvertFileToPdbFileAndGuidAndAgeDetails(const char * LocalFilePath,
                                                char *       PdbFilePath,
                                                char *       GuidAndAgeDetails);
__declspec(dllimport) BOOLEAN
    SymbolInitLoad(PVOID        BufferToStoreDetails,
                   UINT32       StoredLength,
                   BOOLEAN      DownloadIfAvailable,
                   const char * SymbolPath,
                   BOOLEAN      IsSilentLoad);
__declspec(dllimport) BOOLEAN
    SymShowDataBasedOnSymbolTypes(const char * TypeName,
                                  UINT64       Address,
                                  BOOLEAN      IsStruct,
                                  PVOID        BufferAddress,
                                  const char * AdditionalParameters);
__declspec(dllimport) BOOLEAN
    SymQuerySizeof(_In_ const char * StructNameOrTypeName, _Out_ UINT32 * SizeOfField);
__declspec(dllimport) BOOLEAN
    SymCastingQueryForFiledsAndTypes(_In_ const char * StructName,
                                     _In_ const char * FiledOfStructName,
                                     _Out_ PBOOLEAN    IsStructNamePointerOrNot,
                                     _Out_ PBOOLEAN    IsFiledOfStructNamePointerOrNot,
                                     _Out_ char **     NewStructOrTypeName,
                                     _Out_ UINT32 *    OffsetOfFieldFromTop,
                                     _Out_ UINT32 *    SizeOfField);

#ifdef __cplusplus
}
#endif
