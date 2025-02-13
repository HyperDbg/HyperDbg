/**
 * @file HyperDbgSymImports.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating exported functions from symbol parser
 * @version 0.2
 * @date 2023-02-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#ifdef HYPERDBG_SYMBOL_PARSER
#    define IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER __declspec(dllexport)
#else
#    define IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER __declspec(dllimport)
#endif

//
// Header file of symbol-parser
// Imports
//
#ifdef __cplusplus
extern "C" {
#endif

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER VOID
SymSetTextMessageCallback(PVOID Handler);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER VOID
SymbolAbortLoading();

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER UINT64
SymConvertNameToAddress(const char * FunctionOrVariableName, PBOOLEAN WasFound);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER UINT32
SymLoadFileSymbol(UINT64 BaseAddress, const char * PdbFileName, const char * CustomModuleName);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER UINT32
SymUnloadAllSymbols();

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER UINT32
SymUnloadModuleSymbol(char * ModuleName);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER UINT32
SymSearchSymbolForMask(const char * SearchMask);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymGetFieldOffset(CHAR * TypeName, CHAR * FieldName, UINT32 * FieldOffset);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymGetDataTypeSize(CHAR * TypeName, UINT64 * TypeSize);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymCreateSymbolTableForDisassembler(void * CallbackFunction);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymConvertFileToPdbPath(const char * LocalFilePath, char * ResultPath, size_t ResultPathSize);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymConvertFileToPdbFileAndGuidAndAgeDetails(const char * LocalFilePath,
                                            char *       PdbFilePath,
                                            char *       GuidAndAgeDetails,
                                            BOOLEAN      Is32BitModule);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymbolInitLoad(PVOID        BufferToStoreDetails,
               UINT32       StoredLength,
               BOOLEAN      DownloadIfAvailable,
               const char * SymbolPath,
               BOOLEAN      IsSilentLoad);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymShowDataBasedOnSymbolTypes(const char * TypeName,
                              UINT64       Address,
                              BOOLEAN      IsStruct,
                              PVOID        BufferAddress,
                              const char * AdditionalParameters);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymQuerySizeof(_In_ const char * StructNameOrTypeName, _Out_ UINT32 * SizeOfField);
IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
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
