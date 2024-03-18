/**
 * @file script-engine.c
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Script engine parser and codegen
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

// #define _SCRIPT_ENGINE_LALR_DBG_EN
// #define _SCRIPT_ENGINE_LL1_DBG_EN
// #define _SCRIPT_ENGINE_CODEGEN_DBG_EN

/**
 * @brief Converts name to address
 *
 * @param FunctionOrVariableName
 * @param WasFound
 * @return UINT64
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

 Load symbol files
 *
 * @param BaseAddress
 * @param PdbFileName
 * @param CustomModuleName
 *
 * @return UINT32
 */
UINT32
ScriptEngineLoadFileSymbol(UINT64 BaseAddress, const char * PdbFileName, const char * CustomModuleName)
{
    //
    // A wrapper for pdb parser
    //
    return SymLoadFileSymbol(BaseAddress, PdbFileName, CustomModuleName);
}

/**
 * @brief Set the message handler as an alternative to printf
 *
 * @param Handler
 * @return VOID
 */
VOID
ScriptEngineSetTextMessageCallback(PVOID Handler)
{
    SymSetTextMessageCallback(Handler);
}

/**
 * @brief Unload all the previously loaded symbols
 *
 * @return UINT32
 */
UINT32
ScriptEngineUnloadAllSymbols()
{
    //
    // A wrapper for pdb unloader
    //
    return SymUnloadAllSymbols();
}

/**
 * @brief Unload a special pdb
 *
 * @param ModuleName
 * @return UINT32
 */
UINT32
ScriptEngineUnloadModuleSymbol(char * ModuleName)
{
    //
    // A wrapper for pdb unloader
    //
    return SymUnloadModuleSymbol(ModuleName);
}

/**
 * @brief Search for a special mask
 *
 * @param SearchMask
 * @return UINT32
 */
UINT32
ScriptEngineSearchSymbolForMask(const char * SearchMask)
{
    //
    // A wrapper for pdb mask searcher
    //
    return SymSearchSymbolForMask(SearchMask);
}

/**
 * @brief Get offset of a field from the structure
 *
 * @param TypeName
 * @param FieldName
 * @param FieldOffset
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineGetFieldOffset(CHAR * TypeName, CHAR * FieldName, UINT32 * FieldOffset)
{
    //
    // A wrapper for search for fields in the structure
    //
    return SymGetFieldOffset(TypeName, FieldName, FieldOffset);
}

/**
 * @brief Get size of a data type (structure)
 *
 * @param TypeName
 * @param TypeSize
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineGetDataTypeSize(CHAR * TypeName, UINT64 * TypeSize)
{
    //
    // A wrapper for getting size of the structure
    //
    return SymGetDataTypeSize(TypeName, TypeSize);
}

/**
 * @brief Create symbol table for disassembler
 *
 * @param CallbackFunction
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineCreateSymbolTableForDisassembler(void * CallbackFunction)
{
    //
    // A wrapper for pdb symbol table callback creator
    //
    return SymCreateSymbolTableForDisassembler(CallbackFunction);
}

/**
 * @brief Convert local file to pdb path
 *
 * @param LocalFilePath
 * @param ResultPath
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineConvertFileToPdbPath(const char * LocalFilePath, char * ResultPath)
{
    //
    // A wrapper for pdb to path converter
    //
    return SymConvertFileToPdbPath(LocalFilePath, ResultPath);
}

/**
 * @brief Initial load of the symbols
 *
 * @param BufferToStoreDetails
 * @param StoredLength
 * @param DownloadIfAvailable
 * @param SymbolPath
 * @param IsSilentLoad
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineSymbolInitLoad(PVOID        BufferToStoreDetails,
                           UINT32       StoredLength,
                           BOOLEAN      DownloadIfAvailable,
                           const char * SymbolPath,
                           BOOLEAN      IsSilentLoad)
{
    //
    // A wrapper for pdb and modules parser
    //
    return SymbolInitLoad(BufferToStoreDetails, StoredLength, DownloadIfAvailable, SymbolPath, IsSilentLoad);
}

/**
 * @brief Show data based on symbol types
 *
 * @param TypeName
 * @param Address
 * @param IsStruct
 * @param BufferAddress
 * @param AdditionalParameters
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineShowDataBasedOnSymbolTypes(const char * TypeName,
                                       UINT64       Address,
                                       BOOLEAN      IsStruct,
                                       PVOID        BufferAddress,
                                       const char * AdditionalParameters)
{
    //
    // A wrapper for showing types and data within structures
    //
    return SymShowDataBasedOnSymbolTypes(TypeName, Address, IsStruct, BufferAddress, AdditionalParameters);
}

/**
 * @brief Cancel loading
 *
 * @return VOID
 */
VOID
ScriptEngineSymbolAbortLoading()
{
    //
    // A wrapper for aborting download and reload
    //
    SymbolAbortLoading();
}

/**
 * @brief Convert file to pdb attributes for symbols
 *
 * @param LocalFilePath
 * @param PdbFilePath
 * @param GuidAndAgeDetails
 * @param Is32BitModule
 *
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetails(const char * LocalFilePath, char * PdbFilePath, char * GuidAndAgeDetails, BOOLEAN Is32BitModule)
{
    //
    // A wrapper for pdb to path file and guid and age detail converter
    //
    return SymConvertFileToPdbFileAndGuidAndAgeDetails(LocalFilePath, PdbFilePath, GuidAndAgeDetails, Is32BitModule);
}

/**
 * @brief The entry point of script engine
 *
 * @param str
 * @return PSYMBOL_BUFFER
 */
PSYMBOL_BUFFER
ScriptEngineParse(char * str)
{
    PTOKEN_LIST Stack = NewTokenList();

    PTOKEN_LIST    MatchedStack = NewTokenList();
    PSYMBOL_BUFFER CodeBuffer   = NewSymbolBuffer();

    // will modify here later
    PSYMBOL_BUFFER UserDefinedFunctions = NewSymbolBuffer();

    SCRIPT_ENGINE_ERROR_TYPE Error        = SCRIPT_ENGINE_ERROR_FREE;
    char *                   ErrorMessage = NULL;

    static FirstCall = 1;
    if (FirstCall)
    {
        IdTable                  = NewTokenList();
        FunctionParameterIdTable = NewTokenList();
        FirstCall                = 0;
    }

    PTOKEN TopToken = NewUnknownToken();

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
    PTOKEN EndToken = NewToken(END_OF_STACK, "$");

    //
    // Start Token
    //
    PTOKEN StartToken = NewToken(NON_TERMINAL, START_VARIABLE);

    Push(Stack, EndToken);
    Push(Stack, StartToken);

    c = sgetc(str);

    PTOKEN CurrentIn = Scan(str, &c);
    if (CurrentIn->Type == UNKNOWN)
    {
        Error               = SCRIPT_ENGINE_ERROR_SYNTAX;
        ErrorMessage        = HandleError(&Error, str);
        CodeBuffer->Message = ErrorMessage;

        RemoveTokenList(Stack);
        RemoveTokenList(MatchedStack);
        RemoveToken(&CurrentIn);
        return CodeBuffer;
    }

    do
    {
        RemoveToken(&TopToken);
        TopToken = Pop(Stack);

#ifdef _SCRIPT_ENGINE_LL1_DBG_EN
        printf("\nTop Token :\n");
        PrintToken(TopToken);
        printf("\nCurrent Input :\n");
        PrintToken(CurrentIn);
        printf("\n");
#endif

        if (TopToken->Type == NON_TERMINAL)
        {
            if (!strcmp(TopToken->Value, "BOOLEAN_EXPRESSION"))
            {
                UINT64 BooleanExpressionSize = BooleanExpressionExtractEnd(str, &WaitForWaitStatementBooleanExpression, CurrentIn);

                ScriptEngineBooleanExpresssionParse(BooleanExpressionSize, CurrentIn, MatchedStack, UserDefinedFunctions, CodeBuffer, str, &c, &Error);
                if (Error != SCRIPT_ENGINE_ERROR_FREE)
                {
                    break;
                }

                RemoveToken(&CurrentIn);
                CurrentIn = Scan(str, &c);
                if (CurrentIn->Type == UNKNOWN)
                {
                    Error = SCRIPT_ENGINE_ERROR_UNKNOWN_TOKEN;
                    break;
                }

                RemoveToken(&CurrentIn);
                CurrentIn = Scan(str, &c);
                if (CurrentIn->Type == UNKNOWN)
                {
                    Error = SCRIPT_ENGINE_ERROR_UNKNOWN_TOKEN;
                    break;
                }
                RemoveToken(&TopToken);
                TopToken = Pop(Stack);
            }
            else
            {
                NonTerminalId = GetNonTerminalId(TopToken);
                if (NonTerminalId == INVALID)
                {
                    Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                    break;
                }

                TerminalId = GetTerminalId(CurrentIn);
                if (TerminalId == INVALID)
                {
                    Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                    break;
                }

                RuleId = ParseTable[NonTerminalId][TerminalId];
                if (RuleId == INVALID)
                {
                    Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                    break;
                }

                //
                // Push RHS Reversely into stack
                //
                for (int i = RhsSize[RuleId] - 1; i >= 0; i--)
                {
                    PTOKEN Token = (PTOKEN)&Rhs[RuleId][i];

                    if (Token->Type == EPSILON)
                        break;

                    PTOKEN DuplicatedToken = CopyToken(Token);
                    Push(Stack, DuplicatedToken);
                }
            }
        }
        else if (TopToken->Type == SEMANTIC_RULE)
        {
            if (!strcmp(TopToken->Value, "@PUSH"))
            {
                RemoveToken(&TopToken);
                TopToken = Pop(Stack);

                Push(MatchedStack, CurrentIn);

                CurrentIn = Scan(str, &c);
                if (CurrentIn->Type == UNKNOWN)
                {
                    Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                    break;
                }
            }

            else
            {
                if (!strcmp(TopToken->Value, "@START_OF_FOR"))
                {
                    WaitForWaitStatementBooleanExpression = TRUE;
                }
                CodeGen(MatchedStack, UserDefinedFunctions, CodeBuffer, TopToken, &Error);
                if (Error != SCRIPT_ENGINE_ERROR_FREE)
                {
                    break;
                }
            }
        }
        else
        {
            if (!IsEqual(TopToken, CurrentIn))
            {
                Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }
            else
            {
                RemoveToken(&CurrentIn);
                CurrentIn = Scan(str, &c);

                if (CurrentIn->Type == UNKNOWN)
                {
                    Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                    break;
                }
            }
        }
#ifdef _SCRIPT_ENGINE_LL1_DBG_EN
        printf("Stack: \n");
        PrintTokenList(Stack);
        printf("\n");
#endif
    } while (TopToken->Type != END_OF_STACK);

    if (Error != SCRIPT_ENGINE_ERROR_FREE)
    {
        ErrorMessage = HandleError(&Error, str);
        CleanTempList();
    }
    else
    {
        ErrorMessage = NULL;
    }
    CodeBuffer->Message = ErrorMessage;

    if (Stack)
        RemoveTokenList(Stack);

    if (MatchedStack)
        RemoveTokenList(MatchedStack);

    if (UserDefinedFunctions)
        RemoveSymbolBuffer(UserDefinedFunctions);

    if (CurrentIn)
        RemoveToken(&CurrentIn);

    if (TopToken)
        RemoveToken(&TopToken);

    if (FunctionParameterIdTable)
    {
        RemoveTokenList(FunctionParameterIdTable);
        FunctionParameterIdTable = NewTokenList();
    }

    return CodeBuffer;
}

/**
 * @brief Script Engine code generator
 *
 * @param MatchedStack
 * @param CodeBuffer
 * @param Operator
 * @param Error
 */
void
CodeGen(PTOKEN_LIST MatchedStack, PSYMBOL_BUFFER UserDefinedFunctions, PSYMBOL_BUFFER CodeBuffer, PTOKEN Operator, PSCRIPT_ENGINE_ERROR_TYPE Error)
{
    PTOKEN Op0  = NULL;
    PTOKEN Op1  = NULL;
    PTOKEN Op2  = NULL;
    PTOKEN Temp = NULL;

    PSYMBOL         OperatorSymbol = NULL;
    PSYMBOL         Op0Symbol      = NULL;
    PSYMBOL         Op1Symbol      = NULL;
    PSYMBOL         Op2Symbol      = NULL;
    PSYMBOL         TempSymbol     = NULL;
    VARIABLE_TYPE * VariableType   = NULL;

    //
    // It is in user-defined function if CurrentFunctionSymbol is not null
    //
    static PSYMBOL CurrentFunctionSymbol = NULL;
    OperatorSymbol                       = ToSymbol(Operator, Error);

#ifdef _SCRIPT_ENGINE_CODEGEN_DBG_EN
    //
    // Print Debug Info
    //
    printf("Operator :\n");
    PrintToken(Operator);
    printf("\n");

    printf("Semantic Stack:\n");
    PrintTokenList(MatchedStack);
    printf("\n");

    printf("Code Buffer:\n");
    PrintSymbolBuffer(CodeBuffer);
    printf(".\n.\n.\n\n");
#endif

    while (TRUE)
    {
        if (IsVariableType(Operator))
        {
            PTOKEN PToken = CopyToken(Operator);
            PToken->Type  = INPUT_VARIABLE_TYPE;
            Push(MatchedStack, PToken);
        }
        else if (!strcmp(Operator->Value, "@START_OF_USER_DEFINED_FUNCTION"))
        {
            Op0          = Pop(MatchedStack);
            VariableType = HandleType(MatchedStack);

            if (VariableType == VARIABLE_TYPE_UNKNOWN)
            {
                *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                break;
            }

            UINT64 CurrentPointer = CodeBuffer->Pointer;
            //
            // Add jmp instruction to Code Buffer
            //
            PSYMBOL JumpInstruction = NewSymbol();
            JumpInstruction->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
            JumpInstruction->Value  = FUNC_JMP;
            PushSymbol(CodeBuffer, JumpInstruction);
            RemoveSymbol(&JumpInstruction);

            //
            // Push jump address
            //

            PSYMBOL JumpAddressSymbol = NewSymbol();
            JumpAddressSymbol->Type   = SYMBOL_NUM_TYPE;
            JumpAddressSymbol->Value  = 0xffffffffffffffff;
            PushSymbol(CodeBuffer, JumpAddressSymbol);
            RemoveSymbol(&JumpAddressSymbol);

            PushSymbol(UserDefinedFunctions, NewFunctionSymbol(Op0->Value, VariableType));
            CurrentFunctionSymbol = NewFunctionSymbol(Op0->Value, VariableType);
            PushSymbol(CodeBuffer, CurrentFunctionSymbol);

            PushSymbol(CodeBuffer, OperatorSymbol);

            PSYMBOL StackTempNumberSymbol = NewSymbol();
            StackTempNumberSymbol->Type   = SYMBOL_NUM_TYPE;
            StackTempNumberSymbol->Value  = 0xffffffffffffffff;
            PushSymbol(CodeBuffer, StackTempNumberSymbol);
            RemoveSymbol(&StackTempNumberSymbol);
        }
        else if (!strcmp(Operator->Value, "@FUNCTION_PARAMETER"))
        {
            Op0          = Pop(MatchedStack);
            VariableType = HandleType(MatchedStack);

            if (VariableType == VARIABLE_TYPE_UNKNOWN)
            {
                *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                break;
            }

            Op0Symbol = NewSymbol();
            free((void *)Op0Symbol->Value);
            Op0Symbol->Value = NewFunctionParameterIdentifier(Op0);
            SetType(&Op0Symbol->Type, SYMBOL_FUNCTION_PARAMETER_ID_TYPE);

            PushSymbol(UserDefinedFunctions, Op0Symbol);
        }
        else if (!strcmp(Operator->Value, "@END_OF_USER_DEFINED_FUNCTION"))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);
            UINT64  CurrentPointer = CodeBuffer->Pointer;
            PSYMBOL Symbol         = NULL;

            BOOL         Found = FALSE;
            unsigned int i     = 0;
            // for loop not able to begin from CodeBuffer->Pointer to 0 because of string and wstring's size
            for (; i < CodeBuffer->Pointer;)
            {
                Symbol = CodeBuffer->Head + i;

                if (Symbol->Type == SYMBOL_USER_DEFINED_FUNCTION_TYPE && !strcmp((const char *)CurrentFunctionSymbol->Value, (const char *)Symbol->Value))
                {
                    Found = TRUE;
                    break;
                }
                else if (Symbol->Type == SYMBOL_STRING_TYPE || Symbol->Type == SYMBOL_WSTRING_TYPE)
                {
                    int temp = GetSymbolHeapSize(Symbol);
                    i += temp;
                }
                else
                {
                    i++;
                }
            }

            if (!Found)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }

            //
            // skip executing user-defined function first time
            //
            Symbol        = CodeBuffer->Head + i - 1;
            Symbol->Value = CurrentPointer;

            long long unsigned StackTempNumber = 0;
            for (int j = i; j < CurrentPointer; j++)
            {
                Symbol = CodeBuffer->Head + j;
                if (Symbol->Type == SYMBOL_STACK_TEMP_TYPE && (Symbol->Value + 1) > StackTempNumber)
                {
                    StackTempNumber = Symbol->Value + 1;
                }
            }

            Symbol        = CodeBuffer->Head + i + 2;
            Symbol->Value = StackTempNumber;

            CurrentFunctionSymbol = NULL;
        }
        else if (!strcmp(Operator->Value, "@RETURN_OF_USER_DEFINED_FUNCTION_WITHOUT_VALUE"))
        {
            if (!CurrentFunctionSymbol)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }
            if (CurrentFunctionSymbol->VariableType != (unsigned long long)VARIABLE_TYPE_VOID)
            {
                *Error = SCRIPT_ENGINE_ERROR_NON_VOID_FUNCTION_NOT_RETURNING_VALUE;
                break;
            }
            PushSymbol(CodeBuffer, OperatorSymbol);
        }
        else if (!strcmp(Operator->Value, "@RETURN_OF_USER_DEFINED_FUNCTION_WITH_VALUE"))
        {
            if (!CurrentFunctionSymbol)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }
            if (CurrentFunctionSymbol->VariableType == (unsigned long long)VARIABLE_TYPE_VOID)
            {
                *Error = SCRIPT_ENGINE_ERROR_VOID_FUNCTION_RETURNING_VALUE;
                break;
            }

            PushSymbol(CodeBuffer, OperatorSymbol);

            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);
            PushSymbol(CodeBuffer, Op0Symbol);
            FreeTemp(Op0);

            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
        }

        else if (!strcmp(Operator->Value, "@CALL_USER_DEFINED_FUNCTION"))
        {
            PSYMBOL      Symbol        = NULL;
            unsigned int i             = 0;
            PTOKEN       FunctionToken = Top(MatchedStack);
            BOOL         Found         = FALSE;

            //
            // find function's address
            //
            for (; i < CodeBuffer->Pointer;)
            {
                Symbol = CodeBuffer->Head + i;

                if (Symbol->Type == SYMBOL_USER_DEFINED_FUNCTION_TYPE && !strcmp((const char *)FunctionToken->Value, (const char *)Symbol->Value))
                {
                    Found = TRUE;
                    break;
                }
                else if (Symbol->Type == SYMBOL_STRING_TYPE || Symbol->Type == SYMBOL_WSTRING_TYPE)
                {
                    int temp = GetSymbolHeapSize(Symbol);
                    i += temp;
                }
                else
                {
                    i++;
                }
            }

            if (!Found)
            {
                *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_FUNCTION;
                break;
            }

            FunctionToken->Type = FUNCTION_TYPE;
            PushSymbol(CodeBuffer, OperatorSymbol);
        }
        else if (!strcmp(Operator->Value, "@CALL_USER_DEFINED_FUNCTION_PARAMETER"))
        {
            // will rewrite here to input variable's type
            PushSymbol(CodeBuffer, OperatorSymbol);

            Op0Symbol = ToSymbol(Top(MatchedStack), Error);
            PushSymbol(CodeBuffer, Op0Symbol);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
        }
        else if (!strcmp(Operator->Value, "@END_OF_CALLING_USER_DEFINED_FUNCTION_WITHOUT_RETURNING_VALUE") || !strcmp(Operator->Value, "@END_OF_CALLING_USER_DEFINED_FUNCTION_WITH_RETURNING_VALUE"))
        {
            PSYMBOL      Symbol                    = NULL;
            unsigned int i                         = 0;
            int          TargetFunctionVariableNum = 0;
            int          VariableNum               = 0;
            PTOKEN       FunctionToken             = NULL;
            BOOL         Found                     = FALSE;
            PTOKEN       ReturnValueToken          = NULL;

            while (MatchedStack->Pointer > 0)
            {
                FunctionToken = Pop(MatchedStack);

                if (FunctionToken->Type == FUNCTION_TYPE)
                {
                    break;
                }
                else
                {
                    VariableNum++;
                    // RemoveToken(&FunctionToken);
                }
            }

            //
            // find function's address
            //
            for (; i < CodeBuffer->Pointer;)
            {
                Symbol = CodeBuffer->Head + i;

                if (Symbol->Type == SYMBOL_USER_DEFINED_FUNCTION_TYPE && !strcmp((const char *)FunctionToken->Value, (const char *)Symbol->Value))
                {
                    Found = TRUE;
                    break;
                }
                else if (Symbol->Type == SYMBOL_STRING_TYPE || Symbol->Type == SYMBOL_WSTRING_TYPE)
                {
                    int temp = GetSymbolHeapSize(Symbol);
                    i += temp;
                }
                else
                {
                    i++;
                }
            }

            if (!Found)
            {
                *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_FUNCTION;
                break;
            }

            // will rewrite here
            for (unsigned int j = 0; j < UserDefinedFunctions->Pointer; j++)
            {
                Symbol = UserDefinedFunctions->Head + j;
                if (Symbol->Type == SYMBOL_USER_DEFINED_FUNCTION_TYPE && !strcmp((const char *)FunctionToken->Value, (const char *)Symbol->Value))
                {
                    j++;
                    Symbol = UserDefinedFunctions->Head + j;
                    while (Symbol->Type != SYMBOL_USER_DEFINED_FUNCTION_TYPE && j < UserDefinedFunctions->Pointer)
                    {
                        TargetFunctionVariableNum++;
                        j++;
                        Symbol = UserDefinedFunctions->Head + j;
                    }
                    goto ExitLoop;
                }
            }

        ExitLoop:

            if (VariableNum != TargetFunctionVariableNum)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }

            //
            // Add call user-defined function instruction
            //
            PushSymbol(CodeBuffer, OperatorSymbol);

            //
            // Add function's address
            //
            PSYMBOL JumpAddressSymbol = NewSymbol();
            JumpAddressSymbol->Type   = SYMBOL_NUM_TYPE;
            JumpAddressSymbol->Value  = i;
            PushSymbol(CodeBuffer, JumpAddressSymbol);
            RemoveSymbol(&JumpAddressSymbol);

            //
            // Add return address
            //
            JumpAddressSymbol        = NewSymbol();
            JumpAddressSymbol->Type  = SYMBOL_RETURN_ADDRESS_TYPE;
            JumpAddressSymbol->Value = CodeBuffer->Pointer + 1;
            PushSymbol(CodeBuffer, JumpAddressSymbol);
            RemoveSymbol(&JumpAddressSymbol);

            if (!strcmp(Operator->Value, "@END_OF_CALLING_USER_DEFINED_FUNCTION_WITH_RETURNING_VALUE"))
            {
                if (FunctionToken->Type == (TOKEN_TYPE)VARIABLE_TYPE_VOID)
                {
                    *Error = SCRIPT_ENGINE_ERROR_VOID_FUNCTION_RETURNING_VALUE;
                    break;
                }

                //
                // Add return variable symbol
                //
                Temp = NewTemp(Error, CurrentFunctionSymbol);
                Push(MatchedStack, Temp);

                PSYMBOL Symbol = NewSymbol();
                Symbol->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
                Symbol->Value  = FUNC_MOV_RETURN_VALUE;
                PushSymbol(CodeBuffer, Symbol);
                RemoveSymbol(&Symbol);

                TempSymbol = ToSymbol(Temp, Error);
                PushSymbol(CodeBuffer, TempSymbol);

                if (*Error != SCRIPT_ENGINE_ERROR_FREE)
                {
                    break;
                }
            }

            RemoveToken(&FunctionToken);
        }
        else if (!strcmp(Operator->Value, "@MOV"))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            Op1 = Pop(MatchedStack);
            if (Op1->Type == GLOBAL_UNRESOLVED_ID)
            {
                Op1Symbol = NewSymbol();
                free((void *)Op1Symbol->Value);
                Op1Symbol->Value = NewGlobalIdentifier(Op1);
                SetType(&Op1Symbol->Type, SYMBOL_GLOBAL_ID_TYPE);
            }
            else if (Op1->Type == LOCAL_UNRESOLVED_ID)
            {
                Op1Symbol = NewSymbol();
                free((void *)Op1Symbol->Value);
                Op1Symbol->Value = NewLocalIdentifier(Op1);
                SetType(&Op1Symbol->Type, SYMBOL_LOCAL_ID_TYPE);
            }
            else
            {
                Op1Symbol = ToSymbol(Op1, Error);
            }

            if (MatchedStack->Pointer > 0)
            {
                if (Top(MatchedStack)->Type == INPUT_VARIABLE_TYPE)
                {
                    VariableType = HandleType(MatchedStack);

                    if (VariableType == VARIABLE_TYPE_UNKNOWN)
                    {
                        *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                        break;
                    }

                    Op1Symbol->VariableType = (unsigned long long)VariableType;
                }
            }

            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, Op1Symbol);

            //
            // Free the operand if it is a temp value
            //
            FreeTemp(Op0);
            FreeTemp(Op1);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
        }
        else if (IsType2Func(Operator))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            PushSymbol(CodeBuffer, Op0Symbol);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
        }
        else if (IsType1Func(Operator))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            Temp = NewTemp(Error, CurrentFunctionSymbol);
            Push(MatchedStack, Temp);
            TempSymbol = ToSymbol(Temp, Error);

            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, TempSymbol);

            FreeTemp(Op0);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
        }
        else if (IsType4Func(Operator))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);
            PSYMBOL_BUFFER TempStack    = NewSymbolBuffer();
            UINT32         OperandCount = 0;
            do
            {
                if (Op1)
                {
                    RemoveToken(&Op1);
                }
                Op1 = Pop(MatchedStack);
                if (Op1->Type != SEMANTIC_RULE)
                {
                    Op1Symbol = ToSymbol(Op1, Error);

                    FreeTemp(Op1);
                    PushSymbol(TempStack, Op1Symbol);
                    RemoveSymbol(&Op1Symbol);

                    OperandCount++;
                    if (*Error != SCRIPT_ENGINE_ERROR_FREE)
                    {
                        RemoveSymbolBuffer(TempStack);
                        break;
                    }
                }

            } while (!(Op1->Type == SEMANTIC_RULE && !strcmp(Op1->Value, "@VARGSTART")));

            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);
            FreeTemp(Op0);

            char * Format = Op0->Value;

            PSYMBOL OperandCountSymbol = NewSymbol();
            OperandCountSymbol->Type   = SYMBOL_VARIABLE_COUNT_TYPE;
            OperandCountSymbol->Value  = OperandCount;

            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, OperandCountSymbol);

            RemoveSymbol(&OperandCountSymbol);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                RemoveSymbolBuffer(TempStack);
                break;
            }

            unsigned int FirstArgPointer = CodeBuffer->Pointer;

            PSYMBOL      Symbol;
            unsigned int ArgCount = TempStack->Pointer;
            for (int i = TempStack->Pointer - 1; i >= 0; i--)
            {
                Symbol = TempStack->Head + i;
                PushSymbol(CodeBuffer, Symbol);
            }
            PSYMBOL FirstArg = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                                         (unsigned long long)(FirstArgPointer * sizeof(SYMBOL)));
            RemoveSymbolBuffer(TempStack);

            UINT32 i   = 0;
            char * Str = Format;
            do
            {
                //
                // Not the best way but some how for optimization
                //
                if (*Str == '%')
                {
                    CHAR Temp = *(Str + 1);

                    if (Temp == 'd' || Temp == 'i' || Temp == 'u' || Temp == 'o' ||
                        Temp == 'x' || Temp == 'c' || Temp == 'p' || Temp == 's' ||

                        !strncmp(Str, "%ws", 3) || !strncmp(Str, "%ls", 3) ||

                        !strncmp(Str, "%ld", 3) || !strncmp(Str, "%li", 3) ||
                        !strncmp(Str, "%lu", 3) || !strncmp(Str, "%lo", 3) ||
                        !strncmp(Str, "%lx", 3) ||

                        !strncmp(Str, "%hd", 3) || !strncmp(Str, "%hi", 3) ||
                        !strncmp(Str, "%hu", 3) || !strncmp(Str, "%ho", 3) ||
                        !strncmp(Str, "%hx", 3) ||

                        !strncmp(Str, "%lld", 4) || !strncmp(Str, "%lli", 4) ||
                        !strncmp(Str, "%llu", 4) || !strncmp(Str, "%llo", 4) ||
                        !strncmp(Str, "%llx", 4)

                    )
                    {
                        if (i < ArgCount)
                        {
                            Symbol = FirstArg + i;
                        }
                        else
                        {
                            *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                            break;
                        }
                        Symbol->Type &= 0xffffffff;
                        Symbol->Type |= (UINT64)(Str - Format - 1) << 32;
                        i++;
                    }
                }
                Str++;
            } while (*Str);
            if (i != ArgCount)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
            }

            if (*Error == SCRIPT_ENGINE_ERROR_SYNTAX)
            {
                break;
            }
        }
        else if (IsType5Func(Operator))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);
        }
        else if (!strcmp(Operator->Value, "@IGNORE_LVALUE"))
        {
            Op0 = Pop(MatchedStack);
        }
        else if (IsType6Func(Operator))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            Op1       = Pop(MatchedStack);
            Op1Symbol = ToSymbol(Op1, Error);

            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, Op1Symbol);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }

            Temp = NewTemp(Error, CurrentFunctionSymbol);
            Push(MatchedStack, Temp);
            TempSymbol = ToSymbol(Temp, Error);
            PushSymbol(CodeBuffer, TempSymbol);

            //
            // Free the operand if it is a temp value
            //
            FreeTemp(Op0);
            FreeTemp(Op1);
        }
        else if (IsType7Func(Operator))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            Op1       = Pop(MatchedStack);
            Op1Symbol = ToSymbol(Op1, Error);

            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, Op1Symbol);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
            //
            // Free the operand if it is a temp value
            //
            FreeTemp(Op0);
            FreeTemp(Op1);
        }
        else if (IsType8Func(Operator))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            Op1       = Pop(MatchedStack);
            Op1Symbol = ToSymbol(Op1, Error);

            Op2       = Pop(MatchedStack);
            Op2Symbol = ToSymbol(Op2, Error);

            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, Op1Symbol);
            PushSymbol(CodeBuffer, Op2Symbol);

            Temp = NewTemp(Error, CurrentFunctionSymbol);
            Push(MatchedStack, Temp);
            TempSymbol = ToSymbol(Temp, Error);
            PushSymbol(CodeBuffer, TempSymbol);

            FreeTemp(Op2);

            //
            // Free the operand if it is a temp value
            //
            FreeTemp(Op0);
            FreeTemp(Op1);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
        }
        else if (IsType14Func(Operator))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            Op1       = Pop(MatchedStack);
            Op1Symbol = ToSymbol(Op1, Error);

            Op2       = Pop(MatchedStack);
            Op2Symbol = ToSymbol(Op2, Error);

            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, Op1Symbol);
            PushSymbol(CodeBuffer, Op2Symbol);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
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
            Op0Symbol = ToSymbol(Op0, Error);

            Op1       = Pop(MatchedStack);
            Op1Symbol = ToSymbol(Op1, Error);

            Temp = NewTemp(Error, CurrentFunctionSymbol);
            Push(MatchedStack, Temp);
            TempSymbol = ToSymbol(Temp, Error);

            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, Op1Symbol);
            PushSymbol(CodeBuffer, TempSymbol);

            //
            // Free the operand if it is a temp value
            //
            FreeTemp(Op0);
            FreeTemp(Op1);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
        }
        else if (IsOneOperandOperator(Operator))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            PushSymbol(CodeBuffer, Op0Symbol);

            //
            // Free the operand if it is a temp value
            //
            FreeTemp(Op0);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
        }
        else if (!strcmp(Operator->Value, "@VARGSTART"))
        {
            PTOKEN OperatorCopy = CopyToken(Operator);
            Push(MatchedStack, OperatorCopy);
        }
        else if (!strcmp(Operator->Value, "@START_OF_IF"))
        {
            PTOKEN OperatorCopy = CopyToken(Operator);
            Push(MatchedStack, OperatorCopy);
        }
        else if (!strcmp(Operator->Value, "@JZ"))
        {
            // UINT64 CurrentPointer = CodeBuffer->Pointer;
            PushSymbol(CodeBuffer, OperatorSymbol);

            PSYMBOL JumpAddressSymbol = NewSymbol();
            JumpAddressSymbol->Type   = SYMBOL_NUM_TYPE;
            JumpAddressSymbol->Value  = 0xffffffffffffffff;
            PushSymbol(CodeBuffer, JumpAddressSymbol);
            RemoveSymbol(&JumpAddressSymbol);

            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            PushSymbol(CodeBuffer, Op0Symbol);

            char str[20] = {0};
            sprintf(str, "%llu", (UINT64)CodeBuffer->Pointer);
            PTOKEN CurrentAddressToken = NewToken(DECIMAL, str);
            Push(MatchedStack, CurrentAddressToken);

            FreeTemp(Op0);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
        }
        else if (!strcmp(Operator->Value, "@JMP_TO_END_AND_JZCOMPLETED"))
        {
            //
            // Set JZ jump address
            //
            UINT64  CurrentPointer           = CodeBuffer->Pointer;
            PTOKEN  JumpSemanticAddressToken = Pop(MatchedStack);
            UINT64  JumpSemanticAddress      = DecimalToInt(JumpSemanticAddressToken->Value);
            PSYMBOL JumpAddressSymbol        = (PSYMBOL)(CodeBuffer->Head + JumpSemanticAddress - 2);
            JumpAddressSymbol->Value         = CurrentPointer + 2;
            RemoveToken(&JumpSemanticAddressToken);

            //
            // Add jmp instruction to Code Buffer
            //
            PSYMBOL JumpInstruction = NewSymbol();
            JumpInstruction->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
            JumpInstruction->Value  = FUNC_JMP;
            PushSymbol(CodeBuffer, JumpInstruction);
            RemoveSymbol(&JumpInstruction);

            //
            // Add -1 decimal code to jump address
            //
            JumpAddressSymbol        = NewSymbol();
            JumpAddressSymbol->Type  = SYMBOL_NUM_TYPE;
            JumpAddressSymbol->Value = 0xffffffffffffffff;
            PushSymbol(CodeBuffer, JumpAddressSymbol);
            RemoveSymbol(&JumpAddressSymbol);

            //
            // push current pointer to stack
            //
            char str[20] = {0};
            sprintf(str, "%llu", CurrentPointer);
            PTOKEN CurrentAddressToken = NewToken(DECIMAL, str);
            Push(MatchedStack, CurrentAddressToken);
        }
        else if (!strcmp(Operator->Value, "@END_OF_IF"))
        {
            UINT64  CurrentPointer           = CodeBuffer->Pointer;
            PTOKEN  JumpSemanticAddressToken = Pop(MatchedStack);
            PSYMBOL JumpAddressSymbol;
            while (strcmp(JumpSemanticAddressToken->Value, "@START_OF_IF"))
            {
                UINT64 JumpSemanticAddress = DecimalToInt(JumpSemanticAddressToken->Value);
                JumpAddressSymbol          = (PSYMBOL)(CodeBuffer->Head + JumpSemanticAddress + 1);
                JumpAddressSymbol->Value   = CurrentPointer;

                RemoveToken(&JumpSemanticAddressToken);
                JumpSemanticAddressToken = Pop(MatchedStack);
            }
            RemoveToken(&JumpSemanticAddressToken);
        }
        else if (!strcmp(Operator->Value, "@START_OF_WHILE"))
        {
            //
            // Push @START_OF_WHILE token into matched stack
            //
            PTOKEN OperatorCopy = CopyToken(Operator);
            Push(MatchedStack, OperatorCopy);

            char str[20] = {0};
            sprintf(str, "%llu", (UINT64)CodeBuffer->Pointer);
            PTOKEN CurrentAddressToken = NewToken(DECIMAL, str);
            Push(MatchedStack, CurrentAddressToken);
        }
        else if (!strcmp(Operator->Value, "@START_OF_WHILE_COMMANDS"))
        {
            UINT64 CurrentPointer = CodeBuffer->Pointer;
            PTOKEN JzToken        = NewToken(SEMANTIC_RULE, "@JZ");

            RemoveSymbol(&OperatorSymbol);
            OperatorSymbol = ToSymbol(JzToken, Error);
            RemoveToken(&JzToken);

            PSYMBOL JumpAddressSymbol = NewSymbol();
            JumpAddressSymbol->Type   = SYMBOL_NUM_TYPE;
            JumpAddressSymbol->Value  = 0xffffffffffffffff;

            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            PTOKEN StartOfWhileToken = Pop(MatchedStack);

            char str[20];
            sprintf(str, "%llu", CurrentPointer + 1);
            PTOKEN CurrentAddressToken = NewToken(DECIMAL, str);
            Push(MatchedStack, CurrentAddressToken);
            Push(MatchedStack, StartOfWhileToken);

            PushSymbol(CodeBuffer, OperatorSymbol);
            PushSymbol(CodeBuffer, JumpAddressSymbol);

            PushSymbol(CodeBuffer, Op0Symbol);

            RemoveSymbol(&JumpAddressSymbol);

            FreeTemp(Op0);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
        }
        else if (!strcmp(Operator->Value, "@END_OF_WHILE"))
        {
            //
            // Add jmp instruction to Code Buffer
            //
            PSYMBOL JumpInstruction = NewSymbol();
            JumpInstruction->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
            JumpInstruction->Value  = FUNC_JMP;
            PushSymbol(CodeBuffer, JumpInstruction);
            RemoveSymbol(&JumpInstruction);

            //
            // Add jmp address to Code buffer
            //
            PTOKEN  JumpAddressToken  = Pop(MatchedStack);
            UINT64  JumpAddress       = DecimalToInt(JumpAddressToken->Value);
            PSYMBOL JumpAddressSymbol = ToSymbol(JumpAddressToken, Error);

            PushSymbol(CodeBuffer, JumpAddressSymbol);
            RemoveSymbol(&JumpAddressSymbol);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }

            //
            // Set jumps addresses
            //

            UINT64 CurrentPointer = CodeBuffer->Pointer;

            do
            {
                RemoveToken(&JumpAddressToken);
                JumpAddressToken = Pop(MatchedStack);
                if (!strcmp(JumpAddressToken->Value, "@START_OF_WHILE"))
                {
                    break;
                }
                JumpAddress              = DecimalToInt(JumpAddressToken->Value);
                JumpAddressSymbol        = (PSYMBOL)(CodeBuffer->Head + JumpAddress);
                JumpAddressSymbol->Value = CurrentPointer;

            } while (TRUE);
            RemoveToken(&JumpAddressToken);
        }
        else if (!strcmp(Operator->Value, "@START_OF_DO_WHILE"))
        {
            //
            // Push @START_OF_DO_WHILE token into matched stack
            //
            PTOKEN OperatorCopy = CopyToken(Operator);
            Push(MatchedStack, OperatorCopy);

            char str[20];
            sprintf(str, "%llu", (UINT64)CodeBuffer->Pointer);
            PTOKEN CurrentAddressToken = NewToken(DECIMAL, str);
            Push(MatchedStack, CurrentAddressToken);
        }
        else if (!strcmp(Operator->Value, "@END_OF_DO_WHILE"))
        {
            //
            // Add jmp instruction to Code Buffer
            //
            PSYMBOL JumpInstruction = NewSymbol();
            JumpInstruction->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
            JumpInstruction->Value  = FUNC_JNZ;
            PushSymbol(CodeBuffer, JumpInstruction);
            RemoveSymbol(&JumpInstruction);

            //
            // Add Op0 to CodeBuffer
            //
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            //
            // Add jmp address to Code buffer
            //
            PTOKEN JumpAddressToken = Pop(MatchedStack);
            UINT64 JumpAddress      = DecimalToInt(JumpAddressToken->Value);

            PSYMBOL JumpAddressSymbol = ToSymbol(JumpAddressToken, Error);

            PushSymbol(CodeBuffer, JumpAddressSymbol);
            PushSymbol(CodeBuffer, Op0Symbol);

            RemoveSymbol(&JumpAddressSymbol);

            FreeTemp(Op0);

            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }

            //
            // Set jumps addresses
            //

            UINT64 CurrentPointer = CodeBuffer->Pointer;

            do
            {
                RemoveToken(&JumpAddressToken);
                JumpAddressToken = Pop(MatchedStack);
                if (!strcmp(JumpAddressToken->Value, "@START_OF_DO_WHILE"))
                {
                    break;
                }
                JumpAddress = DecimalToInt(JumpAddressToken->Value);

#ifdef _SCRIPT_ENGINE_LL1_DBG_EN
                printf("Jz Jump Address = %d\n", JumpAddress);
#endif
                JumpAddressSymbol        = (PSYMBOL)(CodeBuffer->Head + JumpAddress);
                JumpAddressSymbol->Value = CurrentPointer;

            } while (TRUE);
            RemoveToken(&JumpAddressToken);
        }
        else if (!strcmp(Operator->Value, "@START_OF_FOR"))
        {
            //
            // Push @START_OF_FOR token into matched stack
            //
            PTOKEN OperatorCopy = CopyToken(Operator);
            Push(MatchedStack, OperatorCopy);

            //
            // Push current pointer into matched stack
            //
            char str[20] = {0};
            sprintf(str, "%llu", (UINT64)CodeBuffer->Pointer);
            PTOKEN CurrentAddressToken = NewToken(DECIMAL, str);
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
            RemoveSymbol(&JnzInstruction);

            //
            // Add JZ address to Code CodeBuffer
            //
            PSYMBOL JnzAddressSymbol = NewSymbol();
            JnzAddressSymbol->Type   = SYMBOL_NUM_TYPE;
            JnzAddressSymbol->Value  = 0xffffffffffffffff;
            PushSymbol(CodeBuffer, JnzAddressSymbol);
            RemoveSymbol(&JnzAddressSymbol);

            //
            // Add Op0 to CodeBuffer
            //
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            PushSymbol(CodeBuffer, Op0Symbol);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
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
            RemoveSymbol(&JumpInstruction);

            //
            // Add jmp address to Code CodeBuffer
            //
            PSYMBOL JumpAddressSymbol = NewSymbol();
            JumpAddressSymbol->Type   = SYMBOL_NUM_TYPE;
            JumpAddressSymbol->Value  = 0xffffffffffffffff;
            PushSymbol(CodeBuffer, JumpAddressSymbol);
            RemoveSymbol(&JumpAddressSymbol);

            //
            // Pop start_of_for address
            //
            PTOKEN StartOfForAddressToken = Pop(MatchedStack);

            //
            // Push current pointer into matched stack
            //
            char str[20] = {0};
            sprintf(str, "%llu", (UINT64)CodeBuffer->Pointer);
            PTOKEN CurrentAddressToken = NewToken(DECIMAL, str);
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
            RemoveSymbol(&JumpInstruction);

            //
            // Add jmp address to Code buffer
            //
            PTOKEN JumpAddressToken = Pop(MatchedStack);
            UINT64 JumpAddress      = DecimalToInt(JumpAddressToken->Value);

            PSYMBOL JumpAddressSymbol = ToSymbol(JumpAddressToken, Error);

            PushSymbol(CodeBuffer, JumpAddressSymbol);
            RemoveToken(&JumpAddressToken);
            RemoveSymbol(&JumpAddressSymbol);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }

            //
            // Set jmp address
            //
            UINT64 CurrentPointer = CodeBuffer->Pointer;
            JumpAddressToken      = Pop(MatchedStack);
            JumpAddress           = DecimalToInt(JumpAddressToken->Value);

            JumpAddressSymbol        = (PSYMBOL)(CodeBuffer->Head + JumpAddress - 1);
            JumpAddressSymbol->Value = CurrentPointer;

            //
            // Push address of jz address to stack
            //
            char str[20] = {0};
            sprintf(str, "%llu", JumpAddress - 4);
            PTOKEN JzAddressToken = NewToken(DECIMAL, str);
            Push(MatchedStack, JzAddressToken);

            //
            // Push @INC_DEC token to matched stack
            //
            PTOKEN IncDecToken = NewToken(SEMANTIC_RULE, "@INC_DEC");
            Push(MatchedStack, IncDecToken);

            //
            // Push start of inc_dec address to matched stack
            //
            Push(MatchedStack, JumpAddressToken);
        }
        else if (!strcmp(Operator->Value, "@END_OF_FOR"))
        {
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
            RemoveSymbol(&JumpInstruction);

            //
            // Add jmp address to Code buffer
            //
            PTOKEN JumpAddressToken = Pop(MatchedStack);
            UINT64 JumpAddress      = DecimalToInt(JumpAddressToken->Value);

            PSYMBOL JumpAddressSymbol = ToSymbol(JumpAddressToken, Error);

            PushSymbol(CodeBuffer, JumpAddressSymbol);
            RemoveSymbol(&JumpAddressSymbol);
            RemoveToken(&JumpAddressToken);

            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }

            JumpAddressToken = Pop(MatchedStack);

            //
            // Set jumps addresses
            //

            UINT64 CurrentPointer = CodeBuffer->Pointer;

            do
            {
                RemoveToken(&JumpAddressToken);
                JumpAddressToken = Pop(MatchedStack);
                if (!strcmp(JumpAddressToken->Value, "@START_OF_FOR"))
                {
                    break;
                }
                else
                {
                    JumpAddress = DecimalToInt(JumpAddressToken->Value);

                    JumpAddressSymbol        = (PSYMBOL)(CodeBuffer->Head + JumpAddress);
                    JumpAddressSymbol->Value = CurrentPointer;
                }

            } while (TRUE);
            RemoveToken(&JumpAddressToken);
        }
        else if (!strcmp(Operator->Value, "@BREAK"))
        {
            //
            // Pop Objects from stack while reaching @START_OF_*
            //

            PTOKEN_LIST TempStack = NewTokenList();
            PTOKEN      TempToken;
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

                    UINT64 CurrentPointer = CodeBuffer->Pointer + 1;
                    char   str[20];
                    sprintf(str, "%llu", CurrentPointer);
                    PTOKEN CurrentAddressToken = NewToken(DECIMAL, str);
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
                    RemoveSymbol(&JumpInstruction);

                    //
                    // Add jmp address to Code buffer
                    //
                    PSYMBOL JumpAddressSymbol = NewSymbol();
                    JumpAddressSymbol->Type   = SYMBOL_NUM_TYPE;
                    JumpAddressSymbol->Value  = 0xffffffffffffffff;
                    PushSymbol(CodeBuffer, JumpAddressSymbol);
                    RemoveSymbol(&JumpAddressSymbol);

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
                    *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                    RemoveToken(&TempToken);
                    break;
                }
                else
                {
                    Push(TempStack, TempToken);
                }

            } while (TRUE);
            RemoveTokenList(TempStack);
        }
        else if (!strcmp(Operator->Value, "@CONTINUE"))
        {
            //
            // Pop Objects from stack while reaching @INC_DEC
            //
            PTOKEN_LIST TempStack = NewTokenList();
            PTOKEN      TempToken;
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
                    RemoveSymbol(&JumpInstruction);

                    //
                    // Add jmp address to Code buffer
                    //
                    TempToken = Pop(TempStack);
                    Push(MatchedStack, TempToken);

                    PSYMBOL JumpAddressSymbol = NewSymbol();
                    JumpAddressSymbol->Type   = SYMBOL_NUM_TYPE;
                    JumpAddressSymbol->Value  = DecimalToInt(TempToken->Value);
                    PushSymbol(CodeBuffer, JumpAddressSymbol);
                    RemoveSymbol(&JumpAddressSymbol);

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
                    *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                    break;
                }
                else
                {
                    Push(TempStack, TempToken);
                }

            } while (TRUE);

            RemoveTokenList(TempStack);
        }
        else if (IsType9Func(Operator))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            Temp = NewTemp(Error, CurrentFunctionSymbol);
            Push(MatchedStack, Temp);
            TempSymbol = ToSymbol(Temp, Error);

            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, TempSymbol);

            FreeTemp(Op0);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
        }
        else if (IsType10Func(Operator))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            Op1       = Pop(MatchedStack);
            Op1Symbol = ToSymbol(Op1, Error);

            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, Op1Symbol);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }

            Temp = NewTemp(Error, CurrentFunctionSymbol);
            Push(MatchedStack, Temp);
            TempSymbol = ToSymbol(Temp, Error);
            PushSymbol(CodeBuffer, TempSymbol);

            //
            // Free the operand if it is a temp value
            //
            FreeTemp(Op0);
            FreeTemp(Op1);
        }
        else if (IsType11Func(Operator))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            Op1       = Pop(MatchedStack);
            Op1Symbol = ToSymbol(Op1, Error);

            PTOKEN  Op2       = Pop(MatchedStack);
            PSYMBOL Op2Symbol = ToSymbol(Op2, Error);

            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, Op1Symbol);
            PushSymbol(CodeBuffer, Op2Symbol);

            Temp = NewTemp(Error, CurrentFunctionSymbol);
            Push(MatchedStack, Temp);
            TempSymbol = ToSymbol(Temp, Error);
            PushSymbol(CodeBuffer, TempSymbol);

            FreeTemp(Op2);

            //
            // Free the operand if it is a temp value
            //
            FreeTemp(Op0);
            FreeTemp(Op1);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
        }
        else if (IsType12Func(Operator))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            Temp = NewTemp(Error, CurrentFunctionSymbol);
            Push(MatchedStack, Temp);
            TempSymbol = ToSymbol(Temp, Error);

            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, TempSymbol);

            FreeTemp(Op0);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
        }
        else if (IsType13Func(Operator))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            Op1       = Pop(MatchedStack);
            Op1Symbol = ToSymbol(Op1, Error);

            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, Op1Symbol);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }

            Temp = NewTemp(Error, CurrentFunctionSymbol);
            Push(MatchedStack, Temp);
            TempSymbol = ToSymbol(Temp, Error);
            PushSymbol(CodeBuffer, TempSymbol);

            //
            // Free the operand if it is a temp value
            //
            FreeTemp(Op0);
            FreeTemp(Op1);
        }
        else
        {
            *Error = SCRIPT_ENGINE_ERROR_UNHANDLED_SEMANTIC_RULE;
        }
        break;
    }

#ifdef _SCRIPT_ENGINE_CODEGEN_DBG_EN
    //
    // Print Debug Info
    //
    printf("Semantic Stack:\n");
    PrintTokenList(MatchedStack);
    printf("\n");

    printf("Code Buffer:\n");
    PrintSymbolBuffer(CodeBuffer);
    printf("------------------------------------------\n\n");
#endif

    if (Op0)
        RemoveToken(&Op0);

    if (Op1)
        RemoveToken(&Op1);

    if (Op2)
        RemoveToken(&Op2);

    RemoveSymbol(&OperatorSymbol);

    if (Op0Symbol)
        RemoveSymbol(&Op0Symbol);

    if (Op1Symbol)
        RemoveSymbol(&Op1Symbol);

    if (Op2Symbol)
        RemoveSymbol(&Op2Symbol);

    if (TempSymbol)
        RemoveSymbol(&TempSymbol);

    return;
}

/**
 * @brief Computes the boolean expression length starting from the current input position
 *
 * @param str
 * @param WaitForWaitStatementBooleanExpression
 * @param CurrentIn
 * @return UINT64
 */
UINT64
BooleanExpressionExtractEnd(char * str, BOOL * WaitForWaitStatementBooleanExpression, PTOKEN CurrentIn)
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
        if (!strcmp(CurrentIn->Value, "("))
        {
            OpenParanthesesCount++;
        }
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
 * @brief LALR parser used for parsing boolean expression
 *
 * @param BooleanExpressionSize
 * @param FirstToken
 * @param MatchedStack
 * @param CodeBuffer
 * @param str
 * @param c
 * @param Error
 */
void
ScriptEngineBooleanExpresssionParse(
    UINT64                    BooleanExpressionSize,
    PTOKEN                    FirstToken,
    PTOKEN_LIST               MatchedStack,
    PSYMBOL_BUFFER            UserDefinedFunctions,
    PSYMBOL_BUFFER            CodeBuffer,
    char *                    str,
    char *                    c,
    PSCRIPT_ENGINE_ERROR_TYPE Error)
{
    PTOKEN_LIST Stack = NewTokenList();

    PTOKEN State = NewToken(STATE_ID, "0");
    Push(Stack, State);

#ifdef _SCRIPT_ENGINE_LALR_DBG_EN
    printf("Boolean Expression: ");
    printf("%s", FirstToken->Value);
    for (int i = InputIdx - 1; i < BooleanExpressionSize; i++)
    {
        printf("%c", str[i]);
    }
    printf("\n\n");
#endif

    //
    // End of File Token
    //
    PTOKEN EndToken = NewToken(END_OF_STACK, "$");

    PTOKEN CurrentIn = CopyToken(FirstToken);

    PTOKEN TopToken     = NULL;
    PTOKEN Lhs          = NULL;
    PTOKEN Temp         = NULL;
    PTOKEN Operand      = NULL;
    PTOKEN SemanticRule = NULL;

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
        StateId        = (int)DecimalToSignedInt(TopToken->Value);
        if (StateId == INVALID || TerminalId < 0)
        {
            *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
            break;
        }
        Action = LalrActionTable[StateId][TerminalId];

#ifdef _SCRIPT_ENGINE_LALR_DBG_EN
        printf("Stack :\n");
        PrintTokenList(Stack);
        printf("Action : %d\n\n", Action);
#endif
        if (Action == LALR_ACCEPT)
        {
            *Error = SCRIPT_ENGINE_ERROR_FREE;
            break;
        }
        if (Action == INVALID)
        {
            *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
            break;
        }
        if (Action == 0)
        {
            *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
            break;
        }
        else if (Action >= 0) // Shift
        {
            StateId = Action;
            Push(Stack, CurrentIn);

            char buffer[20] = {0};
            sprintf(buffer, "%d", StateId);
            State = NewToken(STATE_ID, buffer);
            Push(Stack, State);

            InputIdxTemp = InputIdx;
            Ctemp        = *c;

            CurrentIn = Scan(str, c);
            if (InputIdx - 1 > BooleanExpressionSize)
            {
                InputIdx = InputIdxTemp;
                *c       = Ctemp;

                RemoveToken(&CurrentIn);

                CurrentIn = CopyToken(EndToken);
            }
        }
        else if (Action < 0) // Reduce
        {
            StateId      = -Action;
            Lhs          = (PTOKEN)&LalrLhs[StateId - 1];
            RhsSize      = LalrGetRhsSize(StateId - 1);
            SemanticRule = (PTOKEN)&LalrSemanticRules[StateId - 1];

            for (int i = 0; i < 2 * RhsSize; i++)
            {
                Temp = Pop(Stack);
                if (SemanticRule->Type == SEMANTIC_RULE && !strcmp(SemanticRule->Value, "@PUSH"))
                {
                    if (LalrIsOperandType(Temp))
                    {
                        Operand = Temp;
                        Push(MatchedStack, Operand);
                    }
                    else
                    {
                        RemoveToken(&Temp);
                    }
                }
                else
                {
                    RemoveToken(&Temp);
                }
            }
            if (SemanticRule->Type == SEMANTIC_RULE)
            {
                if (!strcmp(SemanticRule->Value, "@PUSH"))
                {
                }
                else
                {
                    CodeGen(MatchedStack, UserDefinedFunctions, CodeBuffer, SemanticRule, Error);
                    if (*Error != SCRIPT_ENGINE_ERROR_FREE)
                    {
                        break;
                    }
                }
            }

            Temp    = Top(Stack);
            StateId = (int)DecimalToSignedInt(Temp->Value);

            Goto = LalrGotoTable[StateId][LalrGetNonTerminalId(Lhs)];

            PTOKEN LhsCopy = CopyToken(Lhs);

            char buffer[20] = {0};
            sprintf(buffer, "%d", Goto);
            State = NewToken(STATE_ID, buffer);
            Push(Stack, LhsCopy);
            Push(Stack, State);
        }
    }

    if (EndToken)
        RemoveToken(&EndToken);

    if (Stack)
        RemoveTokenList(Stack);

    if (CurrentIn)
        RemoveToken(&CurrentIn);

    return;
}

/**
 * @brief Allocates a new SYMBOL and returns the reference to it
 *
 * @return PSYMBOL
 */
PSYMBOL
NewSymbol(void)
{
    PSYMBOL Symbol;
    Symbol = (PSYMBOL)malloc(sizeof(SYMBOL));

    if (Symbol == NULL)
    {
        //
        // There was an error allocating buffer
        //
        return NULL;
    }

    Symbol->Value        = 0;
    Symbol->Len          = 0;
    Symbol->Type         = 0;
    Symbol->VariableType = 0;
    return Symbol;
}

/**
 * @brief Allocates a new SYMBOL with string type and returns the reference to it
 *
 * @param value
 * @return PSYMBOL
 */
PSYMBOL
NewStringSymbol(PTOKEN Token)
{
    PSYMBOL Symbol;
    int     BufferSize = (3 * sizeof(unsigned long long) + Token->Len) / sizeof(SYMBOL) + 1;
    Symbol             = (PSYMBOL)calloc(sizeof(SYMBOL), BufferSize);

    if (Symbol == NULL)
    {
        //
        // There was an error allocating buffer
        //
        return NULL;
    }

    memcpy(&Symbol->Value, Token->Value, Token->Len);
    SetType(&Symbol->Type, SYMBOL_STRING_TYPE);
    Symbol->Len          = Token->Len;
    Symbol->VariableType = 0;
    return Symbol;
}

/**
 * @brief Allocates a new SYMBOL with wstring type and returns the reference to it
 *
 * @param value
 * @return PSYMBOL
 */
PSYMBOL
NewWstringSymbol(PTOKEN Token)
{
    PSYMBOL Symbol;
    int     BufferSize = (3 * sizeof(unsigned long long) + Token->Len) / sizeof(SYMBOL) + 1;
    Symbol             = (PSYMBOL)malloc(BufferSize * sizeof(SYMBOL));

    if (Symbol == NULL)
    {
        //
        // There was an error allocating buffer
        //
        return NULL;
    }

    memcpy(&Symbol->Value, Token->Value, Token->Len);
    SetType(&Symbol->Type, SYMBOL_WSTRING_TYPE);
    Symbol->Len          = Token->Len;
    Symbol->VariableType = 0;
    return Symbol;
}

/**
 * @brief
 *
 * @return PSYMBOL
 */
PSYMBOL
NewFunctionSymbol(char * FunctionName, VARIABLE_TYPE * VariableType)
{
    int     Length = (int)strlen(FunctionName);
    PSYMBOL Symbol = NewSymbol();
    Symbol         = (PSYMBOL)malloc(sizeof(SYMBOL));

    if (Symbol == NULL)
    {
        //
        // There was an error allocating buffer
        //
        return NULL;
    }

    Symbol->Value = (unsigned long long)calloc(Length + 1, sizeof(char));

    if (Symbol->Value == 0ull)
    {
        //
        // There was an error allocating buffer
        //
        return NULL;
    }

    memcpy((void *)Symbol->Value, FunctionName, Length);
    Symbol->Len          = Length;
    Symbol->Type         = SYMBOL_USER_DEFINED_FUNCTION_TYPE;
    Symbol->VariableType = (long long unsigned)VariableType;
    return Symbol;
}

/**
 * @brief
 *
 * @return PSYMBOL
 */

/**
 * @brief Returns the number of SYMBOL objects (24 bytes) allocated by string or wstring sybmol
 *
 * @param Symbol
 * @return unsigned int
 */
unsigned int
GetSymbolHeapSize(PSYMBOL Symbol)
{
    int Temp = (3 * sizeof(unsigned long long) + (int)Symbol->Len) / sizeof(SYMBOL) + 1;
    return Temp;
}

/**
 * @brief Frees the memory allocate by this Symbol
 *
 * @param Symbol
 */
void
RemoveSymbol(PSYMBOL * Symbol)
{
    free(*Symbol);
    *Symbol = NULL;
    return;
}

/**
 * @brief Prints symbol
 *
 * @param Symbol
 */
void
PrintSymbol(PSYMBOL Symbol)
{
    if (Symbol->Type == SYMBOL_STRING_TYPE)
    {
        printf("Type:%llx, Value:0x%p\n", Symbol->Type, &Symbol->Value);
    }
    else
    {
        printf("Type:%llx, Value:0x%llx\n", Symbol->Type, Symbol->Value);
    }
}

/**
 * @brief Converts Token to Symbol and returns the reference to it
 *
 * @param Token
 * @param Error
 * @return PSYMBOL
 */
PSYMBOL
ToSymbol(PTOKEN Token, PSCRIPT_ENGINE_ERROR_TYPE Error)
{
    PSYMBOL Symbol = NewSymbol();
    switch (Token->Type)
    {
    case GLOBAL_ID:
        Symbol->Value = GetGlobalIdentifierVal(Token);
        SetType(&Symbol->Type, SYMBOL_GLOBAL_ID_TYPE);
        return Symbol;

    case LOCAL_ID:
        Symbol->Value = GetLocalIdentifierVal(Token);
        SetType(&Symbol->Type, SYMBOL_LOCAL_ID_TYPE);
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

    case STACK_TEMP:
        Symbol->Value = DecimalToInt(Token->Value);
        SetType(&Symbol->Type, SYMBOL_STACK_TEMP_TYPE);
        return Symbol;

    case STRING:
        RemoveSymbol(&Symbol);
        return NewStringSymbol(Token);

    case WSTRING:
        RemoveSymbol(&Symbol);
        return NewWstringSymbol(Token);

    case FUNCTION_PARAMETER_ID:
        Symbol->Value = GetFunctionParameterIdentifier(Token);
        SetType(&Symbol->Type, SYMBOL_FUNCTION_PARAMETER_ID_TYPE);
        return Symbol;

    default:
        *Error        = SCRIPT_ENGINE_ERROR_UNRESOLVED_VARIABLE;
        Symbol->Type  = INVALID;
        Symbol->Value = INVALID;
        return Symbol;
    }
}

/**
 * @brief allocates a new Symbol Buffer and returns the reference to it
 *
 * @return PSYMBOL_BUFFER
 */
PSYMBOL_BUFFER
NewSymbolBuffer(void)
{
    PSYMBOL_BUFFER SymbolBuffer;
    SymbolBuffer = (PSYMBOL_BUFFER)malloc(sizeof(*SymbolBuffer));

    if (SymbolBuffer == NULL)
    {
        //
        // There was an error allocating buffer
        //
        return NULL;
    }

    SymbolBuffer->Pointer = 0;
    SymbolBuffer->Size    = SYMBOL_BUFFER_INIT_SIZE;
    SymbolBuffer->Head    = (PSYMBOL)malloc(SymbolBuffer->Size * sizeof(SYMBOL));
    SymbolBuffer->Message = NULL;
    return SymbolBuffer;
}

/**
 * @brief Frees the memory allocated by SymbolBuffer
 *
 * @param SymbolBuffer
 */
void
RemoveSymbolBuffer(PSYMBOL_BUFFER SymbolBuffer)
{
    free(SymbolBuffer->Message);
    free(SymbolBuffer->Head);
    free(SymbolBuffer);
}

/**
 * @brief Gets a symbol and push it into the symbol buffer
 *
 * @param SymbolBuffer
 * @param Symbol
 * @return PSYMBOL_BUFFER
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

    if (Symbol->Type == SYMBOL_STRING_TYPE || Symbol->Type == SYMBOL_WSTRING_TYPE)
    {
        //
        // Update Pointer
        //
        SymbolBuffer->Pointer += GetSymbolHeapSize(Symbol);

        //
        // Handle Overflow
        //
        if (SymbolBuffer->Pointer >= SymbolBuffer->Size - 1)
        {
            //
            // Calculate new size for the symbol B
            //
            unsigned int NewSize = SymbolBuffer->Size;
            do
            {
                NewSize *= 2;
            } while (NewSize <= SymbolBuffer->Pointer);

            //
            // Allocate a new buffer for string list with doubled length
            //
            PSYMBOL NewHead = (PSYMBOL)malloc(NewSize * sizeof(SYMBOL));

            if (NewHead == NULL)
            {
                printf("err, could not allocate buffer");
                return NULL;
            }

            //
            // Copy old buffer to new buffer
            //
            memcpy(NewHead, SymbolBuffer->Head, SymbolBuffer->Size * sizeof(SYMBOL));

            //
            // Free old buffer
            //
            free(SymbolBuffer->Head);

            //
            // Update Head and size of SymbolBuffer
            //
            SymbolBuffer->Size = NewSize;
            SymbolBuffer->Head = NewHead;
        }
        WriteAddr       = (PSYMBOL)((uintptr_t)SymbolBuffer->Head + (uintptr_t)Pointer * (uintptr_t)sizeof(SYMBOL));
        WriteAddr->Type = Symbol->Type;
        WriteAddr->Len  = Symbol->Len;
        memcpy((char *)&WriteAddr->Value, (char *)&Symbol->Value, Symbol->Len);
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
        if (Pointer >= SymbolBuffer->Size - 1)
        {
            //
            // Allocate a new buffer for string list with doubled length
            //
            PSYMBOL NewHead = (PSYMBOL)malloc(2 * SymbolBuffer->Size * sizeof(SYMBOL));

            if (NewHead == NULL)
            {
                printf("err, could not allocate buffer");
                return NULL;
            }

            //
            // Copy old Buffer to new buffer
            //
            memcpy(NewHead, SymbolBuffer->Head, SymbolBuffer->Size * sizeof(SYMBOL));

            //
            // Free Old buffer
            //
            free(SymbolBuffer->Head);

            //
            // Update Head and size of SymbolBuffer
            //
            SymbolBuffer->Size *= 2;
            SymbolBuffer->Head = NewHead;
        }
    }

    return SymbolBuffer;
}

/**
 * @brief Prints a symbol buffer
 *
 * @param SymbolBuffer
 */
void
PrintSymbolBuffer(const PSYMBOL_BUFFER SymbolBuffer)
{
    PSYMBOL Symbol;
    for (unsigned int i = 0; i < SymbolBuffer->Pointer;)
    {
        Symbol = SymbolBuffer->Head + i;

        printf("%8x:", i);
        PrintSymbol(Symbol);
        if (Symbol->Type == SYMBOL_STRING_TYPE || Symbol->Type == SYMBOL_WSTRING_TYPE)
        {
            int temp = GetSymbolHeapSize(Symbol);
            i += temp;
        }
        else
        {
            i++;
        }
    }
}

/**
 * @brief Converts register string to integer
 *
 * @param str
 * @return unsigned long long int
 */
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

/**
 * @brief Converts pseudo register string to integer
 *
 * @param str
 * @return unsigned long long int
 */
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

/**
 * @brief Converts a sematinc rule token to integer
 *
 * @param str
 * @return unsigned long long int
 */
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

/**
 * @brief Prints some information about the error
 *
 * @param Error
 * @param str
 * @return char*
 */
char *
HandleError(PSCRIPT_ENGINE_ERROR_TYPE Error, char * str)
{
    //
    // calculate position of current line
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

    //
    // allocate required memory for message, 16 for line, 100 for error information,
    // (CurrentTokenIdx - CurrentLineIdx) for space and,
    // (LineEnd - CurrentLineIdx) for input string
    //
    int    MessageSize = 16 + 100 + (CurrentTokenIdx - CurrentLineIdx) + (LineEnd - CurrentLineIdx);
    char * Message     = (char *)malloc(MessageSize);

    if (Message == NULL)
    {
        printf("err, could not allocate buffer");
        return NULL;
    }

    //
    // add line number
    //
    strcpy(Message, "Line ");
    char Line[16] = {0};
    sprintf(Line, "%d:\n", CurrentLine);
    strcat(Message, Line);

    //
    // add the line which error happened at
    //
    strncat(Message, (str + CurrentLineIdx), LineEnd - CurrentLineIdx);

    strcat(Message, "\n");

    //
    // add pointer
    //
    char Space = ' ';
    int  n     = (CurrentTokenIdx - CurrentLineIdx);
    for (int i = 0; i < n; i++)
    {
        strncat(Message, &Space, 1);
    }
    strcat(Message, "^\n");

    //
    // add error cause and details
    //
    switch (*Error)
    {
    case SCRIPT_ENGINE_ERROR_SYNTAX:
        strcat(Message, "Syntax Error: ");
        strcat(Message, "Invalid Syntax");
        return Message;

    case SCRIPT_ENGINE_ERROR_UNKNOWN_TOKEN:
        strcat(Message, "Syntax Error: ");
        strcat(Message, "Unknown Token");
        return Message;

    case SCRIPT_ENGINE_ERROR_UNRESOLVED_VARIABLE:
        strcat(Message, "Syntax Error: ");
        strcat(Message, "Unresolved Variable");
        return Message;

    case SCRIPT_ENGINE_ERROR_UNHANDLED_SEMANTIC_RULE:
        strcat(Message, "Syntax Error: ");
        strcat(Message, "Unhandled Semantic Rule");
        return Message;

    case SCRIPT_ENGINE_ERROR_TEMP_LIST_FULL:
        strcat(Message, "Internal Error: ");
        strcat(Message, "Please split the expression to many smaller expressions.");
        return Message;

    case SCRIPT_ENGINE_ERROR_UNDEFINED_FUNCTION:
        strcat(Message, "Undefined Function");
        return Message;
    case SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE:
        strcat(Message, "Undefined Variable Type");
        return Message;
    case SCRIPT_ENGINE_ERROR_VOID_FUNCTION_RETURNING_VALUE:
        strcat(Message, "Returning a value in void function");
        return Message;
    case SCRIPT_ENGINE_ERROR_NON_VOID_FUNCTION_NOT_RETURNING_VALUE:
        strcat(Message, "Not returning a value in noo-void function");
        return Message;
    default:
        strcat(Message, "Unknown Error: ");
        return Message;
    }
}

/**
 * @brief Returns the integer assigned to global variable
 *
 * @param Token
 * @return int
 */
int
GetGlobalIdentifierVal(PTOKEN Token)
{
    PTOKEN CurrentToken;
    for (uintptr_t i = 0; i < IdTable->Pointer; i++)
    {
        CurrentToken = *(IdTable->Head + i);
        if (!strcmp(Token->Value, CurrentToken->Value))
        {
            return (int)i;
        }
    }
    return -1;
}

/**
 * @brief Returns the integer assigned to local variable
 *
 * @param Token
 * @return int
 */
int
GetLocalIdentifierVal(PTOKEN Token)
{
    PTOKEN CurrentToken;
    for (uintptr_t i = 0; i < IdTable->Pointer; i++)
    {
        CurrentToken = *(IdTable->Head + i);
        if (!strcmp(Token->Value, CurrentToken->Value))
        {
            return (int)i;
        }
    }
    return -1;
}

/**
 * @brief Allocates a new global variable and returns the integer assigned to it
 *
 * @param Token
 * @return int
 */
int
NewGlobalIdentifier(PTOKEN Token)
{
    PTOKEN CopiedToken = CopyToken(Token);
    IdTable            = Push(IdTable, CopiedToken);
    return IdTable->Pointer - 1;
}

/**
 * @brief Allocates a new local variable and returns the integer assigned to it
 *
 * @param Token
 * @return int
 */
int
NewLocalIdentifier(PTOKEN Token)
{
    PTOKEN CopiedToken = CopyToken(Token);
    IdTable            = Push(IdTable, CopiedToken);
    return IdTable->Pointer - 1;
}

/**
 * @brief
 *
 * @param Token
 * @return int
 */
int
NewFunctionParameterIdentifier(PTOKEN Token)
{
    PTOKEN CopiedToken       = CopyToken(Token);
    FunctionParameterIdTable = Push(FunctionParameterIdTable, CopiedToken);
    return FunctionParameterIdTable->Pointer - 1;
}

/**
 * @brief
 *
 * @param Token
 * @return int
 */
int
GetFunctionParameterIdentifier(PTOKEN Token)
{
    PTOKEN CurrentToken;
    for (uintptr_t i = 0; i < FunctionParameterIdTable->Pointer; i++)
    {
        CurrentToken = *(FunctionParameterIdTable->Head + i);
        if (!strcmp(Token->Value, CurrentToken->Value))
        {
            return (int)i;
        }
    }
    return -1;
}

/**
 * @brief Returns the size of Right Hand Side (RHS) of a rule
 *
 * @param RuleId
 * @return int
 */
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

/**
 * @brief Returns TRUE if the Token can be the operand of an operator
 *
 * @param Token
 * @return BOOL
 */
BOOL
LalrIsOperandType(PTOKEN Token)
{
    if (Token->Type == GLOBAL_ID)
    {
        return TRUE;
    }
    else if (Token->Type == GLOBAL_UNRESOLVED_ID)
    {
        return TRUE;
    }
    if (Token->Type == LOCAL_ID)
    {
        return TRUE;
    }
    else if (Token->Type == LOCAL_UNRESOLVED_ID)
    {
        return TRUE;
    }
    else if (Token->Type == FUNCTION_PARAMETER_ID)
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
    else if (Token->Type == STRING)
    {
        return TRUE;
    }
    else if (Token->Type == WSTRING)
    {
        return TRUE;
    }
    return FALSE;
}

PSYMBOL_BUFFER
GetStackBuffer()
{
    PSYMBOL_BUFFER StackBuffer = NewSymbolBuffer();
    for (int i = 0; i < 128; i++)
    {
        PushSymbol(StackBuffer, NewSymbol());
    }
    return StackBuffer;
}
