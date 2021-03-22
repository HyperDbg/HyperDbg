

#pragma once

#ifndef COMMON_H
#    define COMMON_H

#    define SYMBOL_BUFFER_INIT_SIZE 64
#    define MAX_TEMP_COUNT          32

/**
* @brief maximum length of string in the token
*/
#    define TOKEN_VALUE_MAX_LEN 8

/**
* @brief init size of token list
*/
#    define TOKEN_LIST_INIT_SIZE 1024

/**
* @brief enumerates possible types for token
*/
typedef enum TOKEN_TYPE
{
    ID,
    DECIMAL,
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
struct _TOKEN
{
    TOKEN_TYPE   Type;
    char *       Value;
    unsigned int len;
    unsigned int max_len;
};

/**
* @brief Pointer to _TOKEN structure
*/
typedef struct _TOKEN * TOKEN;

/**
* @brief this structure is a dynamic containter of TOKENS
*/
typedef struct _TOKEN_LIST
{
    TOKEN *      Head;
    unsigned int Pointer;
    unsigned int Size;
} * TOKEN_LIST;

// TODO: automate generation of KeyWordList

////////////////////////////////////////////////////
// TOKEN related functions						  //
////////////////////////////////////////////////////
TOKEN
NewToken(void);

void
RemoveToken(TOKEN Token);

void
PrintToken(TOKEN Token);

void
Append(TOKEN Token, char c);

TOKEN
NewTemp(void);

void
FreeTemp(TOKEN Temp);

////////////////////////////////////////////////////
//			TOKEN_LIST related functions		  //
////////////////////////////////////////////////////
TOKEN_LIST
NewTokenList(void);

void
RemoveTokenList(TOKEN_LIST TokenList);

void
PrintTokenList(TOKEN_LIST TokenList);

TOKEN_LIST
Push(TOKEN_LIST TokenList, TOKEN Token);

TOKEN
Pop(TOKEN_LIST TokenList);

TOKEN
Top(TOKEN_LIST TokenList);

char
IsNoneTerminal(TOKEN Token);

char
IsSemanticRule(TOKEN Token);

char
IsEqual(const TOKEN Token1, const TOKEN Token2);

int
GetNonTerminalId(TOKEN Token);

int
GetTerminalId(TOKEN Token);

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
HexToInt(char * str);

unsigned long long int
OctalToInt(char * str);

unsigned long long int
BinaryToInt(char * str);

////////////////////////////////////////////////////
//	       Semantic Rule Related Functions		  //
////////////////////////////////////////////////////
char
IsType1Func(TOKEN Operator);

char
IsType2Func(TOKEN Operator);

char
IsType3Func(TOKEN Operator);

char
IsNaiveOperator(TOKEN Operator);

#endif // !COMMON_H
