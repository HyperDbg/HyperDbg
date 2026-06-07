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

#ifdef _WIN32
    // MSVC (Windows)
#   ifdef HYPERDBG_SYMBOL_PARSER
#       define IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER __declspec(dllexport)
#   else
#       define IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER __declspec(dllimport)
#   endif
#else
    // GCC/Clang (Linux)
#   ifdef HYPERDBG_SYMBOL_PARSER
#       define IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER __attribute__((visibility("default")))
#   else
#       define IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER
#   endif
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
SymConvertNameToAddress(const CHAR * FunctionOrVariableName, PBOOLEAN WasFound);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER UINT32
SymLoadFileSymbol(UINT64 BaseAddress, const CHAR * PdbFileName, const CHAR * CustomModuleName);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER UINT32
SymUnloadAllSymbols();

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER UINT32
SymUnloadModuleSymbol(CHAR * ModuleName);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER UINT32
SymSearchSymbolForMask(const CHAR * SearchMask);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymGetFieldOffset(CHAR * TypeName, CHAR * FieldName, UINT32 * FieldOffset);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymGetDataTypeSize(CHAR * TypeName, UINT64 * TypeSize);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymCreateSymbolTableForDisassembler(PVOID CallbackFunction);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymConvertFileToPdbPath(const CHAR * LocalFilePath, CHAR * ResultPath, SIZE_T ResultPathSize);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymConvertFileToPdbFileAndGuidAndAgeDetails(const CHAR * LocalFilePath,
                                            CHAR *       PdbFilePath,
                                            CHAR *       GuidAndAgeDetails,
                                            BOOLEAN      Is32BitModule);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymConvertLoadedModuleToPdbFileAndGuidAndAgeDetails(const BYTE * LoadedImageBytes,
                                                    SIZE_T       LoadedImageSize,
                                                    const CHAR * LocalFilePath,
                                                    CHAR *       PdbFilePath,
                                                    CHAR *       GuidAndAgeDetails,
                                                    BOOLEAN      Is32BitModule);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymbolInitLoad(PVOID        BufferToStoreDetails,
               UINT32       StoredLength,
               BOOLEAN      DownloadIfAvailable,
               const CHAR * SymbolPath,
               BOOLEAN      IsSilentLoad);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymShowDataBasedOnSymbolTypes(const CHAR * TypeName,
                              UINT64       Address,
                              BOOLEAN      IsStruct,
                              PVOID        BufferAddress,
                              const CHAR * AdditionalParameters);

IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymQuerySizeof(_In_ const CHAR * StructNameOrTypeName, _Out_ UINT32 * SizeOfField);
IMPORT_EXPORT_HYPERDBG_SYMBOL_PARSER BOOLEAN
SymCastingQueryForFiledsAndTypes(_In_ const CHAR * StructName,
                                 _In_ const CHAR * FiledOfStructName,
                                 _Out_ PBOOLEAN    IsStructNamePointerOrNot,
                                 _Out_ PBOOLEAN    IsFiledOfStructNamePointerOrNot,
                                 _Out_ CHAR **     NewStructOrTypeName,
                                 _Out_ UINT32 *    OffsetOfFieldFromTop,
                                 _Out_ UINT32 *    SizeOfField);

#ifdef __cplusplus
}
#endif
