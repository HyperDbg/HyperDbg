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
typedef enum TOKEN_TYPE
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
    UNKNOWN
} TOKEN_TYPE;

/**
* @brief read tokens from input stored in this structure
*/
typedef struct _TOKEN
{
    TOKEN_TYPE   Type;
    char *       Value;
    unsigned int Len;
    unsigned int MaxLen;
} TOKEN, *PTOKEN;

/**
* @brief this structure is a dynamic containter of TOKENS
*/
typedef struct _TOKEN_LIST
{
    PTOKEN *     Head;
    unsigned int Pointer;
    unsigned int Size;
} TOKEN_LIST, *PTOKEN_LIST;

////////////////////////////////////////////////////
// PTOKEN related functions						  //
////////////////////////////////////////////////////
PTOKEN
NewUnknownToken(void);

PTOKEN
NewToken(TOKEN_TYPE Type, char * Value);

void
RemoveToken(PTOKEN * Token);

void
PrintToken(PTOKEN Token);

void
Append(PTOKEN Token, char c);

PTOKEN
CopyToken(PTOKEN Token);

PTOKEN
NewTemp(PSCRIPT_ENGINE_ERROR_TYPE);

void
FreeTemp(PTOKEN Temp);

void
CleanTempList(void);

////////////////////////////////////////////////////
//			TOKEN_LIST related functions		  //
////////////////////////////////////////////////////

PTOKEN_LIST
NewTokenList(void);

void
RemoveTokenList(PTOKEN_LIST TokenList);

void
PrintTokenList(PTOKEN_LIST TokenList);

PTOKEN_LIST
Push(PTOKEN_LIST TokenList, PTOKEN Token);

PTOKEN
Pop(PTOKEN_LIST TokenList);

PTOKEN
Top(PTOKEN_LIST TokenList);

char
IsNoneTerminal(PTOKEN Token);

char
IsSemanticRule(PTOKEN Token);

char
IsEqual(const PTOKEN Token1, const PTOKEN Token2);

int
GetNonTerminalId(PTOKEN Token);

int
LalrGetNonTerminalId(PTOKEN Token);

int
LalrGetTerminalId(PTOKEN Token);

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
IsBinary(char c);

char
IsOctal(char c);

void
SetType(unsigned long long * Val, unsigned char Type);

unsigned long long int
DecimalToInt(char * str);

unsigned long long int
DecimalToSignedInt(char * str);

unsigned long long int
HexToInt(char * str);

unsigned long long int
OctalToInt(char * str);

unsigned long long int
BinaryToInt(char * str);

////////////////////////////////////////////////////
//	       Semantic Rule Related Functions		  //
////////////////////////////////////////////////////

char
IsType1Func(PTOKEN Operator);

char
IsType2Func(PTOKEN Operator);

char
IsType4Func(PTOKEN Operator);

char
IsType5Func(PTOKEN Operator);

char
IsType6Func(PTOKEN Operator);

char
IsType7Func(PTOKEN Operator);

char
IsType8Func(PTOKEN Operator);

char
IsTwoOperandOperator(PTOKEN Operator);

char
IsOneOperandOperator(PTOKEN Operator);

#endif // !COMMON_H
