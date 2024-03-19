/**
 * @file HyperDbgScriptImports.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating exported functions from script engine
 * @version 0.2
 * @date 2023-02-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//
// Header file of script-engine
// Imports
//
#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllimport) PSYMBOL_BUFFER ScriptEngineParse(char * str);
__declspec(dllimport) PSYMBOL_BUFFER GetStackBuffer();
__declspec(dllimport) void PrintSymbolBuffer(const PSYMBOL_BUFFER SymbolBuffer);
__declspec(dllimport) void PrintSymbol(PSYMBOL Symbol);
__declspec(dllimport) void RemoveSymbolBuffer(PSYMBOL_BUFFER SymbolBuffer);

//
// pdb parser
//
__declspec(dllimport) VOID
    ScriptEngineSetTextMessageCallback(PVOID Handler);
__declspec(dllimport) VOID
    ScriptEngineSymbolAbortLoading();
__declspec(dllimport) UINT64
    ScriptEngineConvertNameToAddress(const char * FunctionOrVariableName, PBOOLEAN WasFound);
__declspec(dllimport) UINT32
    ScriptEngineLoadFileSymbol(UINT64 BaseAddress, const char * PdbFileName, const char * CustomModuleName);
__declspec(dllimport) UINT32
    ScriptEngineUnloadAllSymbols();
__declspec(dllimport) UINT32
    ScriptEngineUnloadModuleSymbol(char * ModuleName);
__declspec(dllimport) UINT32
    ScriptEngineSearchSymbolForMask(const char * SearchMask);
__declspec(dllimport) BOOLEAN
    ScriptEngineGetFieldOffset(CHAR * TypeName, CHAR * FieldName, UINT32 * FieldOffset);
__declspec(dllimport) BOOLEAN
    ScriptEngineGetDataTypeSize(CHAR * TypeName, UINT64 * TypeSize);
__declspec(dllimport) BOOLEAN
    ScriptEngineCreateSymbolTableForDisassembler(void * CallbackFunction);
__declspec(dllimport) BOOLEAN
    ScriptEngineConvertFileToPdbPath(const char * LocalFilePath, char * ResultPath);
__declspec(dllimport) BOOLEAN
    ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetails(const char * LocalFilePath, char * PdbFilePath, char * GuidAndAgeDetails, BOOLEAN Is32BitModule);
__declspec(dllimport) BOOLEAN
    ScriptEngineSymbolInitLoad(PVOID BufferToStoreDetails, UINT32 StoredLength, BOOLEAN DownloadIfAvailable, const char * SymbolPath, BOOLEAN IsSilentLoad);
__declspec(dllimport) BOOLEAN
    ScriptEngineShowDataBasedOnSymbolTypes(const char * TypeName, UINT64 Address, BOOLEAN IsStruct, PVOID BufferAddress, const char * AdditionalParameters);

#ifdef __cplusplus
}
#endif
