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
NewStringSymbol(PTOKEN Token);

PSYMBOL
NewWstringSymbol(PTOKEN Token);

unsigned int
GetSymbolHeapSize(PSYMBOL Symbol);

void
RemoveSymbol(PSYMBOL * Symbol);

PSYMBOL_BUFFER
NewSymbolBuffer(void);

PSYMBOL_BUFFER
PushSymbol(PSYMBOL_BUFFER SymbolBuffer, const PSYMBOL Symbol);

PSYMBOL
ToSymbol(PTOKEN PTOKEN, PSCRIPT_ENGINE_ERROR_TYPE Error);

void
ScriptEngineBooleanExpresssionParse(
    UINT64                    BooleanExpressionSize,
    PTOKEN                    FirstToken,
    PTOKEN_LIST               MatchedStack,
    PSYMBOL_BUFFER            CodeBuffer,
    char *                    str,
    char *                    c,
    PSCRIPT_ENGINE_ERROR_TYPE Error);

UINT64
BooleanExpressionExtractEnd(char * str, BOOL * WaitForWaitStatementBooleanExpression, PTOKEN CurrentIn);

void
CodeGen(
    PTOKEN_LIST               MatchedStack,
    PSYMBOL_BUFFER            CodeBuffer,
    PTOKEN                    Operator,
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

PUSER_DEFINED_FUNCTION_NODE
GetUserDefinedFunctionNode(PTOKEN Token);

BOOLEAN
FuncGetNumberOfOperands(UINT64 FuncType, UINT32 * NumberOfGetOperands, UINT32 * NumberOfSetOperands);

#endif
