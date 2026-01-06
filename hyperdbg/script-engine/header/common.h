/**
 * @file common.h
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 *
 * @details Common routines header
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#ifndef COMMON_H
#    define COMMON_H

/**
*
initial size of symbol buffer
*/
#    define SYMBOL_BUFFER_INIT_SIZE 64

/**
 * @brief maximum length of string in the token
 */
#    define TOKEN_VALUE_MAX_LEN 8

/**
 * @brief init size of token list
 */
#    define TOKEN_LIST_INIT_SIZE 256

/**
 * @brief enumerates possible types for token
 */
typedef enum _SCRIPT_ENGINE_TOKEN_TYPE
{
    LOCAL_ID,
    LOCAL_UNRESOLVED_ID,
    GLOBAL_ID,
    GLOBAL_UNRESOLVED_ID,
    DECIMAL,
    STATE_ID,
    HEX,
    OCTAL,
    BINARY,
    SPECIAL_TOKEN,
    KEYWORD,
    WHITE_SPACE,
    COMMENT,
    REGISTER,
    PSEUDO_REGISTER,
    NON_TERMINAL,
    SEMANTIC_RULE,
    END_OF_STACK,
    EPSILON,
    TEMP,
    STRING,
    WSTRING,
    FUNCTION_ID,
    FUNCTION_PARAMETER_ID,
    SCRIPT_VARIABLE_TYPE,
    DEFERENCE_TEMP,
    UNKNOWN
} SCRIPT_ENGINE_TOKEN_TYPE;

/**
 * @brief read tokens from input stored in this structure
 */
typedef struct _SCRIPT_ENGINE_TOKEN
{
    SCRIPT_ENGINE_TOKEN_TYPE Type;
    char *                   Value;
    unsigned int             Len;
    unsigned int             MaxLen;
    VARIABLE_TYPE *          VariableType;
    unsigned long long       VariableMemoryIdx;
} SCRIPT_ENGINE_TOKEN, *PSCRIPT_ENGINE_TOKEN;

/**
 * @brief this structure is a dynamic container of TOKENS
 */
typedef struct _SCRIPT_ENGINE_TOKEN_LIST
{
    PSCRIPT_ENGINE_TOKEN * Head;
    unsigned int           Pointer;
    unsigned int           Size;
} SCRIPT_ENGINE_TOKEN_LIST, *PSCRIPT_ENGINE_TOKEN_LIST;

////////////////////////////////////////////////////
// PTOKEN related functions						  //
////////////////////////////////////////////////////
PSCRIPT_ENGINE_TOKEN
NewUnknownToken(void);

PSCRIPT_ENGINE_TOKEN
NewToken(SCRIPT_ENGINE_TOKEN_TYPE Type, char * Value);

void
RemoveToken(PSCRIPT_ENGINE_TOKEN * Token);

void
PrintToken(PSCRIPT_ENGINE_TOKEN Token);

void
AppendByte(PSCRIPT_ENGINE_TOKEN Token, char c);

void
AppendWchar(PSCRIPT_ENGINE_TOKEN Token, wchar_t c);

PSCRIPT_ENGINE_TOKEN
CopyToken(PSCRIPT_ENGINE_TOKEN Token);

PSCRIPT_ENGINE_TOKEN
NewTemp(PSCRIPT_ENGINE_ERROR_TYPE);

void
    FreeTemp(PSCRIPT_ENGINE_ERROR_TYPE);

VARIABLE_TYPE *
HandleType(PSCRIPT_ENGINE_TOKEN_LIST PtokenStack);

VARIABLE_TYPE *
GetCommonVariableType(VARIABLE_TYPE * Ty1, VARIABLE_TYPE * Ty2);

////////////////////////////////////////////////////
//			SCRIPT_ENGINE_TOKEN_LIST related functions		  //
////////////////////////////////////////////////////

PSCRIPT_ENGINE_TOKEN_LIST
NewTokenList(void);

void
RemoveTokenList(PSCRIPT_ENGINE_TOKEN_LIST TokenList);

void
PrintTokenList(PSCRIPT_ENGINE_TOKEN_LIST TokenList);

PSCRIPT_ENGINE_TOKEN_LIST
Push(PSCRIPT_ENGINE_TOKEN_LIST TokenList, PSCRIPT_ENGINE_TOKEN Token);

PSCRIPT_ENGINE_TOKEN
Pop(PSCRIPT_ENGINE_TOKEN_LIST TokenList);

PSCRIPT_ENGINE_TOKEN
Top(PSCRIPT_ENGINE_TOKEN_LIST TokenList);

PSCRIPT_ENGINE_TOKEN
TopIndexed(PSCRIPT_ENGINE_TOKEN_LIST TokenList, int Index);

char
IsNoneTerminal(PSCRIPT_ENGINE_TOKEN Token);

char
IsSemanticRule(PSCRIPT_ENGINE_TOKEN Token);

char
IsEqual(const PSCRIPT_ENGINE_TOKEN Token1, const PSCRIPT_ENGINE_TOKEN Token2);

int
GetNonTerminalId(PSCRIPT_ENGINE_TOKEN Token);

int
GetTerminalId(PSCRIPT_ENGINE_TOKEN Token);

int
LalrGetNonTerminalId(PSCRIPT_ENGINE_TOKEN Token);

int
LalrGetTerminalId(PSCRIPT_ENGINE_TOKEN Token);

////////////////////////////////////////////////////
//					Util Functions				  //
////////////////////////////////////////////////////

char
IsHex(char c);

char
IsDecimal(char c);

char
IsLetter(char c);

char
IsUnderscore(char c);

char
IsBinary(char c);

char
IsOctal(char c);

void
SetType(unsigned long long * Val, unsigned char Type);

unsigned long long
DecimalToInt(char * str);

unsigned long long
DecimalToSignedInt(char * str);

unsigned long long
HexToInt(char * str);

unsigned long long
OctalToInt(char * str);

unsigned long long
BinaryToInt(char * str);

void
RotateLeftStringOnce(char * str);

////////////////////////////////////////////////////
//	       Semantic Rule Related Functions		  //
////////////////////////////////////////////////////

char
IsType1Func(PSCRIPT_ENGINE_TOKEN Operator);

char
IsType2Func(PSCRIPT_ENGINE_TOKEN Operator);

char
IsType4Func(PSCRIPT_ENGINE_TOKEN Operator);

char
IsType5Func(PSCRIPT_ENGINE_TOKEN Operator);

char
IsType6Func(PSCRIPT_ENGINE_TOKEN Operator);

char
IsType7Func(PSCRIPT_ENGINE_TOKEN Operator);

char
IsType8Func(PSCRIPT_ENGINE_TOKEN Operator);

char
IsType9Func(PSCRIPT_ENGINE_TOKEN Operator);

char
IsType10Func(PSCRIPT_ENGINE_TOKEN Operator);

char
IsType11Func(PSCRIPT_ENGINE_TOKEN Operator);

char
IsType12Func(PSCRIPT_ENGINE_TOKEN Operator);

char
IsType13Func(PSCRIPT_ENGINE_TOKEN Operator);

char
IsType14Func(PSCRIPT_ENGINE_TOKEN Operator);

char
IsType15Func(PSCRIPT_ENGINE_TOKEN Operator);

char
IsType16Func(PSCRIPT_ENGINE_TOKEN Operator);

char
IsAssignmentOperator(PSCRIPT_ENGINE_TOKEN Operator);

char
IsTwoOperandOperator(PSCRIPT_ENGINE_TOKEN Operator);

char
IsOneOperandOperator(PSCRIPT_ENGINE_TOKEN Operator);

/**
 *
 */
typedef struct _USER_DEFINED_FUNCTION_NODE
{
    char *                               Name;
    long long unsigned                   Address;
    long long unsigned                   VariableType;
    long long unsigned                   ParameterNumber;
    long long unsigned                   MaxTempNumber;
    long long unsigned                   LocalVariableNumber;
    long long unsigned                   IdTable;
    long long unsigned                   FunctionParameterIdTable;
    char *                               TempMap;
    struct _USER_DEFINED_FUNCTION_NODE * NextNode;
} USER_DEFINED_FUNCTION_NODE, *PUSER_DEFINED_FUNCTION_NODE;

/**
 *
 */
typedef struct _INCLUDE_NODE
{
    char *                 FilePath;
    struct _INCLUDE_NODE * NextNode;
} INCLUDE_NODE, *PINCLUDE_NODE;

#endif // !COMMON_H
