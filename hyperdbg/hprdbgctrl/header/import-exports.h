/**
 * @file import-exports.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg exported functions
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					  Exports                   //
//////////////////////////////////////////////////

//
// Exports
//
extern "C" {

extern bool inline AsmVmxSupportDetection();

//
// VMM Module
//
__declspec(dllexport) int HyperDbgLoadVmm();
__declspec(dllexport) int HyperDbgUnloadVmm();
__declspec(dllexport) int HyperDbgInstallVmmDriver();
__declspec(dllexport) int HyperDbgUninstallVmmDriver();
__declspec(dllexport) int HyperDbgStopVmmDriver();

//
// General exports
//
__declspec(dllexport) int HyperDbgInterpreter(char * Command);
__declspec(dllexport) void HyperDbgShowSignature();
__declspec(dllexport) void HyperdbgSetTextMessageCallback(Callback handler);
__declspec(dllexport) void HyperDbgScriptReadFileAndExecuteCommand(std::vector<std::string> & PathAndArgs);
__declspec(dllexport) bool HyperDbgContinuePreviousCommand();
__declspec(dllexport) bool HyperDbgCheckMultilineCommand(std::string & CurrentCommand, bool Reset);
}

//////////////////////////////////////////////////
//            	     Imports                    //
//////////////////////////////////////////////////

extern "C" {
__declspec(dllimport) PSYMBOL_BUFFER ScriptEngineParse(char * str);
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
    ScriptEngineLoadFileSymbol(UINT64 BaseAddress, const char * PdbFileName);
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
    ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetails(const char * LocalFilePath, char * PdbFilePath, char * GuidAndAgeDetails);
__declspec(dllimport) BOOLEAN
    ScriptEngineSymbolInitLoad(PVOID BufferToStoreDetails, UINT32 StoredLength, BOOLEAN DownloadIfAvailable, const char * SymbolPath, BOOLEAN IsSilentLoad);
__declspec(dllimport) BOOLEAN
    ScriptEngineShowDataBasedOnSymbolTypes(const char * TypeName, UINT64 Address, BOOLEAN IsStruct, PVOID BufferAddress, const char * AdditionalParameters);
}
