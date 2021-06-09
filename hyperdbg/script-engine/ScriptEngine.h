/**
 * @file ScriptEngine.h
 * @author M.H. Gholamrezei (gholamrezaei.mh@gmail.com)
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

#    include <stdio.h>
#    include "ScriptEngineCommon.h"
#    include "scanner.h"
#    include "common.h"

//
// *** import pdb parser functions ***
//
__declspec(dllimport) UINT64 SymConvertNameToAddress(const char * FunctionOrVariableName, PBOOLEAN WasFound);
__declspec(dllimport) UINT32 SymLoadFileSymbol(UINT64 BaseAddress, const char * PdbFileName);
__declspec(dllimport) UINT32 SymUnloadAllSymbols();
__declspec(dllimport) UINT32 SymSearchSymbolForMask(const char * SearchMask);
__declspec(dllimport) BOOLEAN SymConvertFileToPdbPath(const char * LocalFilePath, char * ResultPath);
__declspec(dllimport) BOOLEAN SymConvertFileToPdbFileAndGuidAndAgeDetails(const char * LocalFilePath, char * PdbFilePath, char * GuidAndAgeDetails);

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
    ScriptEngineSearchSymbolForMask(const char * SearchMask);
__declspec(dllexport) BOOLEAN
    ScriptEngineConvertFileToPdbPath(const char * LocalFilePath, char * ResultPath);
__declspec(dllexport) BOOLEAN
    ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetails(const char * LocalFilePath, char * PdbFilePath, char * GuidAndAgeDetails);

//
// *** Exoort script engine functions ***
//
#    define SYNTAX_ERROR 0
#    define UNKOWN_TOKEN 1

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
ToSymbol(TOKEN Token);

__declspec(dllexport) PSYMBOL_BUFFER ScriptEngineParse(char * str);

char *
ScriptEngineBooleanExpresssionParse(
    UINT64         BooleanExpressionSize,
    TOKEN          FirstToken,
    TOKEN_LIST     MatchedStack,
    PSYMBOL_BUFFER CodeBuffer,
    char *         str,
    char *         c);

UINT64
BooleanExpressionExtractEnd(char * str, BOOL * WaitForWaitStatementBooleanExpression);

void
CodeGen(TOKEN_LIST MatchedStack, PSYMBOL_BUFFER CodeBuffer, TOKEN Operator);

unsigned long long int
RegisterToInt(char * str);

unsigned long long int
PseudoRegToInt(char * str);

unsigned long long int
SemanticRuleToInt(char * str);

char *
HandleError(unsigned int ErrorType, char * str);

int
GetIdentifierVal(TOKEN Token);

int
LalrGetRhsSize(int RuleId);

BOOL
LalrIsOperandType(TOKEN Token);

#endif
