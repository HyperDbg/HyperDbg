/**
 * @file script-engine.h
 * @author M.H. Gholamrezaei (gholamrezaei.mh@gmail.com)
 * @brief Script engine parser and codegen
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#ifndef SCRIPT_ENGINE_H
#    define SCRIPT_ENGINE_H

//
// *** import pdb parser functions ***
//
__declspec(dllimport) UINT64 SymConvertNameToAddress(const char * FunctionOrVariableName, PBOOLEAN WasFound);
__declspec(dllimport) UINT32 SymLoadFileSymbol(UINT64 BaseAddress, const char * PdbFileName);
__declspec(dllimport) UINT32 SymUnloadAllSymbols();
__declspec(dllimport) UINT32 SymUnloadModuleSymbol(char * ModuleName);
__declspec(dllimport) UINT32 SymSearchSymbolForMask(const char * SearchMask);
__declspec(dllimport) BOOLEAN SymConvertFileToPdbPath(const char * LocalFilePath, char * ResultPath);
__declspec(dllimport) BOOLEAN SymConvertFileToPdbFileAndGuidAndAgeDetails(const char * LocalFilePath, char * PdbFilePath, char * GuidAndAgeDetails);
__declspec(dllimport) BOOLEAN SymbolInitLoad(PVOID BufferToStoreDetails, UINT32 StoredLength, BOOLEAN DownloadIfAvailable, const char * SymbolPath, BOOLEAN IsSilentLoad);

//
// *** export pdb wrapper as script engine function ***
//
__declspec(dllexport) UINT64
    ScriptEngineConvertNameToAddress(const char * FunctionOrVariableName, PBOOLEAN WasFound);
__declspec(dllexport) UINT32
    ScriptEngineLoadFileSymbol(UINT64 BaseAddress, const char * PdbFileName);
__declspec(dllexport) UINT32
    ScriptEngineUnloadAllSymbols();
__declspec(dllexport) UINT32
    ScriptEngineUnloadModuleSymbol(char * ModuleName);
__declspec(dllexport) UINT32
    ScriptEngineSearchSymbolForMask(const char * SearchMask);
__declspec(dllexport) BOOLEAN
    ScriptEngineConvertFileToPdbPath(const char * LocalFilePath, char * ResultPath);
__declspec(dllexport) BOOLEAN
    ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetails(const char * LocalFilePath, char * PdbFilePath, char * GuidAndAgeDetails);
__declspec(dllexport) BOOLEAN
    ScriptEngineSymbolInitLoad(PVOID BufferToStoreDetails, UINT32 StoredLength, BOOLEAN DownloadIfAvailable, const char * SymbolPath, BOOLEAN IsSilentLoad);

typedef enum _SCRIPT_ENGINE_ERROR_TYPE
{
    SCRIPT_ENGINE_ERROR_FREE,
    SCRIPT_ENGINE_ERROR_SYNTAX,
    SCRIPT_ENGINE_ERROR_UNKOWN_TOKEN,
    SCRIPT_ENGINE_ERROR_UNRESOLVED_VARIABLE,
    SCRIPT_ENGINE_ERROR_UNHANDLED_SEMANTIC_RULE
} SCRIPT_ENGINE_ERROR_TYPE,
    *PSCRIPT_ENGINE_ERROR_TYPE;

PSYMBOL
NewSymbol(void);

PSYMBOL
NewStringSymbol(char * value);

unsigned int
GetStringSymbolSize(PSYMBOL Symbol);

void
RemoveSymbol(PSYMBOL Symbol);

__declspec(dllexport) void PrintSymbol(PSYMBOL Symbol);

PSYMBOL_BUFFER
NewSymbolBuffer(void);

__declspec(dllexport) void RemoveSymbolBuffer(PSYMBOL_BUFFER SymbolBuffer);

PSYMBOL_BUFFER
PushSymbol(PSYMBOL_BUFFER SymbolBuffer, const PSYMBOL Symbol);

__declspec(dllexport) void PrintSymbolBuffer(const PSYMBOL_BUFFER SymbolBuffer);

PSYMBOL
ToSymbol(TOKEN Token, PSCRIPT_ENGINE_ERROR_TYPE Error);

__declspec(dllexport) PSYMBOL_BUFFER ScriptEngineParse(char * str);

char *
ScriptEngineBooleanExpresssionParse(
    UINT64                     BooleanExpressionSize,
    TOKEN                      FirstToken,
    TOKEN_LIST                 MatchedStack,
    PSYMBOL_BUFFER             CodeBuffer,
    char *                     str,
    char *                     c,
    PSCRIPT_ENGINE_ERROR_TYPE  Error);

UINT64
BooleanExpressionExtractEnd(char * str, BOOL * WaitForWaitStatementBooleanExpression);

void
CodeGen(TOKEN_LIST MatchedStack, PSYMBOL_BUFFER CodeBuffer, TOKEN Operator, PSCRIPT_ENGINE_ERROR_TYPE Error);

unsigned long long int
RegisterToInt(char * str);

unsigned long long int
PseudoRegToInt(char * str);

unsigned long long int
SemanticRuleToInt(char * str);

char *
HandleError(PSCRIPT_ENGINE_ERROR_TYPE Error, char * str);

int
GetIdentifireVal(TOKEN Token);

int
NewIdentifire(TOKEN Token);

int
LalrGetRhsSize(int RuleId);

BOOL
LalrIsOperandType(TOKEN Token);

#endif
