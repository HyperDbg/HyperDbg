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

VOID
ShowMessages(const char * Fmt, ...);

PSYMBOL
NewSymbol(void);

PSYMBOL
NewStringSymbol(PSCRIPT_ENGINE_TOKEN Token);

PSYMBOL
NewWstringSymbol(PSCRIPT_ENGINE_TOKEN Token);

unsigned int
GetSymbolHeapSize(PSYMBOL Symbol);

void
RemoveSymbol(PSYMBOL * Symbol);

PSYMBOL_BUFFER
NewSymbolBuffer(void);

PSYMBOL_BUFFER
PushSymbol(PSYMBOL_BUFFER SymbolBuffer, const PSYMBOL Symbol);

PSYMBOL
ToSymbol(PSCRIPT_ENGINE_TOKEN PTOKEN, PSCRIPT_ENGINE_ERROR_TYPE Error);

void
ScriptEngineBooleanExpresssionParse(
    UINT64                    BooleanExpressionSize,
    PSCRIPT_ENGINE_TOKEN      FirstToken,
    PSCRIPT_ENGINE_TOKEN_LIST MatchedStack,
    PSYMBOL_BUFFER            CodeBuffer,
    char *                    str,
    char *                    c,
    PSCRIPT_ENGINE_ERROR_TYPE Error);

UINT64
BooleanExpressionExtractEnd(char * str, BOOL * WaitForWaitStatementBooleanExpression, PSCRIPT_ENGINE_TOKEN CurrentIn);

void
CodeGen(
    PSCRIPT_ENGINE_TOKEN_LIST MatchedStack,
    PSYMBOL_BUFFER            CodeBuffer,
    PSCRIPT_ENGINE_TOKEN      Operator,
    PSCRIPT_ENGINE_ERROR_TYPE Error);

unsigned long long int
RegisterToInt(char * str);

unsigned long long int
PseudoRegToInt(char * str);

unsigned long long int
SemanticRuleToInt(char * str);

char *
HandleError(PSCRIPT_ENGINE_ERROR_TYPE Error, char * str);

int
NewGlobalIdentifier(PSCRIPT_ENGINE_TOKEN PTOKEN);

int
GetGlobalIdentifierVal(PSCRIPT_ENGINE_TOKEN PTOKEN);

VOID
SetGlobalIdentifierVariableType(PSCRIPT_ENGINE_TOKEN Token, unsigned long long VariableType);

unsigned long long
GetGlobalIdentifierVariableType(PSCRIPT_ENGINE_TOKEN Token);

int
NewLocalIdentifier(PSCRIPT_ENGINE_TOKEN PTOKEN);

int
GetLocalIdentifierVal(PSCRIPT_ENGINE_TOKEN PTOKEN);

VOID
SetLocalIdentifierVariableType(PSCRIPT_ENGINE_TOKEN Token, unsigned long long VariableType);

unsigned long long
GetLocalIdentifierVariableType(PSCRIPT_ENGINE_TOKEN Token);

int
NewFunctionParameterIdentifier(PSCRIPT_ENGINE_TOKEN Token);

int
GetFunctionParameterIdentifier(PSCRIPT_ENGINE_TOKEN Token);

int
LalrGetRhsSize(int RuleId);

BOOL
LalrIsOperandType(PSCRIPT_ENGINE_TOKEN PTOKEN);

PUSER_DEFINED_FUNCTION_NODE
GetUserDefinedFunctionNode(PSCRIPT_ENGINE_TOKEN Token);

BOOLEAN
FuncGetNumberOfOperands(UINT64 FuncType, UINT32 * NumberOfGetOperands, UINT32 * NumberOfSetOperands);

#endif
