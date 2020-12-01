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



#include <stdio.h>
#include "ScriptEngineCommon.h"
#include "scanner.h"
#include "common.h"




char TempMap[MAX_TEMP_COUNT] = { 0 };
unsigned int IdCounter = 0;


char IsNoneTerminal(TOKEN Token);
char IsSemanticRule(TOKEN Token);
char IsEqual(const TOKEN Token1, const TOKEN Token2);

int GetNonTerminalId(TOKEN Token);
int GetTerminalId(TOKEN Token);


PSYMBOL NewSymbol(void);
void RemoveSymbol(PSYMBOL Symbol);
__declspec(dllexport) void PrintSymbol(PSYMBOL Symbol);

PSYMBOL_BUFFER NewSymbolBuffer(void);
__declspec(dllexport) void RemoveSymbolBuffer(PSYMBOL_BUFFER SymbolBuffer);
PSYMBOL_BUFFER PushSymbol(PSYMBOL_BUFFER SymbolBuffer, const PSYMBOL Symbol);
PSYMBOL PopSymbol(PSYMBOL_BUFFER SymbolBuffer);

__declspec(dllexport) void PrintSymbolBuffer(const PSYMBOL_BUFFER SymbolBuffer);

PSYMBOL ToSymbol(TOKEN Token);

// Util Functions:
void SetType(unsigned long long* Val, unsigned char Type);
unsigned long long int DecimalToInt(char* str);
unsigned long long int HexToInt(char* str);
unsigned long long int OctalToInt(char* str);
unsigned long long int BinaryToInt(char* str);

unsigned long long int RegisterToInt(char* str);
unsigned long long int PseudoRegToInt(char* str);
unsigned long long int SemanticRuleToInt(char* str);

TOKEN NewTemp(void);
void FreeTemp(TOKEN Temp);




__declspec(dllexport) PSYMBOL_BUFFER ScriptEngineParse(char* str);

void CodeGen(TOKEN_LIST MatchedStack, PSYMBOL_BUFFER CodeBuffer, TOKEN Operator);
char HasTwoOperand(TOKEN Operator);
