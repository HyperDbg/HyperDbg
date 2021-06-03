/**
 * @file ScriptEngine.c
 * @author M.H. Gholamrezei (gholamrezaei.mh@gmail.com)
 * @author Sina Karvandi (sina@rayanfam.com)
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
UINT64
ScriptEngineConvertNameToAddress(const char * FunctionOrVariableName, PBOOLEAN WasFound)
{
    //
    // A wrapper for pdb parser
    //
    return SymConvertNameToAddress(FunctionOrVariableName, WasFound);
}

/**
*
*
*/

UINT32
ScriptEngineLoadFileSymbol(UINT64 BaseAddress, const char * PdbFileName)
{
    //
    // A wrapper for pdb parser
    //
    return SymLoadFileSymbol(BaseAddress, PdbFileName);
}

UINT32
ScriptEngineUnloadAllSymbols()
{
    //
    // A wrapper for pdb unloader
    //
    return SymUnloadAllSymbols();
}

UINT32
ScriptEngineSearchSymbolForMask(const char * SearchMask)
{
    //
    // A wrapper for pdb mask searcher
    //
    return SymSearchSymbolForMask(SearchMask);
}

/**
*
*
*/
PSYMBOL_BUFFER
ScriptEngineParse(char * str)
{
    TOKEN_LIST Stack = NewTokenList();
    TOKEN_LIST LALRInputTokens;

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
    BOOL WaitForWaitStatementBooleanExpression = FALSE;

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
            if (TopToken->Value == "BOOLEAN_EXPRESSION")
            {
                UINT64 BooleanExpressionSize = BooleanExpressionExtractEnd(str, &WaitForWaitStatementBooleanExpression);

                char * Message = ScriptEngineBooleanExpresssionParse(BooleanExpressionSize, CurrentIn, MatchedStack, CodeBuffer, str, &c);
                if (Message != NULL)
                {
                    CodeBuffer->Message = Message;

                    RemoveToken(StartToken);
                    RemoveToken(EndToken);
                    RemoveTokenList(MatchedStack);
                    RemoveToken(CurrentIn);
                    return CodeBuffer;
                }

                CurrentIn = Scan(str, &c);
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
                TopToken = Pop(Stack);
            }
            else
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
                if (!strcmp(TopToken->Value, "@START_OF_FOR"))
                {
                    WaitForWaitStatementBooleanExpression = TRUE;
                }
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
        printf("Stack: \n");
        PrintTokenList(Stack);
        printf("\n");
#endif

    } while (TopToken->Type != END_OF_STACK);

    RemoveTokenList(Stack);
    //RemoveTokenList(LALRInputTokens);

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
    TOKEN Op2;
    TOKEN Temp;

    PSYMBOL OperatorSymbol;
    PSYMBOL Op0Symbol;
    PSYMBOL Op1Symbol;
    PSYMBOL Op2Symbol;
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
    else if (IsType6Func(Operator))
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

        Op2       = Pop(MatchedStack);
        Op2Symbol = ToSymbol(Op2);
        PushSymbol(CodeBuffer, Op2Symbol);
        RemoveSymbol(Op2Symbol);

        //
        // Free the operand if it is a temp value
        //
        FreeTemp(Op0);
        FreeTemp(Op1);
        FreeTemp(Op2);
    }
    else if (IsTwoOperandOperator(Operator))
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

        //
        // Free the operand if it is a temp value
        //
        FreeTemp(Op0);
        FreeTemp(Op1);
    }
    else if (IsOneOperandOperator(Operator))
    {
        PushSymbol(CodeBuffer, OperatorSymbol);
        Op0       = Pop(MatchedStack);
        Op0Symbol = ToSymbol(Op0);
        PushSymbol(CodeBuffer, Op0Symbol);
        RemoveSymbol(Op0Symbol);

        //
        // Free the operand if it is a temp value
        //
        FreeTemp(Op0);
    }
    else if (!strcmp(Operator->Value, "@VARGSTART"))
    {
        TOKEN OperatorCopy  = NewToken();
        OperatorCopy->Value = malloc(strlen(Operator->Value) + 1);
        strcpy(OperatorCopy->Value, Operator->Value);
        OperatorCopy->Type = Operator->Type;
        Push(MatchedStack, OperatorCopy);
    }
    else if (!strcmp(Operator->Value, "@START_OF_IF"))
    {
        Push(MatchedStack, Operator);
    }
    else if (!strcmp(Operator->Value, "@JZ"))
    {
        UINT64 CurrentPointer = CodeBuffer->Pointer;
        PushSymbol(CodeBuffer, OperatorSymbol);

        PSYMBOL JumpAddressSymbol = NewSymbol();
        JumpAddressSymbol->Type   = SYMBOL_NUM_TYPE;
        JumpAddressSymbol->Value  = 0xffffffffffffffff;
        PushSymbol(CodeBuffer, JumpAddressSymbol);
        RemoveSymbol(JumpAddressSymbol);

        Op0       = Pop(MatchedStack);
        Op0Symbol = ToSymbol(Op0);
        PushSymbol(CodeBuffer, Op0Symbol);
        RemoveSymbol(Op0Symbol);

        TOKEN CurrentAddressToken = NewToken();
        CurrentAddressToken->Type = DECIMAL;

        char * str = malloc(16);
        sprintf(str, "%llu", CurrentPointer);
        CurrentAddressToken->Value = str;
        Push(MatchedStack, CurrentAddressToken);

        FreeTemp(Op0);
    }
    else if (!strcmp(Operator->Value, "@JMP_TO_END_AND_JZCOMPLETED"))
    {
        //
        // Print Debug Info
        //
        printf("Semantic Stack:\n");
        PrintTokenList(MatchedStack);

        printf("Code Buffer:\n");
        PrintSymbolBuffer(CodeBuffer);
        printf("\n");

        //
        // Set JZ jump address
        //
        UINT64  CurrentPointer           = CodeBuffer->Pointer;
        TOKEN   JumpSemanticAddressToken = Pop(MatchedStack);
        UINT64  JumpSemanticAddress      = DecimalToInt(JumpSemanticAddressToken->Value);
        PSYMBOL JumpAddressSymbol        = (PSYMBOL)(CodeBuffer->Head + JumpSemanticAddress + 1);
        JumpAddressSymbol->Value         = CurrentPointer + 2;

        //
        // Add jmp instruction to Code Buffer
        //
        PSYMBOL JumpInstruction = NewSymbol();
        JumpInstruction->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
        JumpInstruction->Value  = FUNC_JMP;
        PushSymbol(CodeBuffer, JumpInstruction);
        RemoveSymbol(JumpInstruction);

        //
        // Add -1 decimal code to jump address
        //
        JumpAddressSymbol        = NewSymbol();
        JumpAddressSymbol->Type  = SYMBOL_NUM_TYPE;
        JumpAddressSymbol->Value = 0xffffffffffffffff;
        PushSymbol(CodeBuffer, JumpAddressSymbol);
        RemoveSymbol(JumpAddressSymbol);

        //
        // push current pointer to stack
        //
        TOKEN CurrentAddressToken = NewToken();
        CurrentAddressToken->Type = DECIMAL;

        char * str = malloc(16);
        sprintf(str, "%llu", CurrentPointer);
        CurrentAddressToken->Value = str;
        Push(MatchedStack, CurrentAddressToken);
    }
    else if (!strcmp(Operator->Value, "@END_OF_IF"))
    {
        UINT64 CurrentPointer = CodeBuffer->Pointer;

        TOKEN   JumpSemanticAddressToken = Pop(MatchedStack);
        PSYMBOL JumpAddressSymbol;
        while (strcmp(JumpSemanticAddressToken->Value, "@START_OF_IF"))
        {
            UINT64 JumpSemanticAddress = DecimalToInt(JumpSemanticAddressToken->Value);
            JumpAddressSymbol          = (PSYMBOL)(CodeBuffer->Head + JumpSemanticAddress + 1);
            JumpAddressSymbol->Value   = CurrentPointer;
            JumpSemanticAddressToken   = Pop(MatchedStack);
        }
    }
    else if (!strcmp(Operator->Value, "@START_OF_WHILE"))
    {
        //
        // Push @START_OF_WHILE token into matched stack
        //
        Push(MatchedStack, Operator);

        UINT64 CurrentPointer      = CodeBuffer->Pointer;
        TOKEN  CurrentAddressToken = NewToken();
        CurrentAddressToken->Type  = DECIMAL;

        char * str = malloc(16);
        sprintf(str, "%llu", CurrentPointer);
        CurrentAddressToken->Value = str;
        Push(MatchedStack, CurrentAddressToken);
    }
    else if (!strcmp(Operator->Value, "@START_OF_WHILE_COMMANDS"))
    {
        UINT64 CurrentPointer = CodeBuffer->Pointer;
        TOKEN  JzToken        = NewToken();
        JzToken->Type         = SEMANTIC_RULE;
        char * str            = malloc(strlen("@JZ") + 1);
        strcpy(str, "@JZ");
        JzToken->Value = str;
        OperatorSymbol = ToSymbol(JzToken);
        PushSymbol(CodeBuffer, OperatorSymbol);

        PSYMBOL JumpAddressSymbol = NewSymbol();
        JumpAddressSymbol->Type   = SYMBOL_NUM_TYPE;
        JumpAddressSymbol->Value  = 0xffffffffffffffff;
        PushSymbol(CodeBuffer, JumpAddressSymbol);
        RemoveSymbol(JumpAddressSymbol);

        Op0       = Pop(MatchedStack);
        Op0Symbol = ToSymbol(Op0);
        PushSymbol(CodeBuffer, Op0Symbol);
        RemoveSymbol(Op0Symbol);

        TOKEN StartOfWhileToken = Pop(MatchedStack);

        TOKEN CurrentAddressToken = NewToken();
        CurrentAddressToken->Type = DECIMAL;
        str                       = malloc(16);
        sprintf(str, "%llu", CurrentPointer + 1);
        CurrentAddressToken->Value = str;
        Push(MatchedStack, CurrentAddressToken);

        Push(MatchedStack, StartOfWhileToken);

        FreeTemp(Op0);

        //
        // Print Debug Info
        //
        printf("Semantic Stack:\n");
        PrintTokenList(MatchedStack);

        printf("Code Buffer:\n");
        PrintSymbolBuffer(CodeBuffer);
        printf("\n");
    }
    else if (!strcmp(Operator->Value, "@END_OF_WHILE"))
    {
        //
        // Print Debug Info
        //
        printf("Semantic Stack:\n");
        PrintTokenList(MatchedStack);

        printf("Code Buffer:\n");
        PrintSymbolBuffer(CodeBuffer);
        printf("\n");

        //
        // Add jmp instruction to Code Buffer
        //
        PSYMBOL JumpInstruction = NewSymbol();
        JumpInstruction->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
        JumpInstruction->Value  = FUNC_JMP;
        PushSymbol(CodeBuffer, JumpInstruction);
        RemoveSymbol(JumpInstruction);

        //
        // Add jmp address to Code buffer
        //
        TOKEN   JumpAddressToken  = Pop(MatchedStack);
        UINT64  JumpAddress       = DecimalToInt(JumpAddressToken->Value);
        PSYMBOL JumpAddressSymbol = ToSymbol(JumpAddressToken);
        PushSymbol(CodeBuffer, JumpAddressSymbol);
        RemoveSymbol(JumpAddressSymbol);

        //
        // Set JZ jump address
        //
        /*UINT64  CurrentPointer           = CodeBuffer->Pointer;
        TOKEN   JumpSemanticAddressToken = Pop(MatchedStack);
        UINT64  JumpSemanticAddress      = DecimalToInt(JumpSemanticAddressToken->Value);
        JumpAddressSymbol        = (PSYMBOL)(CodeBuffer->Head + JumpSemanticAddress);
        JumpAddressSymbol->Value         = CurrentPointer;*/

        //
        // Set jumps addresses
        //

        PUINT64 CurrentPointer = CodeBuffer->Pointer;

        do
        {
            JumpAddressToken = Pop(MatchedStack);
            if (!strcmp(JumpAddressToken->Value, "@START_OF_WHILE"))
            {
                break;
            }
            JumpAddress = DecimalToInt(JumpAddressToken->Value);
            printf("Jz Jump Address = %d\n", JumpAddress);
            JumpAddressSymbol        = (PSYMBOL)(CodeBuffer->Head + JumpAddress);
            JumpAddressSymbol->Value = CurrentPointer;

        } while (TRUE);

        //
        // Print Debug Info
        //
        printf("Semantic Stack:\n");
        PrintTokenList(MatchedStack);

        printf("Code Buffer:\n");
        PrintSymbolBuffer(CodeBuffer);
        printf("\n");
    }
    else if (!strcmp(Operator->Value, "@START_OF_DO_WHILE"))
    {
        //
        // Push @START_OF_DO_WHILE token into matched stack
        //
        Push(MatchedStack, Operator);

        UINT64 CurrentPointer      = CodeBuffer->Pointer;
        TOKEN  CurrentAddressToken = NewToken();
        CurrentAddressToken->Type  = DECIMAL;

        char * str = malloc(16);
        sprintf(str, "%llu", CurrentPointer);
        CurrentAddressToken->Value = str;
        Push(MatchedStack, CurrentAddressToken);
    }
    else if (!strcmp(Operator->Value, "@END_OF_DO_WHILE"))
    {
        //
        // Print Debug Info
        //
        printf("Semantic Stack:\n");
        PrintTokenList(MatchedStack);

        printf("Code Buffer:\n");
        PrintSymbolBuffer(CodeBuffer);
        printf("\n");

        //
        // Add jmp instruction to Code Buffer
        //
        PSYMBOL JumpInstruction = NewSymbol();
        JumpInstruction->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
        JumpInstruction->Value  = FUNC_JNZ;
        PushSymbol(CodeBuffer, JumpInstruction);
        RemoveSymbol(JumpInstruction);

        //
        // Add Op0 to CodeBuffer
        //
        Op0       = Pop(MatchedStack);
        Op0Symbol = ToSymbol(Op0);

        //
        // Add jmp address to Code buffer
        //
        TOKEN  JumpAddressToken = Pop(MatchedStack);
        UINT64 JumpAddress      = DecimalToInt(JumpAddressToken->Value);

        PSYMBOL JumpAddressSymbol = ToSymbol(JumpAddressToken);
        PushSymbol(CodeBuffer, JumpAddressSymbol);
        RemoveSymbol(JumpAddressSymbol);

        PushSymbol(CodeBuffer, Op0Symbol);
        RemoveSymbol(Op0Symbol);

        FreeTemp(Op0);

        //
        // Set jumps addresses
        //

        PUINT64 CurrentPointer = CodeBuffer->Pointer;

        do
        {
            JumpAddressToken = Pop(MatchedStack);
            if (!strcmp(JumpAddressToken->Value, "@START_OF_DO_WHILE"))
            {
                break;
            }
            JumpAddress = DecimalToInt(JumpAddressToken->Value);
            printf("Jz Jump Address = %d\n", JumpAddress);
            JumpAddressSymbol        = (PSYMBOL)(CodeBuffer->Head + JumpAddress);
            JumpAddressSymbol->Value = CurrentPointer;

        } while (TRUE);

        //
        // Print Debug Info
        //
        printf("Semantic Stack:\n");
        PrintTokenList(MatchedStack);

        printf("Code Buffer:\n");
        PrintSymbolBuffer(CodeBuffer);
        printf("\n");
    }
    else if (!strcmp(Operator->Value, "@START_OF_FOR"))
    {
        //
        // Push @START_OF_FOR token into matched stack
        //
        Push(MatchedStack, Operator);

        //
        // Push current pointer into matched stack
        //
        UINT64 CurrentPointer      = CodeBuffer->Pointer;
        TOKEN  CurrentAddressToken = NewToken();
        CurrentAddressToken->Type  = DECIMAL;

        char * str = malloc(16);
        sprintf(str, "%llu", CurrentPointer);
        CurrentAddressToken->Value = str;
        Push(MatchedStack, CurrentAddressToken);
    }
    else if (!strcmp(Operator->Value, "@FOR_INC_DEC"))
    {
        //
        // JZ
        //

        //
        // Add jz instruction to Code Buffer
        //
        PSYMBOL JnzInstruction = NewSymbol();
        JnzInstruction->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
        JnzInstruction->Value  = FUNC_JZ;
        PushSymbol(CodeBuffer, JnzInstruction);
        RemoveSymbol(JnzInstruction);

        //
        // Add JZ addresss to Code CodeBuffer
        //
        PSYMBOL JnzAddressSymbol = NewSymbol();
        JnzAddressSymbol->Type   = SYMBOL_NUM_TYPE;
        JnzAddressSymbol->Value  = 0xffffffffffffffff;
        PushSymbol(CodeBuffer, JnzAddressSymbol);
        RemoveSymbol(JnzAddressSymbol);

        //
        // Add Op0 to CodeBuffer
        //
        Op0       = Pop(MatchedStack);
        Op0Symbol = ToSymbol(Op0);
        PushSymbol(CodeBuffer, Op0Symbol);
        RemoveSymbol(Op0Symbol);

        //
        // JMP
        //

        //
        // Add jmp instruction to Code Buffer
        //
        PSYMBOL JumpInstruction = NewSymbol();
        JumpInstruction->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
        JumpInstruction->Value  = FUNC_JMP;
        PushSymbol(CodeBuffer, JumpInstruction);
        RemoveSymbol(JumpInstruction);

        //
        // Add jmp addresss to Code CodeBuffer
        //
        PSYMBOL JumpAddressSymbol = NewSymbol();
        JumpAddressSymbol->Type   = SYMBOL_NUM_TYPE;
        JumpAddressSymbol->Value  = 0xffffffffffffffff;
        PushSymbol(CodeBuffer, JumpAddressSymbol);
        RemoveSymbol(JumpAddressSymbol);

        //
        // Pop start_of_for address
        //
        TOKEN StartOfForAddressToken = Pop(MatchedStack);

        //
        // Push current pointer into matched stack
        //
        UINT64 CurrentPointer      = CodeBuffer->Pointer;
        TOKEN  CurrentAddressToken = NewToken();
        CurrentAddressToken->Type  = DECIMAL;

        char * str = malloc(16);
        sprintf(str, "%llu", CurrentPointer);
        CurrentAddressToken->Value = str;
        Push(MatchedStack, CurrentAddressToken);

        //
        // Push start_of_for address to matched stack
        //
        Push(MatchedStack, StartOfForAddressToken);
    }
    else if (!strcmp(Operator->Value, "@START_OF_FOR_COMMANDS"))
    {
        //
        // JMP
        //

        //
        // Add jmp instruction to Code Buffer
        //
        PSYMBOL JumpInstruction = NewSymbol();
        JumpInstruction->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
        JumpInstruction->Value  = FUNC_JMP;
        PushSymbol(CodeBuffer, JumpInstruction);
        RemoveSymbol(JumpInstruction);

        //
        // Add jmp address to Code buffer
        //
        TOKEN  JumpAddressToken = Pop(MatchedStack);
        UINT64 JumpAddress      = DecimalToInt(JumpAddressToken->Value);

        PSYMBOL JumpAddressSymbol = ToSymbol(JumpAddressToken);
        PushSymbol(CodeBuffer, JumpAddressSymbol);
        RemoveSymbol(JumpAddressSymbol);

        //
        // Set jmp address
        //
        PUINT64 CurrentPointer = CodeBuffer->Pointer;
        JumpAddressToken       = Pop(MatchedStack);
        JumpAddress            = DecimalToInt(JumpAddressToken->Value);

        JumpAddressSymbol        = (PSYMBOL)(CodeBuffer->Head + JumpAddress - 1);
        JumpAddressSymbol->Value = CurrentPointer;

        //
        // Push address of jz address to stack
        //
        TOKEN JzAddressToken = NewToken();
        JzAddressToken->Type = DECIMAL;
        char * str           = malloc(16);
        sprintf(str, "%llu", JumpAddress - 4);
        JzAddressToken->Value = str;
        Push(MatchedStack, JzAddressToken);

        //
        // Push @INC_DEC token to mathced stack
        //
        TOKEN IncDecToken = NewToken();
        IncDecToken->Type = SEMANTIC_RULE;
        str               = malloc(strlen("@INC_DEC") + 1);
        strcpy(str, "@INC_DEC");
        IncDecToken->Value = str;
        Push(MatchedStack, IncDecToken);

        //
        // Push start of inc_dec address to mathced stack
        //
        Push(MatchedStack, JumpAddressToken);
    }
    else if (!strcmp(Operator->Value, "@END_OF_FOR"))
    {
        //
        // Print Debug Info
        //
        printf("Semantic Stack:\n");
        PrintTokenList(MatchedStack);

        printf("Code Buffer:\n");
        PrintSymbolBuffer(CodeBuffer);
        printf("\n");
        //
        // Jmp
        //

        //
        // Add jmp instruction to Code Buffer
        //
        PSYMBOL JumpInstruction = NewSymbol();
        JumpInstruction->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
        JumpInstruction->Value  = FUNC_JMP;
        PushSymbol(CodeBuffer, JumpInstruction);
        RemoveSymbol(JumpInstruction);

        //
        // Add jmp address to Code buffer
        //
        TOKEN  JumpAddressToken = Pop(MatchedStack);
        UINT64 JumpAddress      = DecimalToInt(JumpAddressToken->Value);

        PSYMBOL JumpAddressSymbol = ToSymbol(JumpAddressToken);
        PushSymbol(CodeBuffer, JumpAddressSymbol);
        RemoveSymbol(JumpAddressSymbol);

        JumpAddressToken = Pop(MatchedStack);

        //
        // Set jumps addresses
        //

        PUINT64 CurrentPointer = CodeBuffer->Pointer;

        do
        {
            JumpAddressToken = Pop(MatchedStack);
            if (!strcmp(JumpAddressToken->Value, "@START_OF_FOR"))
            {
                break;
            }
            else
            {
                JumpAddress = DecimalToInt(JumpAddressToken->Value);
                printf("Jz Jump Address = %d\n", JumpAddress);
                JumpAddressSymbol        = (PSYMBOL)(CodeBuffer->Head + JumpAddress);
                JumpAddressSymbol->Value = CurrentPointer;
            }

        } while (TRUE);

        //
        // Print Debug Info
        //
        printf("Semantic Stack:\n");
        PrintTokenList(MatchedStack);

        printf("Code Buffer:\n");
        PrintSymbolBuffer(CodeBuffer);
        printf("\n");
    }
    else if (!strcmp(Operator->Value, "@BREAK"))
    {
        //
        // Print Debug Info
        //
        printf("Semantic Stack:\n");
        PrintTokenList(MatchedStack);

        printf("Code Buffer:\n");
        PrintSymbolBuffer(CodeBuffer);
        printf("\n");

        //
        // Pop Objects from stack while reaching @START_OF_*
        //
        BOOL       HasError  = FALSE;
        TOKEN_LIST TempStack = NewTokenList();
        TOKEN      TempToken;
        do
        {
            TempToken = Pop(MatchedStack);

            if ((!strcmp(TempToken->Value, "@START_OF_FOR")) ||
                (!strcmp(TempToken->Value, "@START_OF_WHILE")) ||
                (!strcmp(TempToken->Value, "@START_OF_DO_WHILE")))
            {
                //
                // Push back START_OF_*
                //
                Push(MatchedStack, TempToken);

                //
                // Push current pointer into matched stack
                //
                UINT64 CurrentPointer      = CodeBuffer->Pointer + 1;
                TOKEN  CurrentAddressToken = NewToken();
                CurrentAddressToken->Type  = DECIMAL;

                char * str = malloc(16);
                sprintf(str, "%llu", CurrentPointer);
                CurrentAddressToken->Value = str;
                Push(MatchedStack, CurrentAddressToken);

                //
                // JMP
                //

                //
                // Add jmp instruction to Code Buffer
                //
                PSYMBOL JumpInstruction = NewSymbol();
                JumpInstruction->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
                JumpInstruction->Value  = FUNC_JMP;
                PushSymbol(CodeBuffer, JumpInstruction);
                RemoveSymbol(JumpInstruction);

                //
                // Add jmp address to Code buffer
                //
                PSYMBOL JumpAddressSymbol = NewSymbol();
                JumpAddressSymbol->Type   = SYMBOL_NUM_TYPE;
                JumpAddressSymbol->Value  = 0xffffffffffffffff;
                PushSymbol(CodeBuffer, JumpAddressSymbol);
                RemoveSymbol(JumpAddressSymbol);

                //
                //
                //
                do
                {
                    TempToken = Pop(TempStack);
                    Push(MatchedStack, TempToken);

                } while (TempStack->Pointer != 0);
                break;
            }
            else if (MatchedStack->Pointer == 0)
            {
                HasError = TRUE;
                break;
            }
            else
            {
                Push(TempStack, TempToken);
            }

        } while (TRUE);

        //
        // Print Debug Info
        //
        printf("Semantic Stack:\n");
        PrintTokenList(MatchedStack);

        printf("Code Buffer:\n");
        PrintSymbolBuffer(CodeBuffer);

        printf("Break hit\n");

        RemoveTokenList(TempStack);
    }
    else if (!strcmp(Operator->Value, "@CONTINUE"))
    {
        //
        // Print Debug Info
        //
        printf("Semantic Stack:\n");
        PrintTokenList(MatchedStack);

        printf("Code Buffer:\n");
        PrintSymbolBuffer(CodeBuffer);
        printf("\n");

        //
        // Pop Objects from stack while reaching @INC_DEC
        //
        BOOL       HasError  = FALSE;
        TOKEN_LIST TempStack = NewTokenList();
        TOKEN      TempToken;
        do
        {
            TempToken = Pop(MatchedStack);

            if (!strcmp(TempToken->Value, "@INC_DEC"))
            {
                //
                // Push back INC_DEC
                //
                Push(MatchedStack, TempToken);

                //
                // Add jmp instruction to Code Buffer
                //
                PSYMBOL JumpInstruction = NewSymbol();
                JumpInstruction->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
                JumpInstruction->Value  = FUNC_JMP;
                PushSymbol(CodeBuffer, JumpInstruction);
                RemoveSymbol(JumpInstruction);

                //
                // Add jmp address to Code buffer
                //
                TempToken = Pop(TempStack);
                Push(MatchedStack, TempToken);

                PSYMBOL JumpAddressSymbol = NewSymbol();
                JumpAddressSymbol->Type   = SYMBOL_NUM_TYPE;
                JumpAddressSymbol->Value  = DecimalToInt(TempToken->Value);
                PushSymbol(CodeBuffer, JumpAddressSymbol);
                RemoveSymbol(JumpAddressSymbol);

                //
                //
                //
                do
                {
                    TempToken = Pop(TempStack);
                    Push(MatchedStack, TempToken);

                } while (TempStack->Pointer != 0);
                break;
            }
            else if (MatchedStack->Pointer == 0)
            {
                HasError = TRUE;
                break;
            }
            else
            {
                Push(TempStack, TempToken);
            }

        } while (TRUE);

        RemoveTokenList(TempStack);

        //
        // Print Debug Info
        //
        printf("Semantic Stack:\n");
        PrintTokenList(MatchedStack);

        printf("Code Buffer:\n");
        PrintSymbolBuffer(CodeBuffer);
        printf("\n");
    }
    else
    {
        printf("Internal Error: Unhandled semantic rules.\n");
    }
    RemoveSymbol(OperatorSymbol);
    return;
}

UINT64
BooleanExpressionExtractEnd(char * str, BOOL * WaitForWaitStatementBooleanExpression)
{
    UINT64 BooleanExpressionSize = 0;
    if (*WaitForWaitStatementBooleanExpression)
    {
        while (str[InputIdx + BooleanExpressionSize - 1] != ';')
        {
            BooleanExpressionSize += 1;
        }
        *WaitForWaitStatementBooleanExpression = FALSE;
        return InputIdx + BooleanExpressionSize - 1;
    }
    else
    {
        int OpenParanthesesCount = 1;
        while (str[InputIdx + BooleanExpressionSize - 1] != '\0')
        {
            if (str[InputIdx + BooleanExpressionSize - 1] == ')')
            {
                OpenParanthesesCount--;
                if (OpenParanthesesCount == 0)
                {
                    return InputIdx + BooleanExpressionSize - 1;
                }
            }
            else if (str[InputIdx + BooleanExpressionSize - 1] == '(')
            {
                OpenParanthesesCount++;
            }
            BooleanExpressionSize++;
        }
    }
    return -1;
}

/**
*
*
*/
char *
ScriptEngineBooleanExpresssionParse(
    UINT64         BooleanExpressionSize,
    TOKEN          FirstToken,
    TOKEN_LIST     MatchedStack,
    PSYMBOL_BUFFER CodeBuffer,
    char *         str,
    char *         c)
{
    TOKEN_LIST Stack = NewTokenList();

    TOKEN State  = NewToken();
    State->Type  = DECIMAL;
    State->Value = malloc(strlen("0") + 1);
    strcpy(State->Value, "0");

    Push(Stack, State);

    //
    // End of File Token
    //
    TOKEN EndToken = NewToken();
    EndToken->Type = END_OF_STACK;
    strcpy(EndToken->Value, "$");

    TOKEN CurrentIn = FirstToken;
    TOKEN TopToken;
    TOKEN Lhs;
    TOKEN Temp;
    TOKEN Operand = NewToken();
    TOKEN SemanticRule;

    int          Action       = INVALID;
    int          StateId      = 0;
    int          Goto         = 0;
    int          InputPointer = 0;
    int          RhsSize      = 0;
    unsigned int InputIdxTemp;
    char         Ctemp;

    while (1)
    {
        TopToken       = Top(Stack);
        int TerminalId = LalrGetTerminalId(CurrentIn);
        StateId        = DecimalToSignedInt(TopToken->Value);
        if (StateId == INVALID)
        {
            return;
        }
        Action = LalrActionTable[StateId][TerminalId];

#ifdef _SCRIPT_ENGINE_DBG_EN
        printf("Stack :\n");
        PrintTokenList(Stack);
        printf("Action : %d\n", Action);
#endif
        if (Action == LALR_ACCEPT)
        {
            return NULL;
        }
        if (Action == INVALID)
        {
            char * Message      = HandleError(UNKOWN_TOKEN, str);
            CodeBuffer->Message = Message;
            return Message;
        }

        if (Action > 0) // Shift
        {
            StateId = Action;
            Push(Stack, CurrentIn);

            State        = NewToken();
            State->Type  = DECIMAL;
            State->Value = malloc(4);
            sprintf(State->Value, "%d", StateId);
            Push(Stack, State);

            InputIdxTemp = InputIdx;
            Ctemp        = *c;
            CurrentIn    = Scan(str, c);
            if (InputIdx - 1 > BooleanExpressionSize)
            {
                InputIdx  = InputIdxTemp;
                *c        = Ctemp;
                CurrentIn = EndToken;
            }
        }
        else if (Action < 0) // Reduce
        {
            StateId      = -Action;
            Lhs          = &LalrLhs[StateId - 1];
            RhsSize      = LalrGetRhsSize(StateId - 1);
            SemanticRule = &LalrSemanticRules[StateId - 1];

            for (int i = 0; i < 2 * RhsSize; i++)
            {
                Temp = Pop(Stack);
                if (SemanticRule->Type == SEMANTIC_RULE && !strcmp(SemanticRule->Value, "@PUSH"))
                {
                    if (LalrIsOperandType(Temp))
                    {
                        if (Operand->Type == UNKNOWN)
                        {
                            RemoveToken(Operand);
                        }
                        Operand = Temp;
                    }
                }
            }
            if (SemanticRule->Type == SEMANTIC_RULE)
            {
                if (!strcmp(SemanticRule->Value, "@PUSH"))
                {
                    Push(MatchedStack, Operand);
                }
                else
                {
                    CodeGen(MatchedStack, CodeBuffer, SemanticRule);
                }
            }

            Temp    = Top(Stack);
            StateId = DecimalToSignedInt(Temp->Value);

            Goto = LalrGotoTable[StateId][LalrGetNonTerminalId(Lhs)];

            State        = NewToken();
            State->Type  = DECIMAL;
            State->Value = malloc(4);
            sprintf(State->Value, "%d", Goto);
            Push(Stack, Lhs);
            Push(Stack, State);
        }
    }
    return NULL;
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

        printf("%8x:", i);
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
    // if token value is not found, push the token to the token list and return corresponding id
    //
    CurrentToken       = NewToken();
    CurrentToken->Type = Token->Type;
    strcpy(CurrentToken->Value, Token->Value);
    IdTable = Push(IdTable, CurrentToken);
    return IdTable->Pointer - 1;
}

int
LalrGetRhsSize(int RuleId)
{
    int Counter = 0;
    int N       = LalrRhsSize[RuleId];
    for (int i = 0; i < N; i++)
    {
        if (LalrRhs[RuleId][i].Type != EPSILON && LalrRhs[RuleId][i].Type != SEMANTIC_RULE)
        {
            Counter++;
        }
    }
    return Counter;
}

BOOL
LalrIsOperandType(TOKEN Token)
{
    if (Token->Type == ID)
    {
        return TRUE;
    }
    else if (Token->Type == DECIMAL)
    {
        return TRUE;
    }
    else if (Token->Type == HEX)
    {
        return TRUE;
    }
    else if (Token->Type == OCTAL)
    {
        return TRUE;
    }
    else if (Token->Type == BINARY)
    {
        return TRUE;
    }
    else if (Token->Type == REGISTER)
    {
        return TRUE;
    }
    else if (Token->Type == PSEUDO_REGISTER)
    {
        return TRUE;
    }
    else if (Token->Type == TEMP)
    {
        return TRUE;
    }

    return FALSE;
}
