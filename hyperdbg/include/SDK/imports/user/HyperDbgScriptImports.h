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

#ifdef HYPERDBG_SCRIPT_ENGINE
#    define IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE __declspec(dllexport)
#else
#    define IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE __declspec(dllimport)
#endif

//
// Header file of script-engine
// Imports
//
#ifdef __cplusplus
extern "C" {
#endif

//
// Hardware scripts
//
IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE VOID
HardwareScriptInterpreterShowScriptCapabilities(HWDBG_INSTANCE_INFORMATION * InstanceInfo);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
HardwareScriptInterpreterCheckScriptBufferWithScriptCapabilities(HWDBG_INSTANCE_INFORMATION * InstanceInfo,
                                                                 PVOID                        ScriptBuffer,
                                                                 UINT32                       CountOfScriptSymbolChunks,
                                                                 UINT32 *                     NumberOfStages,
                                                                 UINT32 *                     NumberOfOperands,
                                                                 UINT32 *                     NumberOfOperandsImplemented);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
HardwareScriptInterpreterCompressBuffer(UINT64 * Buffer,
                                        size_t   BufferLength,
                                        UINT32   ScriptVariableLength,
                                        UINT32   BramDataWidth,
                                        size_t * NewBufferSize,
                                        size_t * NumberOfBytesPerChunk);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
HardwareScriptInterpreterConvertSymbolToHwdbgShortSymbolBuffer(
    HWDBG_INSTANCE_INFORMATION * InstanceInfo,
    SYMBOL *                     SymbolBuffer,
    size_t                       SymbolBufferLength,
    UINT32                       NumberOfStages,
    HWDBG_SHORT_SYMBOL **        NewShortSymbolBuffer,
    size_t *                     NewBufferSize);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE VOID
HardwareScriptInterpreterFreeHwdbgShortSymbolBuffer(HWDBG_SHORT_SYMBOL * NewShortSymbolBuffer);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE PVOID
ScriptEngineParse(char * str);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
ScriptEngineSetHwdbgInstanceInfo(HWDBG_INSTANCE_INFORMATION * InstancInfo);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE void
PrintSymbolBuffer(const PVOID SymbolBuffer);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE void
RemoveSymbolBuffer(PVOID SymbolBuffer);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE void
PrintSymbol(PVOID Symbol);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE UINT64
ScriptEngineConvertNameToAddress(const char * FunctionOrVariableName, PBOOLEAN WasFound);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE UINT32
ScriptEngineLoadFileSymbol(UINT64 BaseAddress, const char * PdbFileName, const char * CustomModuleName);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE UINT32
ScriptEngineUnloadAllSymbols();

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE UINT32
ScriptEngineUnloadModuleSymbol(char * ModuleName);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE UINT32
ScriptEngineSearchSymbolForMask(const char * SearchMask);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
ScriptEngineGetFieldOffset(CHAR * TypeName, CHAR * FieldName, UINT32 * FieldOffset);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
ScriptEngineGetDataTypeSize(CHAR * TypeName, UINT64 * TypeSize);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
ScriptEngineCreateSymbolTableForDisassembler(void * CallbackFunction);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
ScriptEngineConvertFileToPdbPath(const char * LocalFilePath, char * ResultPath, size_t ResultPathSize);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetails(const char * LocalFilePath, char * PdbFilePath, char * GuidAndAgeDetails, BOOLEAN Is32BitModule);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
ScriptEngineSymbolInitLoad(PVOID BufferToStoreDetails, UINT32 StoredLength, BOOLEAN DownloadIfAvailable, const char * SymbolPath, BOOLEAN IsSilentLoad);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
ScriptEngineShowDataBasedOnSymbolTypes(const char * TypeName, UINT64 Address, BOOLEAN IsStruct, PVOID BufferAddress, const char * AdditionalParameters);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE VOID
ScriptEngineSymbolAbortLoading();

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE VOID
ScriptEngineSetTextMessageCallback(PVOID Handler);

#ifdef __cplusplus
}
#endif
