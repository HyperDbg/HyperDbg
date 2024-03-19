/**
 * @file script-engine.h
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 * @brief Script engine parser and codegen
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#pragma once
#include "type.h"

#ifndef SCRIPT_ENGINE_H
#    define SCRIPT_ENGINE_H

//
// *** export pdb wrapper as script engine function ***
//
__declspec(dllexport) UINT64
    ScriptEngineConvertNameToAddress(const char * FunctionOrVariableName, PBOOLEAN WasFound);
__declspec(dllexport) UINT32
    ScriptEngineLoadFileSymbol(UINT64 BaseAddress, const char * PdbFileName, const char * CustomModuleName);
__declspec(dllexport) UINT32
    ScriptEngineUnloadAllSymbols();
__declspec(dllexport) UINT32
    ScriptEngineUnloadModuleSymbol(char * ModuleName);
__declspec(dllexport) UINT32
    ScriptEngineSearchSymbolForMask(const char * SearchMask);
__declspec(dllexport) BOOLEAN
    ScriptEngineGetFieldOffset(CHAR * TypeName, CHAR * FieldName, UINT32 * FieldOffset);
__declspec(dllexport) BOOLEAN
    ScriptEngineGetDataTypeSize(CHAR * TypeName, UINT64 * TypeSize);
__declspec(dllexport) BOOLEAN
    ScriptEngineCreateSymbolTableForDisassembler(void * CallbackFunction);
__declspec(dllexport) BOOLEAN
    ScriptEngineConvertFileToPdbPath(const char * LocalFilePath, char * ResultPath);
__declspec(dllexport) BOOLEAN
    ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetails(const char * LocalFilePath, char * PdbFilePath, char * GuidAndAgeDetails, BOOLEAN Is32BitModule);
__declspec(dllexport) BOOLEAN
    ScriptEngineSymbolInitLoad(PVOID BufferToStoreDetails, UINT32 StoredLength, BOOLEAN DownloadIfAvailable, const char * SymbolPath, BOOLEAN IsSilentLoad);
__declspec(dllexport) BOOLEAN
    ScriptEngineShowDataBasedOnSymbolTypes(const char * TypeName, UINT64 Address, BOOLEAN IsStruct, PVOID BufferAddress, const char * AdditionalParameters);
__declspec(dllexport) VOID
    ScriptEngineSymbolAbortLoading();
__declspec(dllexport) VOID
    ScriptEngineSetTextMessageCallback(PVOID Handler);

typedef enum _SCRIPT_ENGINE_ERROR_TYPE
{
    SCRIPT_ENGINE_ERROR_FREE,
    SCRIPT_ENGINE_ERROR_SYNTAX,
    SCRIPT_ENGINE_ERROR_UNKNOWN_TOKEN,
    SCRIPT_ENGINE_ERROR_UNRESOLVED_VARIABLE,
    SCRIPT_ENGINE_ERROR_UNHANDLED_SEMANTIC_RULE,
    SCRIPT_ENGINE_ERROR_TEMP_LIST_FULL,
    SCRIPT_ENGINE_ERROR_UNDEFINED_FUNCTION,
    SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE,
    SCRIPT_ENGINE_ERROR_VOID_FUNCTION_RETURNING_VALUE,
    SCRIPT_ENGINE_ERROR_NON_VOID_FUNCTION_NOT_RETURNING_VALUE
} SCRIPT_ENGINE_ERROR_TYPE,
    *PSCRIPT_ENGINE_ERROR_TYPE;

PSYMBOL
NewSymbol(void);

PSYMBOL
NewStringSymbol(PTOKEN Token);

PSYMBOL
NewWstringSymbol(PTOKEN Token);

PSYMBOL
NewFunctionSymbol(char * FunctionName, VARIABLE_TYPE * VariableType);

PSYMBOL
NewVariableSymbol(char * VariableName, VARIABLE_TYPE * VariableType);

unsigned int
GetSymbolHeapSize(PSYMBOL Symbol);

void
RemoveSymbol(PSYMBOL * Symbol);

__declspec(dllexport) void PrintSymbol(PSYMBOL Symbol);

PSYMBOL_BUFFER
NewSymbolBuffer(void);

__declspec(dllexport) void RemoveSymbolBuffer(PSYMBOL_BUFFER SymbolBuffer);

PSYMBOL_BUFFER
PushSymbol(PSYMBOL_BUFFER SymbolBuffer, const PSYMBOL Symbol);

__declspec(dllexport) void PrintSymbolBuffer(const PSYMBOL_BUFFER SymbolBuffer);

PSYMBOL
ToSymbol(PTOKEN PTOKEN, PSCRIPT_ENGINE_ERROR_TYPE Error);

__declspec(dllexport) PSYMBOL_BUFFER ScriptEngineParse(char * str);
__declspec(dllexport) PSYMBOL_BUFFER GetStackBuffer();

void
ScriptEngineBooleanExpresssionParse(
    UINT64                    BooleanExpressionSize,
    PTOKEN                    FirstToken,
    PTOKEN_LIST               MatchedStack,
    PSYMBOL_BUFFER            UserDefinedFunctions,
    PSYMBOL_BUFFER            CodeBuffer,
    char *                    str,
    char *                    c,
    PSCRIPT_ENGINE_ERROR_TYPE Error);

UINT64
BooleanExpressionExtractEnd(char * str, BOOL * WaitForWaitStatementBooleanExpression, PTOKEN CurrentIn);

void
CodeGen(PTOKEN_LIST MatchedStack, PSYMBOL_BUFFER UserDefinedFunctions, PSYMBOL_BUFFER CodeBuffer, PTOKEN Operator, PSCRIPT_ENGINE_ERROR_TYPE Error);

unsigned long long int
RegisterToInt(char * str);

unsigned long long int
PseudoRegToInt(char * str);

unsigned long long int
SemanticRuleToInt(char * str);

char *
HandleError(PSCRIPT_ENGINE_ERROR_TYPE Error, char * str);

int
GetGlobalIdentifierVal(PTOKEN PTOKEN);

int
GetLocalIdentifierVal(PTOKEN PTOKEN);

int
NewGlobalIdentifier(PTOKEN PTOKEN);

int
NewLocalIdentifier(PTOKEN PTOKEN);

int
LalrGetRhsSize(int RuleId);

BOOL
LalrIsOperandType(PTOKEN PTOKEN);

int
NewFunctionParameterIdentifier(PTOKEN Token);

int
GetFunctionParameterIdentifier(PTOKEN Token);
#endif
