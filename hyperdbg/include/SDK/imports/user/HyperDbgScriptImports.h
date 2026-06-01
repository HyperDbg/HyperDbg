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
                                        SIZE_T   BufferLength,
                                        UINT32   ScriptVariableLength,
                                        UINT32   BramDataWidth,
                                        SIZE_T * NewBufferSize,
                                        SIZE_T * NumberOfBytesPerChunk);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
HardwareScriptInterpreterConvertSymbolToHwdbgShortSymbolBuffer(
    HWDBG_INSTANCE_INFORMATION * InstanceInfo,
    SYMBOL *                     SymbolBuffer,
    SIZE_T                       SymbolBufferLength,
    UINT32                       NumberOfStages,
    HWDBG_SHORT_SYMBOL **        NewShortSymbolBuffer,
    SIZE_T *                     NewBufferSize);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE VOID
HardwareScriptInterpreterFreeHwdbgShortSymbolBuffer(HWDBG_SHORT_SYMBOL * NewShortSymbolBuffer);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE PVOID
ScriptEngineParse(CHAR * Str);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
ScriptEngineSetHwdbgInstanceInfo(HWDBG_INSTANCE_INFORMATION * InstancInfo);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE VOID
PrintSymbolBuffer(const PVOID SymbolBuffer);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE VOID
RemoveSymbolBuffer(PVOID SymbolBuffer);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE VOID
PrintSymbol(PVOID Symbol);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE UINT64
ScriptEngineConvertNameToAddress(const CHAR * FunctionOrVariableName, PBOOLEAN WasFound);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE UINT32
ScriptEngineLoadFileSymbol(UINT64 BaseAddress, const CHAR * PdbFileName, const CHAR * CustomModuleName);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE UINT32
ScriptEngineUnloadAllSymbols();

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE UINT32
ScriptEngineUnloadModuleSymbol(CHAR * ModuleName);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE UINT32
ScriptEngineSearchSymbolForMask(const CHAR * SearchMask);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
ScriptEngineGetFieldOffset(CHAR * TypeName, CHAR * FieldName, UINT32 * FieldOffset);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
ScriptEngineGetDataTypeSize(CHAR * TypeName, UINT64 * TypeSize);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
ScriptEngineCreateSymbolTableForDisassembler(PVOID CallbackFunction);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
ScriptEngineConvertFileToPdbPath(const CHAR * LocalFilePath, CHAR * ResultPath, SIZE_T ResultPathSize);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetails(const CHAR * LocalFilePath, CHAR * PdbFilePath, CHAR * GuidAndAgeDetails, BOOLEAN Is32BitModule);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
ScriptEngineSymbolInitLoad(PVOID BufferToStoreDetails, UINT32 StoredLength, BOOLEAN DownloadIfAvailable, const CHAR * SymbolPath, BOOLEAN IsSilentLoad);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE BOOLEAN
ScriptEngineShowDataBasedOnSymbolTypes(const CHAR * TypeName, UINT64 Address, BOOLEAN IsStruct, PVOID BufferAddress, const CHAR * AdditionalParameters);

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE VOID
ScriptEngineSymbolAbortLoading();

IMPORT_EXPORT_HYPERDBG_SCRIPT_ENGINE VOID
ScriptEngineSetTextMessageCallback(PVOID Handler);

#ifdef __cplusplus
}
#endif
