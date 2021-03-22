/**
 * @file ScriptEngine.c
 * @author M.H. Gholamrezei (gholamrezaei.mh@gmail.com)
 * @brief Script engine parser and codegen
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "globals.h"
#include "common.h"
#include "parse_table.h"
#include "ScriptEngine.h"
#include "ScriptEngineCommonDefinitions.h"
#include "string.h"

//#define _SCRIPT_ENGINE_DBG_EN
/**
*
*
*/
PSYMBOL_BUFFER
ScriptEngineParse(char * str)
{
    TOKEN_LIST     Stack        = NewTokenList();
    TOKEN_LIST     MatchedStack = NewTokenList();
    PSYMBOL_BUFFER CodeBuffer   = NewSymbolBuffer();

    static FirstCall = 1;
    if (FirstCall)
    {
        IdTable   = NewTokenList();
        FirstCall = 0;
    }

    TOKEN CurrentIn;
    TOKEN TopToken;

    int  NonTerminalId;
    int  TerminalId;
    int  RuleId;
    char c;

    //
    // Initialize Scanner
    //
    InputIdx       = 0;
    CurrentLine    = 0;
    CurrentLineIdx = 0;

    //
    // End of File Token
    //
    TOKEN EndToken = NewToken();
    EndToken->Type = END_OF_STACK;
    strcpy(EndToken->Value, "$");

    //
    // Start Token
    //
    TOKEN StartToken = NewToken();
    strcpy(StartToken->Value, START_VARIABLE);
    StartToken->Type = NON_TERMINAL;

    Push(Stack, EndToken);
    Push(Stack, StartToken);

    c = sgetc(str);

    CurrentIn = Scan(str, &c);
    if (CurrentIn->Type == UNKNOWN)
    {
        char * Message      = HandleError(UNKOWN_TOKEN, str);
        CodeBuffer->Message = Message;

        RemoveTokenList(Stack);
        RemoveTokenList(MatchedStack);
        RemoveToken(CurrentIn);
        return CodeBuffer;
    }

    do
    {
        TopToken = Pop(Stack);

#ifdef _SCRIPT_ENGINE_DBG_EN
        printf("\nTop Token :\n");
        PrintToken(TopToken);
        printf("\nCurrent Input :\n");
        PrintToken(CurrentIn);
        printf("\n");
#endif

        if (TopToken->Type == NON_TERMINAL)
        {
            NonTerminalId = GetNonTerminalId(TopToken);
            if (NonTerminalId == INVALID)
            {
                char * Message      = HandleError(SYNTAX_ERROR, str);
                CodeBuffer->Message = Message;

                RemoveToken(StartToken);
                RemoveToken(EndToken);
                RemoveTokenList(MatchedStack);
                RemoveToken(CurrentIn);
                return CodeBuffer;
            }
            TerminalId = GetTerminalId(CurrentIn);
            if (TerminalId == INVALID)
            {
                char * Message      = HandleError(SYNTAX_ERROR, str);
                CodeBuffer->Message = Message;

                RemoveToken(StartToken);
                RemoveToken(EndToken);
                RemoveTokenList(MatchedStack);
                RemoveToken(CurrentIn);
                return CodeBuffer;
            }
            RuleId = ParseTable[NonTerminalId][TerminalId];
            if (RuleId == INVALID)
            {
                char * Message      = HandleError(SYNTAX_ERROR, str);
                CodeBuffer->Message = Message;

                RemoveToken(StartToken);
                RemoveToken(EndToken);
                RemoveTokenList(MatchedStack);
                RemoveToken(CurrentIn);
                return CodeBuffer;
            }

            //
            // Push RHS Reversely into stack
            //
            for (int i = RhsSize[RuleId] - 1; i >= 0; i--)
            {
                TOKEN Token = &Rhs[RuleId][i];

                if (Token->Type == EPSILON)
                    break;
                Push(Stack, Token);
            }
        }
        else if (TopToken->Type == SEMANTIC_RULE)
        {
            if (!strcmp(TopToken->Value, "@PUSH"))
            {
                TopToken = Pop(Stack);
                Push(MatchedStack, CurrentIn);

                CurrentIn = Scan(str, &c);

                if (CurrentIn->Type == UNKNOWN)
                {
                    char * Message      = HandleError(UNKOWN_TOKEN, str);
                    CodeBuffer->Message = Message;

                    RemoveToken(StartToken);
                    RemoveToken(EndToken);
                    RemoveTokenList(MatchedStack);
                    RemoveToken(CurrentIn);
                    return CodeBuffer;
                }

                // char t = getchar();
            }
            else
            {
                CodeGen(MatchedStack, CodeBuffer, TopToken);
            }
        }
        else
        {
            if (!IsEqual(TopToken, CurrentIn))
            {
                char * Message      = HandleError(SYNTAX_ERROR, str);
                CodeBuffer->Message = Message;

                RemoveToken(StartToken);
                RemoveToken(EndToken);
                RemoveTokenList(MatchedStack);
                RemoveToken(CurrentIn);
                return CodeBuffer;
            }
            else
            {
                RemoveToken(CurrentIn);
                CurrentIn = Scan(str, &c);

                if (CurrentIn->Type == UNKNOWN)
                {
                    char * Message      = HandleError(SYNTAX_ERROR, str);
                    CodeBuffer->Message = Message;

                    RemoveToken(StartToken);
                    RemoveToken(EndToken);
                    RemoveTokenList(MatchedStack);
                    RemoveToken(CurrentIn);
                    return CodeBuffer;
                }

                /*  printf("\nCurrent Input :\n");
                PrintToken(CurrentIn);
                printf("\n");*/

#ifdef _SCRIPT_ENGINE_DBG_EN
                printf("matched...\n");
#endif
            }
        }
#ifdef _SCRIPT_ENGINE_DBG_EN
        PrintTokenList(Stack);
        printf("\n");
#endif

    } while (TopToken->Type != END_OF_STACK);

    RemoveTokenList(Stack);
    RemoveTokenList(MatchedStack);
    RemoveToken(StartToken);
    RemoveToken(EndToken);
    RemoveToken(CurrentIn);
    return CodeBuffer;
}

void
CodeGen(TOKEN_LIST MatchedStack, PSYMBOL_BUFFER CodeBuffer, TOKEN Operator)
{
    TOKEN Op0;
    TOKEN Op1;
    TOKEN Temp;

    PSYMBOL OperatorSymbol;
    PSYMBOL Op0Symbol;
    PSYMBOL Op1Symbol;
    PSYMBOL TempSymbol;

    OperatorSymbol = ToSymbol(Operator);

    if (!strcmp(Operator->Value, "@MOV"))
    {
        PushSymbol(CodeBuffer, OperatorSymbol);
        Op0       = Pop(MatchedStack);
        Op0Symbol = ToSymbol(Op0);
        PushSymbol(CodeBuffer, Op0Symbol);
        RemoveSymbol(Op0Symbol);

        Op1       = Pop(MatchedStack);
        Op1Symbol = ToSymbol(Op1);
        PushSymbol(CodeBuffer, Op1Symbol);
        RemoveSymbol(Op1Symbol);

        /* printf("%s\t%s,\t%s\n", Operator->Value, Op1->Value, Op0->Value);
        printf("_____________\n");*/

        //
        // Free the operand if it is a temp value
        //
        FreeTemp(Op0);
        FreeTemp(Op1);
    }
    else if (IsType2Func(Operator))
    {
        PushSymbol(CodeBuffer, OperatorSymbol);
        Op0       = Pop(MatchedStack);
        Op0Symbol = ToSymbol(Op0);
        PushSymbol(CodeBuffer, Op0Symbol);
        RemoveSymbol(Op0Symbol);
        /*  printf("%s\t%s\n", Operator->Value, Op0->Value);
        printf("_____________\n");*/
    }
    else if (IsType1Func(Operator))
    {
        PushSymbol(CodeBuffer, OperatorSymbol);
        Op0       = Pop(MatchedStack);
        Op0Symbol = ToSymbol(Op0);
        PushSymbol(CodeBuffer, Op0Symbol);
        RemoveSymbol(Op0Symbol);

        Temp = NewTemp();
        Push(MatchedStack, Temp);
        TempSymbol = ToSymbol(Temp);
        PushSymbol(CodeBuffer, TempSymbol);
        RemoveSymbol(TempSymbol);
        /* printf("%s\t%s,\t%s\n", Operator->Value, Temp->Value, Op0->Value);
        printf("_____________\n");*/

        //
        // Free the operand if it is a temp value
        //
        FreeTemp(Op0);
    }
    else if (IsType4Func(Operator))
    {
        PushSymbol(CodeBuffer, OperatorSymbol);
        PSYMBOL_BUFFER TempStack    = NewSymbolBuffer();
        UINT32         OperandCount = 0;

        do
        {
            Op1 = Pop(MatchedStack);
            if (Op1->Type != SEMANTIC_RULE)
            {
                Op1Symbol = ToSymbol(Op1);
                PushSymbol(TempStack, Op1Symbol);
                RemoveSymbol(Op1Symbol);
                FreeTemp(Op1);
                OperandCount++;
            }

        } while (!(Op1->Type == SEMANTIC_RULE && !strcmp(Op1->Value, "@VARGSTART")));

        Op0       = Pop(MatchedStack);
        Op0Symbol = ToSymbol(Op0);
        PushSymbol(CodeBuffer, Op0Symbol);
        RemoveSymbol(Op0Symbol);

        // TODO: Push OperandCount
        PSYMBOL OperandCountSymbol = NewSymbol();
        OperandCountSymbol->Type   = SYMBOL_VARIABLE_COUNT_TYPE;
        OperandCountSymbol->Value  = OperandCount;
        PushSymbol(CodeBuffer, OperandCountSymbol);
        RemoveSymbol(OperandCountSymbol);

        PSYMBOL Symbol;
        for (int i = TempStack->Pointer - 1; i >= 0; i--)
        {
            Symbol = TempStack->Head + i;
            PushSymbol(CodeBuffer, Symbol);
        }
        RemoveSymbolBuffer(TempStack);

        FreeTemp(Op0);
    }
    else if (IsType5Func(Operator))
    {
        PushSymbol(CodeBuffer, OperatorSymbol);
    }
    else if (IsNaiveOperator(Operator))
    {
        PushSymbol(CodeBuffer, OperatorSymbol);
        Op0       = Pop(MatchedStack);
        Op0Symbol = ToSymbol(Op0);
        PushSymbol(CodeBuffer, Op0Symbol);
        RemoveSymbol(Op0Symbol);

        Op1       = Pop(MatchedStack);
        Op1Symbol = ToSymbol(Op1);
        PushSymbol(CodeBuffer, Op1Symbol);
        RemoveSymbol(Op1Symbol);

        Temp = NewTemp();
        Push(MatchedStack, Temp);
        TempSymbol = ToSymbol(Temp);
        PushSymbol(CodeBuffer, TempSymbol);
        RemoveSymbol(TempSymbol);

        /*  printf("%s\t%s,\t%s,\t%s\n", Operator->Value, Temp->Value, Op0->Value, Op1->Value);
        printf("_____________\n");*/

        //
        // Free the operand if it is a temp value
        //
        FreeTemp(Op0);
        FreeTemp(Op1);
    }
    else if (!strcmp(Operator->Value, "@VARGSTART"))
    {
        TOKEN OperatorCopy  = NewToken();
        OperatorCopy->Value = malloc(strlen(Operator->Value) + 1);
        strcpy(OperatorCopy->Value, Operator->Value);
        OperatorCopy->Type = Operator->Type;
        Push(MatchedStack, OperatorCopy);
    }

    else
    {
        printf("Internal Error: Unhandled semantic ruls.\n");
    }
    RemoveSymbol(OperatorSymbol);
    return;
}

/**
*
*
*
*/
PSYMBOL
NewSymbol(void)
{
    PSYMBOL Symbol;
    Symbol        = (PSYMBOL)malloc(sizeof(*Symbol));
    Symbol->Value = 0;
    Symbol->Type  = 0;
    return Symbol;
}

PSYMBOL
NewStringSymbol(char * value)
{
    PSYMBOL Symbol;
    int     BufferSize = (sizeof(unsigned long long) + (strlen(value))) / sizeof(SYMBOL) + 1;
    Symbol             = (unsigned long long)malloc(BufferSize * sizeof(SYMBOL));
    strcpy(&Symbol->Value, value);
    SetType(&Symbol->Type, SYMBOL_STRING_TYPE);
    return Symbol;
}

unsigned int
GetStringSymbolSize(PSYMBOL Symbol)
{
    int Temp = (sizeof(unsigned long long) + (strlen(&Symbol->Value))) / sizeof(SYMBOL) + 1;
    return Temp;
}

/**
*
*
*
*/
void
RemoveSymbol(PSYMBOL Symbol)
{
    free(Symbol);
    return;
}

/**
*
*
*
*/
void
PrintSymbol(PSYMBOL Symbol)
{
    if (Symbol->Type == SYMBOL_STRING_TYPE)
    {
        printf("Type:%llx, Value:%s\n", Symbol->Type, &Symbol->Value);
    }
    else
    {
        printf("Type:%llx, Value:0x%llx\n", Symbol->Type, Symbol->Value);
    }
}

PSYMBOL
ToSymbol(TOKEN Token)
{
    PSYMBOL Symbol = NewSymbol();
    switch (Token->Type)
    {
    case ID:
        Symbol->Value = GetIdentifierVal(Token);
        SetType(&Symbol->Type, SYMBOL_ID_TYPE);
        return Symbol;
    case DECIMAL:
        Symbol->Value = DecimalToInt(Token->Value);
        SetType(&Symbol->Type, SYMBOL_NUM_TYPE);
        return Symbol;
    case HEX:
        Symbol->Value = HexToInt(Token->Value);
        SetType(&Symbol->Type, SYMBOL_NUM_TYPE);
        return Symbol;
    case OCTAL:
        Symbol->Value = OctalToInt(Token->Value);
        SetType(&Symbol->Type, SYMBOL_NUM_TYPE);
        return Symbol;
    case BINARY:
        Symbol->Value = BinaryToInt(Token->Value);
        SetType(&Symbol->Type, SYMBOL_NUM_TYPE);
        return Symbol;

    case REGISTER:
        Symbol->Value = RegisterToInt(Token->Value);
        SetType(&Symbol->Type, SYMBOL_REGISTER_TYPE);
        return Symbol;

    case PSEUDO_REGISTER:
        Symbol->Value = PseudoRegToInt(Token->Value);
        SetType(&Symbol->Type, SYMBOL_PSEUDO_REG_TYPE);
        return Symbol;

    case SEMANTIC_RULE:
        Symbol->Value = SemanticRuleToInt(Token->Value);
        SetType(&Symbol->Type, SYMBOL_SEMANTIC_RULE_TYPE);
        return Symbol;
    case TEMP:
        Symbol->Value = DecimalToInt(Token->Value);
        SetType(&Symbol->Type, SYMBOL_TEMP_TYPE);
        return Symbol;

    case STRING:
        RemoveSymbol(Symbol);
        return NewStringSymbol(Token->Value);

    default:
        // Raise Error
        printf("Error in Converting Token with type %d to Symbol!\n", Token->Type);
    }
}

/**
*
*
*
*/
PSYMBOL_BUFFER
NewSymbolBuffer(void)
{
    PSYMBOL_BUFFER SymbolBuffer;
    SymbolBuffer          = (PSYMBOL_BUFFER)malloc(sizeof(*SymbolBuffer));
    SymbolBuffer->Pointer = 0;
    SymbolBuffer->Size    = SYMBOL_BUFFER_INIT_SIZE;
    SymbolBuffer->Head    = (PSYMBOL)malloc(SymbolBuffer->Size * sizeof(SYMBOL));
    SymbolBuffer->Message = NULL;
    return SymbolBuffer;
}

/**
*
*
*
*/
void
RemoveSymbolBuffer(PSYMBOL_BUFFER SymbolBuffer)
{
    // PrintSymbolBuffer(SymbolBuffer);
    free(SymbolBuffer->Message);
    free(SymbolBuffer->Head);
    free(SymbolBuffer);
    return;
}

/**
*
*
*
*/
PSYMBOL_BUFFER
PushSymbol(PSYMBOL_BUFFER SymbolBuffer, const PSYMBOL Symbol)
{
    //
    // Calculate address to write new token
    //
    uintptr_t Head      = (uintptr_t)SymbolBuffer->Head;
    uintptr_t Pointer   = (uintptr_t)SymbolBuffer->Pointer;
    PSYMBOL   WriteAddr = (PSYMBOL)(Head + Pointer * sizeof(SYMBOL));

    if (Symbol->Type == SYMBOL_STRING_TYPE)
    {
        //
        // Update Pointer
        //
        SymbolBuffer->Pointer += GetStringSymbolSize(Symbol);

        //
        // Handle Overflow
        //
        if (SymbolBuffer->Pointer >= SymbolBuffer->Size - 1)
        {
            //
            // Calculate new size for the symbol B
            //
            int NewSize = SymbolBuffer->Size;
            do
            {
                NewSize *= 2;
            } while (NewSize <= SymbolBuffer->Pointer);

            //
            // Allocate a new buffer for string list with doubled length
            //
            PSYMBOL NewHead = (PSYMBOL)malloc(NewSize * sizeof(SYMBOL));

            //
            // Copy old buffer to new buffer
            //
            memcpy(NewHead, SymbolBuffer->Head, SymbolBuffer->Size * sizeof(SYMBOL));

            //
            // Free old buffer
            //
            free(SymbolBuffer->Head);

            //
            // Upadate Head and size of SymbolBuffer
            //
            SymbolBuffer->Size = NewSize;
            SymbolBuffer->Head = NewHead;
        }
        WriteAddr       = (PSYMBOL)((uintptr_t)SymbolBuffer->Head + (uintptr_t)Pointer * (uintptr_t)sizeof(SYMBOL));
        WriteAddr->Type = Symbol->Type;
        strcpy((char *)&WriteAddr->Value, (char *)&Symbol->Value);
    }
    else
    {
        //
        // Write input to the appropriate address in SymbolBuffer
        //
        *WriteAddr = *Symbol;

        //
        // Update Pointer
        //
        SymbolBuffer->Pointer++;

        //
        // Handle Overflow
        //
        if (Pointer == SymbolBuffer->Size - 1)
        {
            //
            // Allocate a new buffer for string list with doubled length
            //
            PSYMBOL NewHead = (PSYMBOL)malloc(2 * SymbolBuffer->Size * sizeof(SYMBOL));

            //
            // Copy old Buffer to new buffer
            //
            memcpy(NewHead, SymbolBuffer->Head, SymbolBuffer->Size * sizeof(SYMBOL));

            //
            // Free Old buffer
            //
            free(SymbolBuffer->Head);

            //
            // Upadate Head and size of SymbolBuffer
            //
            SymbolBuffer->Size *= 2;
            SymbolBuffer->Head = NewHead;
        }
    }

    return SymbolBuffer;
}

/**
*
*
*
*/
void
PrintSymbolBuffer(const PSYMBOL_BUFFER SymbolBuffer)
{
    PSYMBOL Symbol;
    for (int i = 0; i < SymbolBuffer->Pointer;)
    {
        Symbol = SymbolBuffer->Head + i;

        PrintSymbol(Symbol);
        if (Symbol->Type == SYMBOL_STRING_TYPE)
        {
            int temp = GetStringSymbolSize(Symbol);
            i += temp;
        }
        else
        {
            i++;
        }
    }
}

unsigned long long int
RegisterToInt(char * str)
{
    for (int i = 0; i < REGISTER_MAP_LIST_LENGTH; i++)
    {
        if (!strcmp(str, RegisterMapList[i].Name))
        {
            return RegisterMapList[i].Type;
        }
    }
    return INVALID;
}
unsigned long long int
PseudoRegToInt(char * str)
{
    for (int i = 0; i < PSEUDO_REGISTER_MAP_LIST_LENGTH; i++)
    {
        if (!strcmp(str, PseudoRegisterMapList[i].Name))
        {
            return PseudoRegisterMapList[i].Type;
        }
    }
    return INVALID;
}
unsigned long long int
SemanticRuleToInt(char * str)
{
    for (int i = 0; i < SEMANTIC_RULES_MAP_LIST_LENGTH; i++)
    {
        if (!strcmp(str, SemanticRulesMapList[i].Name))
        {
            return SemanticRulesMapList[i].Type;
        }
    }
    return INVALID;
}
char *
HandleError(unsigned int ErrorType, char * str)
{
    //
    // allocate rquired memory for message
    //
    int    MessageSize = (InputIdx - CurrentLineIdx) * 2 + 30 + 100;
    char * Message     = (char *)malloc(MessageSize);

    //
    // add line number
    //
    strcpy(Message, "Line ");
    char * Line = (char *)malloc(16);
    sprintf(Line, "%d:\n", CurrentLine);
    strcat(Message, Line);
    free(Line);

    //
    // add the line which error happened at
    //
    unsigned int LineEnd;
    for (int i = InputIdx;; i++)
    {
        if (str[i] == '\n' || str[i] == '\0')
        {
            LineEnd = i;
            break;
        }
    }

    strncat(Message, (str + CurrentLineIdx), LineEnd - CurrentLineIdx);
    strcat(Message, "\n");

    //
    // add pointer
    //
    char Space = ' ';
    for (int i = 0; i < (CurrentTokenIdx - CurrentLineIdx); i++)
    {
        strncat(Message, &Space, 1);
    }
    strcat(Message, "^\n");

    //
    // add error cause and details
    //
    switch (ErrorType)
    {
    case SYNTAX_ERROR:
        strcat(Message, "SyntaxError: ");
        strcat(Message, "Invalid Syntax");
        return Message;

    case UNKOWN_TOKEN:
        strcat(Message, "SyntaxError: ");
        strcat(Message, "Unkown Token");
        return Message;

    default:
        strcat(Message, "Unkown Error: ");
        return Message;
    }
}

int
GetIdentifierVal(TOKEN Token)
{
    TOKEN CurrentToken;
    for (uintptr_t i = 0; i < IdTable->Pointer; i++)
    {
        CurrentToken = *(IdTable->Head + i);
        if (!strcmp(Token->Value, CurrentToken->Value))
        {
            return (int)i;
        }
    }
    //
    // if token value is not found push the token to the token list and return corrsponding id
    //
    CurrentToken       = NewToken();
    CurrentToken->Type = Token->Type;
    strcpy(CurrentToken->Value, Token->Value);
    IdTable = Push(IdTable, CurrentToken);
    return IdTable->Pointer - 1;
}