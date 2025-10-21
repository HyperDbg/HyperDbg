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

//
// Global Variables
//
extern HWDBG_INSTANCE_INFORMATION g_HwdbgInstanceInfo;
extern BOOLEAN                    g_HwdbgInstanceInfoIsValid;
extern PVOID                      g_MessageHandler;

/**
 * @brief Show messages
 *
 * @param Fmt format string message
 */
VOID
ShowMessages(const char * Fmt, ...)
{
    va_list ArgList;
    va_list Args;

    if (g_MessageHandler == NULL)
    {
        va_start(Args, Fmt);
        vprintf(Fmt, Args);
        va_end(Args);
    }
    else
    {
        char TempMessage[COMMUNICATION_BUFFER_SIZE + TCP_END_OF_BUFFER_CHARS_COUNT] = {0};
        va_start(ArgList, Fmt);
        int sprintfresult = vsprintf_s(TempMessage, COMMUNICATION_BUFFER_SIZE + TCP_END_OF_BUFFER_CHARS_COUNT, Fmt, ArgList);
        va_end(ArgList);

        if (sprintfresult != -1)
        {
            //
            // There is another handler
            //
            ((SendMessageWithParamCallback)g_MessageHandler)(TempMessage);
        }
    }
}

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
    //
    // Set the script engine message handler
    //
    g_MessageHandler = Handler;

    //
    // Call message handler of the symbol parser
    //
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
ScriptEngineConvertFileToPdbPath(const char * LocalFilePath, char * ResultPath, size_t ResultPathSize)
{
    //
    // A wrapper for pdb to path converter
    //
    return SymConvertFileToPdbPath(LocalFilePath, ResultPath, ResultPathSize);
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
 * @return PVOID
 */
PVOID
ScriptEngineParse(char * str)
{
    PSCRIPT_ENGINE_TOKEN_LIST Stack = NewTokenList();

    PSCRIPT_ENGINE_TOKEN_LIST MatchedStack = NewTokenList();
    PSYMBOL_BUFFER            CodeBuffer   = NewSymbolBuffer();

    UserDefinedFunctionHead = malloc(sizeof(USER_DEFINED_FUNCTION_NODE));
    RtlZeroMemory(UserDefinedFunctionHead, sizeof(USER_DEFINED_FUNCTION_NODE));
    UserDefinedFunctionHead->Name                     = _strdup("main");
    UserDefinedFunctionHead->IdTable                  = (unsigned long long)NewTokenList();
    UserDefinedFunctionHead->FunctionParameterIdTable = (unsigned long long)NewTokenList();
    UserDefinedFunctionHead->TempMap                  = calloc(MAX_TEMP_COUNT, 1);
    UserDefinedFunctionHead->VariableType             = (unsigned long long)VARIABLE_TYPE_VOID;

    CurrentUserDefinedFunction = UserDefinedFunctionHead;

    SCRIPT_ENGINE_ERROR_TYPE Error        = SCRIPT_ENGINE_ERROR_FREE;
    char *                   ErrorMessage = NULL;

    static FirstCall = 1;
    if (FirstCall)
    {
        GlobalIdTable = NewTokenList();
        FirstCall     = 0;
    }

    PSCRIPT_ENGINE_TOKEN TopToken = NewUnknownToken();

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
    PSCRIPT_ENGINE_TOKEN EndToken = NewToken(END_OF_STACK, "$");

    //
    // Start Token
    //
    PSCRIPT_ENGINE_TOKEN StartToken = NewToken(NON_TERMINAL, START_VARIABLE);

    Push(Stack, EndToken);
    Push(Stack, StartToken);

    c = sgetc(str);

    PSCRIPT_ENGINE_TOKEN CurrentIn = Scan(str, &c);
    if (CurrentIn->Type == UNKNOWN)
    {
        Error               = SCRIPT_ENGINE_ERROR_SYNTAX;
        ErrorMessage        = HandleError(&Error, str);
        CodeBuffer->Message = ErrorMessage;

        RemoveTokenList(Stack);
        RemoveTokenList(MatchedStack);
        RemoveToken(&CurrentIn);
        return (PVOID)CodeBuffer;
    }

    //
    // add stack index
    //
    PSYMBOL TempSymbol = NewSymbol();
    TempSymbol->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
    TempSymbol->Value  = FUNC_ADD;
    PushSymbol(CodeBuffer, TempSymbol);
    RemoveSymbol(&TempSymbol);

    TempSymbol        = NewSymbol();
    TempSymbol->Type  = SYMBOL_NUM_TYPE;
    TempSymbol->Value = 0xffffffffffffffff;
    PushSymbol(CodeBuffer, TempSymbol);
    RemoveSymbol(&TempSymbol);

    TempSymbol        = NewSymbol();
    TempSymbol->Type  = SYMBOL_STACK_INDEX_TYPE;
    TempSymbol->Value = 0;
    PushSymbol(CodeBuffer, TempSymbol);
    RemoveSymbol(&TempSymbol);

    TempSymbol        = NewSymbol();
    TempSymbol->Type  = SYMBOL_STACK_INDEX_TYPE;
    TempSymbol->Value = 0;
    PushSymbol(CodeBuffer, TempSymbol);
    RemoveSymbol(&TempSymbol);

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

                ScriptEngineBooleanExpresssionParse(BooleanExpressionSize, CurrentIn, MatchedStack, CodeBuffer, str, &c, &Error);
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
                    PSCRIPT_ENGINE_TOKEN Token = (PSCRIPT_ENGINE_TOKEN)&Rhs[RuleId][i];

                    if (Token->Type == EPSILON)
                        break;

                    PSCRIPT_ENGINE_TOKEN DuplicatedToken = CopyToken(Token);
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
                CodeGen(MatchedStack, CodeBuffer, TopToken, &Error);
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
    }
    else
    {
        ErrorMessage = NULL;

        PSYMBOL Symbol;
        //
        // change local id to stack temp
        //
        for (UINT64 i = 0; i < CodeBuffer->Pointer; i++)
        {
            Symbol = CodeBuffer->Head + i;
            if (Symbol->Type == SYMBOL_LOCAL_ID_TYPE)
            {
                Symbol->Type = SYMBOL_TEMP_TYPE;
                Symbol->Value += UserDefinedFunctionHead->MaxTempNumber;
            }
            else if (Symbol->Type == SYMBOL_REFERENCE_LOCAL_ID_TYPE)
            {
                Symbol->Type = SYMBOL_REFERENCE_TEMP_TYPE;
                Symbol->Value += UserDefinedFunctionHead->MaxTempNumber;
            }
            else if (Symbol->Type == SYMBOL_DEREFERENCE_LOCAL_ID_TYPE)
            {
                Symbol->Type = SYMBOL_DEREFERENCE_TEMP_TYPE;
                Symbol->Value += UserDefinedFunctionHead->MaxTempNumber;
            }
            else if (Symbol->Type == SYMBOL_VARIABLE_COUNT_TYPE)
            {
                UINT64 VariableCount = Symbol->Value;
                for (UINT64 j = 0; j < VariableCount; j++)
                {
                    Symbol = CodeBuffer->Head + i + j + 1;
                    if ((Symbol->Type & 0x7fffffff) == SYMBOL_LOCAL_ID_TYPE)
                    {
                        Symbol->Type = SYMBOL_TEMP_TYPE | (Symbol->Type & 0xffffffff00000000);
                        Symbol->Value += UserDefinedFunctionHead->MaxTempNumber;
                    }
                    else if ((Symbol->Type & 0x7fffffff) == SYMBOL_REFERENCE_LOCAL_ID_TYPE)
                    {
                        Symbol->Type = SYMBOL_REFERENCE_LOCAL_ID_TYPE | (Symbol->Type & 0xffffffff00000000);
                        Symbol->Value += UserDefinedFunctionHead->MaxTempNumber;
                    }
                    else if ((Symbol->Type & 0x7fffffff) == SYMBOL_DEREFERENCE_LOCAL_ID_TYPE)
                    {
                        Symbol->Type = SYMBOL_DEREFERENCE_LOCAL_ID_TYPE | (Symbol->Type & 0xffffffff00000000);
                        Symbol->Value += UserDefinedFunctionHead->MaxTempNumber;
                    }
                }
                i += VariableCount;
            }
        }

        //
        // set memory size for stack buffer
        //
        Symbol        = CodeBuffer->Head + 1;
        Symbol->Value = CurrentUserDefinedFunction->MaxTempNumber + CurrentUserDefinedFunction->LocalVariableNumber;
    }
    CodeBuffer->Message = ErrorMessage;

    if (Stack)
        RemoveTokenList(Stack);

    if (MatchedStack)
        RemoveTokenList(MatchedStack);

    if (UserDefinedFunctionHead)
    {
        PUSER_DEFINED_FUNCTION_NODE Node = UserDefinedFunctionHead;
        while (Node)
        {
            if (Node->Name)
                free(Node->Name);

            if (Node->IdTable)
                RemoveTokenList((PSCRIPT_ENGINE_TOKEN_LIST)Node->IdTable);

            if (Node->FunctionParameterIdTable)
                RemoveTokenList((PSCRIPT_ENGINE_TOKEN_LIST)Node->FunctionParameterIdTable);

            if (Node->TempMap)
                free(Node->TempMap);

            PUSER_DEFINED_FUNCTION_NODE Temp = Node;
            Node                             = Node->NextNode;
            free(Temp);
        }
        UserDefinedFunctionHead = 0;
    }

    if (CurrentIn)
        RemoveToken(&CurrentIn);

    if (TopToken)
        RemoveToken(&TopToken);

    return (PVOID)CodeBuffer;
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
CodeGen(PSCRIPT_ENGINE_TOKEN_LIST MatchedStack, PSYMBOL_BUFFER CodeBuffer, PSCRIPT_ENGINE_TOKEN Operator, PSCRIPT_ENGINE_ERROR_TYPE Error)
{
    PSCRIPT_ENGINE_TOKEN Op0  = NULL;
    PSCRIPT_ENGINE_TOKEN Op1  = NULL;
    PSCRIPT_ENGINE_TOKEN Op2  = NULL;
    PSCRIPT_ENGINE_TOKEN Temp = NULL;

    PSYMBOL         OperatorSymbol      = NULL;
    PSYMBOL         Op0Symbol           = NULL;
    PSYMBOL         Op1Symbol           = NULL;
    PSYMBOL         Op2Symbol           = NULL;
    PSYMBOL         TempSymbol          = NULL;
    VARIABLE_TYPE * VariableType        = NULL;
    VARIABLE_TYPE * PointerVariableType = NULL;

    //
    // It is in user-defined function if CurrentFunctionSymbol is not null
    //
    OperatorSymbol = ToSymbol(Operator, Error);

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
    PrintSymbolBuffer((PVOID)CodeBuffer);
    printf(".\n.\n.\n\n");
#endif

    while (TRUE)
    {
        if (!strcmp(Operator->Value, "@START_OF_USER_DEFINED_FUNCTION"))
        {
            Op0          = Pop(MatchedStack);
            VariableType = HandleType(MatchedStack);

            if (VariableType->Kind == TY_UNKNOWN)
            {
                *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                break;
            }

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

            PUSER_DEFINED_FUNCTION_NODE Node = UserDefinedFunctionHead;
            while (Node->NextNode)
            {
                Node = Node->NextNode;
            }
            Node->NextNode = malloc(sizeof(USER_DEFINED_FUNCTION_NODE));
            RtlZeroMemory(Node->NextNode, sizeof(USER_DEFINED_FUNCTION_NODE));
            CurrentUserDefinedFunction = Node->NextNode;

            CurrentUserDefinedFunction->Name                     = _strdup(Op0->Value);
            CurrentUserDefinedFunction->Address                  = CodeBuffer->Pointer; // CurrentPointer
            CurrentUserDefinedFunction->VariableType             = (long long unsigned)VariableType;
            CurrentUserDefinedFunction->IdTable                  = (unsigned long long)NewTokenList();
            CurrentUserDefinedFunction->FunctionParameterIdTable = (unsigned long long)NewTokenList();
            CurrentUserDefinedFunction->TempMap                  = calloc(MAX_TEMP_COUNT, 1);

            //
            // push stack base index
            //
            TempSymbol        = NewSymbol();
            TempSymbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            TempSymbol->Value = FUNC_PUSH;
            PushSymbol(CodeBuffer, TempSymbol);
            RemoveSymbol(&TempSymbol);

            TempSymbol        = NewSymbol();
            TempSymbol->Type  = SYMBOL_STACK_BASE_INDEX_TYPE;
            TempSymbol->Value = 0;
            PushSymbol(CodeBuffer, TempSymbol);
            RemoveSymbol(&TempSymbol);

            //
            // move stack index to stack base index
            //
            TempSymbol        = NewSymbol();
            TempSymbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            TempSymbol->Value = FUNC_MOV;
            PushSymbol(CodeBuffer, TempSymbol);
            RemoveSymbol(&TempSymbol);

            TempSymbol        = NewSymbol();
            TempSymbol->Type  = SYMBOL_STACK_INDEX_TYPE;
            TempSymbol->Value = 0;
            PushSymbol(CodeBuffer, TempSymbol);
            RemoveSymbol(&TempSymbol);

            TempSymbol        = NewSymbol();
            TempSymbol->Type  = SYMBOL_STACK_BASE_INDEX_TYPE;
            TempSymbol->Value = 0;
            PushSymbol(CodeBuffer, TempSymbol);
            RemoveSymbol(&TempSymbol);

            //
            // add stack index
            //
            TempSymbol        = NewSymbol();
            TempSymbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            TempSymbol->Value = FUNC_ADD;
            PushSymbol(CodeBuffer, TempSymbol);
            RemoveSymbol(&TempSymbol);

            TempSymbol        = NewSymbol();
            TempSymbol->Type  = SYMBOL_NUM_TYPE;
            TempSymbol->Value = 0xffffffffffffffff;
            PushSymbol(CodeBuffer, TempSymbol);
            RemoveSymbol(&TempSymbol);

            TempSymbol        = NewSymbol();
            TempSymbol->Type  = SYMBOL_STACK_INDEX_TYPE;
            TempSymbol->Value = 0;
            PushSymbol(CodeBuffer, TempSymbol);
            RemoveSymbol(&TempSymbol);

            TempSymbol        = NewSymbol();
            TempSymbol->Type  = SYMBOL_STACK_INDEX_TYPE;
            TempSymbol->Value = 0;
            PushSymbol(CodeBuffer, TempSymbol);
            RemoveSymbol(&TempSymbol);
        }
        else if (!strcmp(Operator->Value, "@FUNCTION_PARAMETER"))
        {
            Op0          = Pop(MatchedStack);
            VariableType = HandleType(MatchedStack);

            if (VariableType->Kind == TY_UNKNOWN)
            {
                *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                break;
            }

            NewFunctionParameterIdentifier(Op0);
            CurrentUserDefinedFunction->ParameterNumber++;
        }
        else if (!strcmp(Operator->Value, "@END_OF_USER_DEFINED_FUNCTION"))
        {
            UINT64  CurrentPointer = CodeBuffer->Pointer;
            PSYMBOL Symbol         = NULL;

            if (!CurrentUserDefinedFunction)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }

            //
            // change local id to stack temp
            //
            for (UINT64 i = CurrentUserDefinedFunction->Address; i < CurrentPointer; i++)
            {
                Symbol = CodeBuffer->Head + i;
                if (Symbol->Type == SYMBOL_LOCAL_ID_TYPE)
                {
                    Symbol->Type = SYMBOL_TEMP_TYPE;
                    Symbol->Value += CurrentUserDefinedFunction->MaxTempNumber;
                }

                else if (Symbol->Type == SYMBOL_VARIABLE_COUNT_TYPE)
                {
                    UINT64 VariableCount = Symbol->Value;
                    for (UINT64 j = 0; j < VariableCount; j++)
                    {
                        Symbol = CodeBuffer->Head + i + j + 1;
                        if ((Symbol->Type & 0x7fffffff) == SYMBOL_LOCAL_ID_TYPE)
                        {
                            Symbol->Type = SYMBOL_TEMP_TYPE | (Symbol->Type & 0xffffffff00000000);
                            Symbol->Value += CurrentUserDefinedFunction->MaxTempNumber;
                        }
                    }
                    i += VariableCount;
                }
            }

            //
            // set memory size for stack buffer
            //
            Symbol        = CodeBuffer->Head + CurrentUserDefinedFunction->Address + 6;
            Symbol->Value = CurrentUserDefinedFunction->MaxTempNumber + CurrentUserDefinedFunction->LocalVariableNumber;

            //
            // modify jump address
            //
            for (UINT64 i = CurrentUserDefinedFunction->Address; i < CurrentPointer; i++)
            {
                Symbol = CodeBuffer->Head + i;
                if (Symbol->Type == SYMBOL_SEMANTIC_RULE_TYPE && Symbol->Value == FUNC_JMP && (CodeBuffer->Head + i + 1)->Value == 0xfffffffffffffff0)
                {
                    (CodeBuffer->Head + i + 1)->Value = CurrentPointer;
                    i++;
                }
            }

            //
            // move stack base index to stack index
            //
            TempSymbol        = NewSymbol();
            TempSymbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            TempSymbol->Value = FUNC_MOV;
            PushSymbol(CodeBuffer, TempSymbol);
            RemoveSymbol(&TempSymbol);

            TempSymbol        = NewSymbol();
            TempSymbol->Type  = SYMBOL_STACK_BASE_INDEX_TYPE;
            TempSymbol->Value = 0;
            PushSymbol(CodeBuffer, TempSymbol);
            RemoveSymbol(&TempSymbol);

            TempSymbol        = NewSymbol();
            TempSymbol->Type  = SYMBOL_STACK_INDEX_TYPE;
            TempSymbol->Value = 0;
            PushSymbol(CodeBuffer, TempSymbol);
            RemoveSymbol(&TempSymbol);

            //
            // pop stack base index
            //
            TempSymbol        = NewSymbol();
            TempSymbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            TempSymbol->Value = FUNC_POP;
            PushSymbol(CodeBuffer, TempSymbol);
            RemoveSymbol(&TempSymbol);

            TempSymbol        = NewSymbol();
            TempSymbol->Type  = SYMBOL_STACK_BASE_INDEX_TYPE;
            TempSymbol->Value = 0;
            PushSymbol(CodeBuffer, TempSymbol);
            RemoveSymbol(&TempSymbol);

            TempSymbol        = NewSymbol();
            TempSymbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            TempSymbol->Value = FUNC_RET;
            PushSymbol(CodeBuffer, TempSymbol);
            RemoveSymbol(&TempSymbol);

            Symbol        = CodeBuffer->Head + CurrentUserDefinedFunction->Address - 1;
            Symbol->Value = CodeBuffer->Pointer;

            CurrentUserDefinedFunction = UserDefinedFunctionHead;
        }
        else if (!strcmp(Operator->Value, "@RETURN_OF_USER_DEFINED_FUNCTION_WITHOUT_VALUE"))
        {
            if (!CurrentUserDefinedFunction)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }
            if (((VARIABLE_TYPE *)CurrentUserDefinedFunction->VariableType)->Kind != TY_VOID)
            {
                *Error = SCRIPT_ENGINE_ERROR_NON_VOID_FUNCTION_NOT_RETURNING_VALUE;
                break;
            }

            //
            // Jump to ret code
            //
            PSYMBOL Symbol = NewSymbol();
            Symbol->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
            Symbol->Value  = FUNC_JMP;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);

            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_NUM_TYPE;
            Symbol->Value = 0xfffffffffffffff0;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);
        }
        else if (!strcmp(Operator->Value, "@RETURN_OF_USER_DEFINED_FUNCTION_WITH_VALUE"))
        {
            if (!CurrentUserDefinedFunction)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }
            if (((VARIABLE_TYPE *)CurrentUserDefinedFunction->VariableType)->Kind == TY_VOID)
            {
                *Error = SCRIPT_ENGINE_ERROR_VOID_FUNCTION_RETURNING_VALUE;
                break;
            }

            //
            // Store return value
            //
            PSYMBOL Symbol = NewSymbol();
            Symbol->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
            Symbol->Value  = FUNC_MOV;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);

            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);
            PushSymbol(CodeBuffer, Op0Symbol);
            FreeTemp(Op0);

            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_RETURN_VALUE_TYPE;
            Symbol->Value = 0;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);

            //
            // Jump to ret code
            //
            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            Symbol->Value = FUNC_JMP;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);

            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_NUM_TYPE;
            Symbol->Value = 0xfffffffffffffff0;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);

            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
        }
        else if (!strcmp(Operator->Value, "@END_OF_CALLING_USER_DEFINED_FUNCTION_WITHOUT_RETURNING_VALUE") || !strcmp(Operator->Value, "@END_OF_CALLING_USER_DEFINED_FUNCTION_WITH_RETURNING_VALUE"))
        {
            PSYMBOL              Symbol        = NULL;
            PSYMBOL              TempSymbol    = NULL;
            int                  VariableNum   = 0;
            PSCRIPT_ENGINE_TOKEN FunctionToken = NULL;

            while (MatchedStack->Pointer > 0)
            {
                FunctionToken = Pop(MatchedStack);

                if (FunctionToken->Type == FUNCTION_ID)
                {
                    break;
                }
                else
                {
                    VariableNum++;
                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = FUNC_PUSH;
                    PushSymbol(CodeBuffer, Symbol);
                    RemoveSymbol(&Symbol);

                    Symbol = ToSymbol(FunctionToken, Error);
                    PushSymbol(CodeBuffer, Symbol);
                    RemoveSymbol(&Symbol);
                    RemoveToken(&FunctionToken);
                }
            }

            PUSER_DEFINED_FUNCTION_NODE Node = GetUserDefinedFunctionNode(FunctionToken);

            if (!Node)
            {
                *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_FUNCTION;
                break;
            }

            if (VariableNum != Node->ParameterNumber)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }

            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            Symbol->Value = FUNC_CALL;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);

            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_NUM_TYPE;
            Symbol->Value = Node->Address;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);

            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            Symbol->Value = FUNC_SUB;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);

            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_NUM_TYPE;
            Symbol->Value = Node->ParameterNumber;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);

            Symbol       = NewSymbol();
            Symbol->Type = SYMBOL_STACK_INDEX_TYPE;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);

            Symbol       = NewSymbol();
            Symbol->Type = SYMBOL_STACK_INDEX_TYPE;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);

            if (!strcmp(Operator->Value, "@END_OF_CALLING_USER_DEFINED_FUNCTION_WITH_RETURNING_VALUE"))
            {
                if (((VARIABLE_TYPE *)Node->VariableType)->Kind == TY_VOID)
                {
                    *Error = SCRIPT_ENGINE_ERROR_VOID_FUNCTION_RETURNING_VALUE;
                    break;
                }

                //
                // Add return variable symbol
                //
                Temp = NewTemp(Error);
                Push(MatchedStack, Temp);

                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                Symbol->Value = FUNC_MOV;
                PushSymbol(CodeBuffer, Symbol);
                RemoveSymbol(&Symbol);

                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_RETURN_VALUE_TYPE;
                Symbol->Value = 0;
                PushSymbol(CodeBuffer, Symbol);
                RemoveSymbol(&Symbol);

                TempSymbol = ToSymbol(Temp, Error);
                PushSymbol(CodeBuffer, TempSymbol);
                RemoveSymbol(&TempSymbol);

                if (*Error != SCRIPT_ENGINE_ERROR_FREE)
                {
                    break;
                }
            }

            RemoveToken(&FunctionToken);
        }
        else if (!strcmp(Operator->Value, "@MULTIPLE_ASSIGNMENT"))
        {
            int                    Op1Capacity = 8;
            int                    Op1Count    = 0;
            PSCRIPT_ENGINE_TOKEN * Op1Array    = (PSCRIPT_ENGINE_TOKEN *)malloc(sizeof(PSCRIPT_ENGINE_TOKEN) * Op1Capacity);
            PSYMBOL                Symbol      = NewSymbol();
            Symbol->Type                       = SYMBOL_SEMANTIC_RULE_TYPE;
            Symbol->Value                      = FUNC_MOV;

            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            for (int i = MatchedStack->Pointer; i > 0; i--)
            {
                Op1 = Top(MatchedStack);

                if (Op1Count >= Op1Capacity)
                {
                    Op1Capacity *= 2;
                    Op1Array = (PSCRIPT_ENGINE_TOKEN *)realloc(Op1Array, sizeof(PSCRIPT_ENGINE_TOKEN) * Op1Capacity);
                }
                Op1Array[Op1Count++] = Op1;

                if (Op1->Type == TEMP || Op1->Type == HEX || Op1->Type == OCTAL || Op1->Type == BINARY || Op1->Type == PSEUDO_REGISTER)
                {
                    *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                    Pop(MatchedStack);
                    break;
                }
                else if (Op1->Type == GLOBAL_UNRESOLVED_ID)
                {
                    PushSymbol(CodeBuffer, Symbol);
                    PushSymbol(CodeBuffer, Op0Symbol);

                    Op1Symbol = NewSymbol();
                    free((void *)Op1Symbol->Value);
                    Op1Symbol->Value = NewGlobalIdentifier(Op1);
                    SetType(&Op1Symbol->Type, SYMBOL_GLOBAL_ID_TYPE);
                    Pop(MatchedStack);
                    PushSymbol(CodeBuffer, Op1Symbol);
                }
                else if (Op1->Type == LOCAL_UNRESOLVED_ID)
                {
                    PushSymbol(CodeBuffer, Symbol);
                    PushSymbol(CodeBuffer, Op0Symbol);

                    Op1Symbol = NewSymbol();
                    free((void *)Op1Symbol->Value);
                    Op1Symbol->Value = NewLocalIdentifier(Op1, 8);
                    SetType(&Op1Symbol->Type, SYMBOL_LOCAL_ID_TYPE);
                    Pop(MatchedStack);
                    PushSymbol(CodeBuffer, Op1Symbol);
                    RemoveSymbol(&Op1Symbol);
                }
                else if (Op1->Type == LOCAL_ID || Op1->Type == GLOBAL_ID || Op1->Type == FUNCTION_PARAMETER_ID || Op1->Type == REGISTER)
                {
                    PushSymbol(CodeBuffer, Symbol);
                    PushSymbol(CodeBuffer, Op0Symbol);

                    Op1Symbol = ToSymbol(Op1, Error);
                    PushSymbol(CodeBuffer, Op1Symbol);
                    Pop(MatchedStack);
                    RemoveSymbol(&Op1Symbol);
                }
                else
                {
                    break;
                }
            }

            RemoveSymbol(&Symbol);
            Op1 = 0;

            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                FreeTemp(Op0);
                break;
            }

            if (MatchedStack->Pointer > 0)
            {
                if (Top(MatchedStack)->Type == SCRIPT_VARIABLE_TYPE)
                {
                    VariableType = HandleType(MatchedStack);

                    if (VariableType->Kind == TY_UNKNOWN)
                    {
                        *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                        break;
                    }

                    for (int i = 0; i < Op1Count; i++)
                    {
                        Op1 = Op1Array[i];
                        if (Op1->Type == LOCAL_UNRESOLVED_ID || Op1->Type == LOCAL_ID)
                        {
                            SetLocalIdentifierVariableType(Op1, (unsigned long long)VariableType);
                        }
                        else if (Op1->Type == GLOBAL_UNRESOLVED_ID || Op1->Type == GLOBAL_ID)
                        {
                            SetGlobalIdentifierVariableType(Op1, (unsigned long long)VariableType);
                        }
                    }

                    Op1 = 0;
                }
            }

            //
            // Free the operand if it is a temp value
            //
            FreeTemp(Op0);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }

            free(Op1Array);
        }
        else if (!strcmp(Operator->Value, "@ARRAY_INDEX_READ") || !strcmp(Operator->Value, "@ARRAY_INDEX_WRITE"))
        {
            int                    ArrayCapacity = 8;
            int                    TokenCount    = 0;
            PSCRIPT_ENGINE_TOKEN * TokenArray    = (PSCRIPT_ENGINE_TOKEN *)malloc(sizeof(PSCRIPT_ENGINE_TOKEN) * ArrayCapacity);
            PSCRIPT_ENGINE_TOKEN   IdToken;
            PSYMBOL                IdSymbol;
            int                    ElementOffset = 0;
            PSYMBOL                Symbol;
            PSCRIPT_ENGINE_TOKEN   OffsetToken;
            PSYMBOL                OffsetSymbol;

            while (MatchedStack->Pointer)
            {
                if (TokenCount >= ArrayCapacity)
                {
                    TokenCount *= 2;
                    TokenArray = (PSCRIPT_ENGINE_TOKEN *)realloc(TokenArray, sizeof(PSCRIPT_ENGINE_TOKEN) * ArrayCapacity);
                }

                Temp = Top(MatchedStack);
                if (!strcmp(Top(MatchedStack)->Value, "@ARRAY_DIM_NUMBER"))
                {
                    Pop(MatchedStack);
                    TokenArray[TokenCount] = Pop(MatchedStack);
                    TokenCount++;
                }
                else
                {
                    break;
                }
            }

            IdToken  = Pop(MatchedStack);
            IdSymbol = ToSymbol(IdToken, Error);

            if (IdToken->Type == LOCAL_UNRESOLVED_ID)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }

            for (int i = 0; i < TokenCount / 2; i++)
            {
                PSCRIPT_ENGINE_TOKEN tmp       = TokenArray[i];
                TokenArray[i]                  = TokenArray[TokenCount - i - 1];
                TokenArray[TokenCount - i - 1] = tmp;
            }

            VariableType = (VARIABLE_TYPE *)IdToken->VariableType;
            Temp         = NewTemp();
            TempSymbol   = ToSymbol(Temp, Error);
            OffsetToken  = NewTemp();
            OffsetSymbol = ToSymbol(OffsetToken, Error);

            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            Symbol->Value = FUNC_MOV;
            PushSymbol(CodeBuffer, Symbol);

            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_NUM_TYPE;
            Symbol->Value = 0;
            PushSymbol(CodeBuffer, Symbol);

            PushSymbol(CodeBuffer, OffsetSymbol);

            for (int i = 0; i < TokenCount; i++)
            {
                if (!VariableType->Base)
                {
                    *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                    break;
                }

                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                Symbol->Value = FUNC_MUL;
                PushSymbol(CodeBuffer, Symbol);

                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_NUM_TYPE;
                Symbol->Value = VariableType->Base->Size;
                PushSymbol(CodeBuffer, Symbol);

                Symbol = ToSymbol(TokenArray[i], Error);
                PushSymbol(CodeBuffer, Symbol);

                PushSymbol(CodeBuffer, TempSymbol);

                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                Symbol->Value = FUNC_ADD;
                PushSymbol(CodeBuffer, Symbol);

                PushSymbol(CodeBuffer, TempSymbol);
                PushSymbol(CodeBuffer, OffsetSymbol);
                PushSymbol(CodeBuffer, OffsetSymbol);

                VariableType = VariableType->Base;
            }

            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }

            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            Symbol->Value = FUNC_ADD;
            PushSymbol(CodeBuffer, Symbol);

            PushSymbol(CodeBuffer, IdSymbol);
            PushSymbol(CodeBuffer, OffsetSymbol);
            PushSymbol(CodeBuffer, OffsetSymbol);

            if (!strcmp(Operator->Value, "@ARRAY_INDEX_READ"))
            {
                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                Symbol->Value = FUNC_POI;
                PushSymbol(CodeBuffer, Symbol);

                PushSymbol(CodeBuffer, OffsetSymbol);
                PushSymbol(CodeBuffer, OffsetSymbol);
            }
            else if (!strcmp(Operator->Value, "@ARRAY_INDEX_WRITE"))
            {
                OffsetToken->Type = DEFERENCE_TEMP;
            }

            Push(MatchedStack, OffsetToken);

            FreeTemp(Temp);
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
                Op1Symbol->Value = NewLocalIdentifier(Op1, 8);
                SetType(&Op1Symbol->Type, SYMBOL_LOCAL_ID_TYPE);
            }
            else
            {
                Op1Symbol = ToSymbol(Op1, Error);
            }

            if (MatchedStack->Pointer > 0)
            {
                if (!strcmp(Top(MatchedStack)->Value, "@DECLARE_POINTER_TYPE") || Top(MatchedStack)->Type == SCRIPT_VARIABLE_TYPE)
                {
                    if (!strcmp(Top(MatchedStack)->Value, "@DECLARE_POINTER_TYPE"))
                    {
                        PointerVariableType             = calloc(1, sizeof(VARIABLE_TYPE));
                        PointerVariableType->Kind       = TY_PTR;
                        PointerVariableType->Size       = 8;
                        PointerVariableType->Align      = 8;
                        PointerVariableType->IsUnsigned = TRUE;
                        Pop(MatchedStack);
                    }

                    VariableType = HandleType(MatchedStack);

                    if (VariableType->Kind == TY_UNKNOWN)
                    {
                        *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                        break;
                    }

                    if (PointerVariableType)
                    {
                        PointerVariableType->Base = VariableType;
                        VariableType              = PointerVariableType;
                    }

                    if (Op1->Type == LOCAL_UNRESOLVED_ID || Op1->Type == LOCAL_ID)
                    {
                        SetLocalIdentifierVariableType(Op1, (unsigned long long)VariableType);
                    }
                    else if (Op1->Type == GLOBAL_UNRESOLVED_ID || Op1->Type == GLOBAL_ID)
                    {
                        SetGlobalIdentifierVariableType(Op1, (unsigned long long)VariableType);
                    }
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
        else if (!strcmp(Operator->Value, "@DECLARE_POINTER_TYPE"))
        {
            Push(MatchedStack, CopyToken(Operator));
        }
        else if (!strcmp(Operator->Value, "@ARRAY_DIM_NUMBER") || !strcmp(Operator->Value, "@ARRAY_LEFT_BRACKET") || !strcmp(Operator->Value, "@ARRAY_L_VALUE"))
        {
            Push(MatchedStack, CopyToken(Operator));
        }
        else if (!strcmp(Operator->Value, "@DEREFERENCE"))
        {
            PSYMBOL Symbol;

            Op0 = Pop(MatchedStack);
            Op1 = Pop(MatchedStack);

            if (Op1->Type == LOCAL_UNRESOLVED_ID)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }

            if (((VARIABLE_TYPE *)Op1->VariableType)->Size == 4)
            {
                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                Symbol->Value = FUNC_ED;

                Op0Symbol = ToSymbol(Op0, Error);
                Op1Symbol = ToSymbol(Op1, Error);

                Temp       = NewTemp(Error);
                TempSymbol = ToSymbol(Temp, Error);

                PushSymbol(CodeBuffer, Symbol);
                PushSymbol(CodeBuffer, Op0Symbol);
                PushSymbol(CodeBuffer, Op1Symbol);
                PushSymbol(CodeBuffer, TempSymbol);
                FreeTemp(Temp);
            }
            else if (((VARIABLE_TYPE *)Op1->VariableType)->Size == 8)
            {
                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                Symbol->Value = FUNC_EQ;

                Op0Symbol = ToSymbol(Op0, Error);
                Op1Symbol = ToSymbol(Op1, Error);

                Temp       = NewTemp(Error);
                TempSymbol = ToSymbol(Temp, Error);

                PushSymbol(CodeBuffer, Symbol);
                PushSymbol(CodeBuffer, Op0Symbol);
                PushSymbol(CodeBuffer, Op1Symbol);
                PushSymbol(CodeBuffer, TempSymbol);
                FreeTemp(Temp);
            }

            else if (((VARIABLE_TYPE *)Op1->VariableType)->Size == 1)
            {
                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                Symbol->Value = FUNC_EB;

                Op0Symbol = ToSymbol(Op0, Error);
                Op1Symbol = ToSymbol(Op1, Error);

                Temp       = NewTemp(Error);
                TempSymbol = ToSymbol(Temp, Error);

                PushSymbol(CodeBuffer, Symbol);
                PushSymbol(CodeBuffer, Op0Symbol);
                PushSymbol(CodeBuffer, Op1Symbol);
                PushSymbol(CodeBuffer, TempSymbol);
                FreeTemp(Temp);
            }
        }
        else if (!strcmp(Operator->Value, "@ARRAY_DECLARITION"))
        {
            int                    TokenCapacity = 8;
            int                    TokenCount    = 0;
            PSCRIPT_ENGINE_TOKEN * TokenArray    = (PSCRIPT_ENGINE_TOKEN *)malloc(sizeof(PSCRIPT_ENGINE_TOKEN) * TokenCapacity);
            PSCRIPT_ENGINE_TOKEN   IdToken       = NULL;
            PSYMBOL                IdSymbol      = NewSymbol();
            ;
            VARIABLE_TYPE * VariableType2             = NULL;
            int             Last_ARRAY_DIM_NUMBER_Idx = 0;
            int             ArrayElementCount         = 0;
            PSYMBOL         Symbol                    = NULL;
            int             BaseTypeSize              = 0;

            for (int i = MatchedStack->Pointer; i > 0; i--)
            {
                Temp = Pop(MatchedStack);
                if (!strcmp(Temp->Value, "@ARRAY_L_VALUE"))
                {
                    break;
                }

                if (TokenCount >= TokenCapacity)
                {
                    TokenCapacity *= 2;
                    TokenArray = (PSCRIPT_ENGINE_TOKEN *)realloc(TokenArray, sizeof(PSCRIPT_ENGINE_TOKEN) * TokenCapacity);
                }
                TokenArray[TokenCount++] = Temp;
            }

            for (size_t i = 0; i < TokenCount / 2; i++)
            {
                PSCRIPT_ENGINE_TOKEN tmp       = TokenArray[i];
                TokenArray[i]                  = TokenArray[TokenCount - i - 1];
                TokenArray[TokenCount - i - 1] = tmp;
            }

            IdToken = Pop(MatchedStack);

            if (MatchedStack->Pointer > 0)
            {
                if (Top(MatchedStack)->Type == SCRIPT_VARIABLE_TYPE)
                {
                    VariableType = HandleType(MatchedStack);

                    if (VariableType->Kind == TY_UNKNOWN)
                    {
                        *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                        break;
                    }
                }
            }

            if (!VariableType)
            {
                VariableType = VARIABLE_TYPE_LONG;
            }

            BaseTypeSize = VariableType->Size;

            for (int i = TokenCount - 1; i >= 0; i--)
            {
                if (!strcmp(TokenArray[i]->Value, "@ARRAY_DIM_NUMBER"))
                {
                    Last_ARRAY_DIM_NUMBER_Idx = i;
                    break;
                }
            }

            for (int i = Last_ARRAY_DIM_NUMBER_Idx; i >= 0; i--)
            {
                if (!strcmp(TokenArray[i]->Value, "@ARRAY_DIM_NUMBER"))
                {
                    VariableType2           = calloc(1, sizeof(VARIABLE_TYPE));
                    VariableType2->Kind     = TY_ARRAY;
                    VariableType2->Size     = VariableType->Size * atoi(TokenArray[i - 1]->Value);
                    VariableType2->Align    = VariableType->Align;
                    VariableType2->Base     = VariableType;
                    VariableType2->ArrayLen = atoi(TokenArray[i - 1]->Value);
                    VariableType            = VariableType2;
                    i--;
                }
            }

            if (IdToken->Type == LOCAL_UNRESOLVED_ID)
            {
                IdSymbol->Value = NewLocalIdentifier(IdToken, VariableType->Size);
                SetType(&IdSymbol->Type, SYMBOL_REFERENCE_LOCAL_ID_TYPE);
                SetLocalIdentifierVariableType(IdToken, (unsigned long long)VariableType);
            }

            for (int i = Last_ARRAY_DIM_NUMBER_Idx + 1; i < TokenCount; i++)
            {
                if (TokenArray[i]->Type != SEMANTIC_RULE)
                {
                    if ((ArrayElementCount * BaseTypeSize) > VariableType->Size)
                    {
                        *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                        break;
                    }

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = FUNC_MUL;
                    PushSymbol(CodeBuffer, Symbol);

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_NUM_TYPE;
                    Symbol->Value = BaseTypeSize;
                    PushSymbol(CodeBuffer, Symbol);

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_NUM_TYPE;
                    Symbol->Value = ArrayElementCount;
                    PushSymbol(CodeBuffer, Symbol);

                    Temp       = NewTemp();
                    TempSymbol = ToSymbol(Temp, Error);
                    PushSymbol(CodeBuffer, TempSymbol);

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = FUNC_ADD;

                    PushSymbol(CodeBuffer, Symbol);
                    PushSymbol(CodeBuffer, IdSymbol);
                    PushSymbol(CodeBuffer, TempSymbol);
                    PushSymbol(CodeBuffer, TempSymbol);

                    Symbol       = NewSymbol();
                    Symbol->Type = SYMBOL_SEMANTIC_RULE_TYPE;

                    if (BaseTypeSize == 4)
                    {
                        Symbol->Value = FUNC_ED;
                    }
                    else if (BaseTypeSize == 8)
                    {
                        Symbol->Value = FUNC_EQ;
                    }
                    else if (BaseTypeSize == 1)
                    {
                        Symbol->Value = FUNC_EB;
                    }

                    PushSymbol(CodeBuffer, Symbol);

                    Symbol = ToSymbol(TokenArray[i], Error);
                    PushSymbol(CodeBuffer, Symbol);

                    PushSymbol(CodeBuffer, TempSymbol);

                    PushSymbol(CodeBuffer, TempSymbol);

                    FreeTemp(Temp);

                    ArrayElementCount++;
                }
            }

            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }

            while ((ArrayElementCount * BaseTypeSize) < VariableType->Size)
            {
                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                Symbol->Value = FUNC_MUL;
                PushSymbol(CodeBuffer, Symbol);

                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_NUM_TYPE;
                Symbol->Value = BaseTypeSize;
                PushSymbol(CodeBuffer, Symbol);

                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_NUM_TYPE;
                Symbol->Value = ArrayElementCount;
                PushSymbol(CodeBuffer, Symbol);

                Temp       = NewTemp();
                TempSymbol = ToSymbol(Temp, Error);
                PushSymbol(CodeBuffer, TempSymbol);

                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                Symbol->Value = FUNC_ADD;

                PushSymbol(CodeBuffer, Symbol);
                PushSymbol(CodeBuffer, IdSymbol);
                PushSymbol(CodeBuffer, TempSymbol);
                PushSymbol(CodeBuffer, TempSymbol);

                Symbol       = NewSymbol();
                Symbol->Type = SYMBOL_SEMANTIC_RULE_TYPE;

                if (BaseTypeSize == 4)
                {
                    Symbol->Value = FUNC_ED;
                }
                else if (BaseTypeSize == 8)
                {
                    Symbol->Value = FUNC_EQ;
                }
                else if (BaseTypeSize == 1)
                {
                    Symbol->Value = FUNC_EB;
                }

                PushSymbol(CodeBuffer, Symbol);

                Symbol = Symbol = NewSymbol();
                Symbol->Type    = SYMBOL_NUM_TYPE;
                Symbol->Value   = 0;
                PushSymbol(CodeBuffer, Symbol);

                PushSymbol(CodeBuffer, TempSymbol);

                PushSymbol(CodeBuffer, TempSymbol);

                FreeTemp(Temp);

                ArrayElementCount++;
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
            Op0          = Pop(MatchedStack);
            VariableType = (VARIABLE_TYPE *)Op0->VariableType;
            Op0Symbol    = ToSymbol(Op0, Error);

            Temp = NewTemp(Error);
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
                        RemoveSymbolBuffer((PVOID)TempStack);
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
                RemoveSymbolBuffer((PVOID)TempStack);
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
            RemoveSymbolBuffer((PVOID)TempStack);

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
        else if (IsType16Func(Operator))
        {
            PushSymbol(CodeBuffer, OperatorSymbol);

            Temp = NewTemp(Error);
            Push(MatchedStack, Temp);
            TempSymbol = ToSymbol(Temp, Error);

            PushSymbol(CodeBuffer, TempSymbol);

            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
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

            Temp = NewTemp(Error);
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

            Temp = NewTemp(Error);
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
        else if (IsAssignmentOperator(Operator))
        {
            BOOL Handled = FALSE;
            Op1          = TopIndexed(MatchedStack, 1);

            if (((VARIABLE_TYPE *)Op1->VariableType)->Kind == TY_PTR)
            {
                if (!strcmp(Operator->Value, "@ADD_ASSIGNMENT"))
                {
                    PSYMBOL Symbol = NULL;

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = FUNC_MUL;
                    PushSymbol(CodeBuffer, Symbol);

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_NUM_TYPE;
                    Symbol->Value = ((VARIABLE_TYPE *)Op1->VariableType)->Base->Size;
                    PushSymbol(CodeBuffer, Symbol);

                    Op0       = Pop(MatchedStack);
                    Op0Symbol = ToSymbol(Op0, Error);
                    PushSymbol(CodeBuffer, Op0Symbol);

                    Temp       = NewTemp(Error);
                    TempSymbol = ToSymbol(Temp, Error);
                    PushSymbol(CodeBuffer, TempSymbol);

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = FUNC_ADD;
                    PushSymbol(CodeBuffer, Symbol);

                    PushSymbol(CodeBuffer, TempSymbol);

                    Op1       = Pop(MatchedStack);
                    Op1Symbol = ToSymbol(Op1, Error);
                    PushSymbol(CodeBuffer, Op1Symbol);
                    PushSymbol(CodeBuffer, Op1Symbol);

                    FreeTemp(Op0);
                    if (*Error != SCRIPT_ENGINE_ERROR_FREE)
                    {
                        break;
                    }
                }
                else if (!strcmp(Operator->Value, "@SUB_ASSIGNMENT"))
                {
                    PSYMBOL Symbol = NULL;

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = FUNC_MUL;
                    PushSymbol(CodeBuffer, Symbol);

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_NUM_TYPE;
                    Symbol->Value = ((VARIABLE_TYPE *)Op1->VariableType)->Base->Size;
                    PushSymbol(CodeBuffer, Symbol);

                    Op0       = Pop(MatchedStack);
                    Op0Symbol = ToSymbol(Op0, Error);
                    PushSymbol(CodeBuffer, Op0Symbol);

                    Temp       = NewTemp(Error);
                    TempSymbol = ToSymbol(Temp, Error);
                    PushSymbol(CodeBuffer, TempSymbol);

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = FUNC_SUB;
                    PushSymbol(CodeBuffer, Symbol);

                    PushSymbol(CodeBuffer, TempSymbol);

                    Op1       = Pop(MatchedStack);
                    Op1Symbol = ToSymbol(Op1, Error);
                    PushSymbol(CodeBuffer, Op1Symbol);
                    PushSymbol(CodeBuffer, Op1Symbol);

                    FreeTemp(Op0);
                    if (*Error != SCRIPT_ENGINE_ERROR_FREE)
                    {
                        break;
                    }
                }
                else
                {
                    *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                    Op0    = Pop(MatchedStack);
                    Op1    = Pop(MatchedStack);
                }
                Handled = TRUE;
            }

            if (!Handled)
            {
                PushSymbol(CodeBuffer, OperatorSymbol);
                Op0       = Pop(MatchedStack);
                Op0Symbol = ToSymbol(Op0, Error);

                Op1       = Pop(MatchedStack);
                Op1Symbol = ToSymbol(Op1, Error);

                PushSymbol(CodeBuffer, Op0Symbol);
                PushSymbol(CodeBuffer, Op1Symbol);
                PushSymbol(CodeBuffer, Op1Symbol);

                //
                // Free the operand if it is a temp value
                //
                FreeTemp(Op0);
                if (*Error != SCRIPT_ENGINE_ERROR_FREE)
                {
                    break;
                }
            }
        }
        else if (IsTwoOperandOperator(Operator))
        {
            BOOL Handled = FALSE;
            Op0          = TopIndexed(MatchedStack, 0);
            Op1          = TopIndexed(MatchedStack, 1);

            if (!strcmp(Operator->Value, "@ADD") || !strcmp(Operator->Value, "@SUB"))
            {
                if (((VARIABLE_TYPE *)Op0->VariableType)->Kind == TY_PTR && ((VARIABLE_TYPE *)Op1->VariableType)->Kind == TY_PTR)
                {
                    *Error  = SCRIPT_ENGINE_ERROR_SYNTAX;
                    Op0     = Pop(MatchedStack);
                    Op1     = Pop(MatchedStack);
                    Handled = TRUE;
                }
                else if (((VARIABLE_TYPE *)Op0->VariableType)->Kind == TY_PTR && ((VARIABLE_TYPE *)Op1->VariableType)->Kind != TY_PTR)
                {
                    if (!strcmp(Operator->Value, "@SUB"))
                    {
                        *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                        Op0    = Pop(MatchedStack);
                        Op1    = Pop(MatchedStack);
                    }
                    else
                    {
                        PSYMBOL Symbol = NULL;

                        Op0       = Pop(MatchedStack);
                        Op1       = Pop(MatchedStack);
                        Op0Symbol = ToSymbol(Op0, Error);
                        Op1Symbol = ToSymbol(Op1, Error);

                        Symbol        = NewSymbol();
                        Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                        Symbol->Value = FUNC_MUL;
                        PushSymbol(CodeBuffer, Symbol);

                        Symbol        = NewSymbol();
                        Symbol->Type  = SYMBOL_NUM_TYPE;
                        Symbol->Value = ((VARIABLE_TYPE *)Op0->VariableType)->Base->Size;
                        PushSymbol(CodeBuffer, Symbol);

                        Op1Symbol = ToSymbol(Op1, Error);
                        PushSymbol(CodeBuffer, Op1Symbol);

                        Temp       = NewTemp(Error);
                        TempSymbol = ToSymbol(Temp, Error);
                        PushSymbol(CodeBuffer, TempSymbol);

                        PushSymbol(CodeBuffer, OperatorSymbol);
                        Op0Symbol = ToSymbol(Op0, Error);
                        PushSymbol(CodeBuffer, Op0Symbol);
                        PushSymbol(CodeBuffer, TempSymbol);
                        PushSymbol(CodeBuffer, TempSymbol);

                        Push(MatchedStack, Temp);

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
                    Handled = TRUE;
                }
                else if (((VARIABLE_TYPE *)Op0->VariableType)->Kind != TY_PTR && ((VARIABLE_TYPE *)Op1->VariableType)->Kind == TY_PTR)
                {
                    PSYMBOL Symbol = NULL;

                    Op0       = Pop(MatchedStack);
                    Op1       = Pop(MatchedStack);
                    Op0Symbol = ToSymbol(Op0, Error);
                    Op1Symbol = ToSymbol(Op1, Error);

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = FUNC_MUL;
                    PushSymbol(CodeBuffer, Symbol);

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_NUM_TYPE;
                    Symbol->Value = ((VARIABLE_TYPE *)Op1->VariableType)->Base->Size;
                    PushSymbol(CodeBuffer, Symbol);

                    PushSymbol(CodeBuffer, Op0Symbol);

                    Temp       = NewTemp(Error);
                    TempSymbol = ToSymbol(Temp, Error);
                    PushSymbol(CodeBuffer, TempSymbol);

                    PushSymbol(CodeBuffer, OperatorSymbol);
                    PushSymbol(CodeBuffer, TempSymbol);
                    PushSymbol(CodeBuffer, Op1Symbol);
                    PushSymbol(CodeBuffer, TempSymbol);

                    Push(MatchedStack, Temp);

                    //
                    // Free the operand if it is a temp value
                    //
                    FreeTemp(Op0);
                    FreeTemp(Op1);
                    if (*Error != SCRIPT_ENGINE_ERROR_FREE)
                    {
                        break;
                    }

                    Handled = TRUE;
                }

                else if (((VARIABLE_TYPE *)Op0->VariableType)->Kind == TY_ARRAY && ((VARIABLE_TYPE *)Op1->VariableType)->Kind == TY_ARRAY)
                {
                    *Error  = SCRIPT_ENGINE_ERROR_SYNTAX;
                    Op0     = Pop(MatchedStack);
                    Op1     = Pop(MatchedStack);
                    Handled = TRUE;
                }
                else if (((VARIABLE_TYPE *)Op0->VariableType)->Kind == TY_ARRAY && ((VARIABLE_TYPE *)Op1->VariableType)->Kind != TY_ARRAY)
                {
                    if (!strcmp(Operator->Value, "@SUB"))
                    {
                        *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                        Op0    = Pop(MatchedStack);
                        Op1    = Pop(MatchedStack);
                    }
                    else
                    {
                        PSYMBOL Symbol           = NULL;
                        int     VariableBaseSize = 0;
                        VariableType             = (VARIABLE_TYPE *)Op0->VariableType;
                        while (VariableType->Base)
                        {
                            VariableType = VariableType->Base;
                        }
                        VariableBaseSize = VariableType->Size;

                        Op0       = Pop(MatchedStack);
                        Op1       = Pop(MatchedStack);
                        Op0Symbol = ToSymbol(Op0, Error);
                        Op1Symbol = ToSymbol(Op1, Error);

                        Symbol        = NewSymbol();
                        Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                        Symbol->Value = FUNC_MUL;
                        PushSymbol(CodeBuffer, Symbol);

                        Symbol        = NewSymbol();
                        Symbol->Type  = SYMBOL_NUM_TYPE;
                        Symbol->Value = VariableBaseSize;
                        PushSymbol(CodeBuffer, Symbol);

                        Op1Symbol = ToSymbol(Op1, Error);
                        PushSymbol(CodeBuffer, Op1Symbol);

                        Temp       = NewTemp(Error);
                        TempSymbol = ToSymbol(Temp, Error);
                        PushSymbol(CodeBuffer, TempSymbol);

                        PushSymbol(CodeBuffer, OperatorSymbol);
                        Op0Symbol = ToSymbol(Op0, Error);
                        PushSymbol(CodeBuffer, Op0Symbol);
                        PushSymbol(CodeBuffer, TempSymbol);
                        PushSymbol(CodeBuffer, TempSymbol);

                        Push(MatchedStack, Temp);

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
                    Handled = TRUE;
                }
                else if (((VARIABLE_TYPE *)Op0->VariableType)->Kind != TY_ARRAY && ((VARIABLE_TYPE *)Op1->VariableType)->Kind == TY_ARRAY)
                {
                    PSYMBOL Symbol           = NULL;
                    int     VariableBaseSize = 0;
                    VariableType             = (VARIABLE_TYPE *)Op1->VariableType;
                    while (VariableType->Base)
                    {
                        VariableType = VariableType->Base;
                    }
                    VariableBaseSize = VariableType->Size;

                    Op0       = Pop(MatchedStack);
                    Op1       = Pop(MatchedStack);
                    Op0Symbol = ToSymbol(Op0, Error);
                    Op1Symbol = ToSymbol(Op1, Error);

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = FUNC_MUL;
                    PushSymbol(CodeBuffer, Symbol);

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_NUM_TYPE;
                    Symbol->Value = VariableBaseSize;
                    PushSymbol(CodeBuffer, Symbol);

                    PushSymbol(CodeBuffer, Op0Symbol);

                    Temp       = NewTemp(Error);
                    TempSymbol = ToSymbol(Temp, Error);
                    PushSymbol(CodeBuffer, TempSymbol);

                    PushSymbol(CodeBuffer, OperatorSymbol);
                    PushSymbol(CodeBuffer, TempSymbol);
                    PushSymbol(CodeBuffer, Op1Symbol);
                    PushSymbol(CodeBuffer, TempSymbol);

                    Push(MatchedStack, Temp);

                    //
                    // Free the operand if it is a temp value
                    //
                    FreeTemp(Op0);
                    FreeTemp(Op1);
                    if (*Error != SCRIPT_ENGINE_ERROR_FREE)
                    {
                        break;
                    }

                    Handled = TRUE;
                }
            }

            if (!Handled)
            {
                PushSymbol(CodeBuffer, OperatorSymbol);
                Op0       = Pop(MatchedStack);
                Op0Symbol = ToSymbol(Op0, Error);

                Op1       = Pop(MatchedStack);
                Op1Symbol = ToSymbol(Op1, Error);

                Temp               = NewTemp(Error);
                Temp->VariableType = (unsigned long long)GetCommonVariableType((VARIABLE_TYPE *)Op0->VariableType, (VARIABLE_TYPE *)Op1->VariableType);
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
        }
        else if (IsOneOperandOperator(Operator))
        {
            BOOL Handled = FALSE;
            Op0          = Top(MatchedStack);

            if (((VARIABLE_TYPE *)Op0->VariableType)->Kind == TY_PTR)
            {
                if (!strcmp(Operator->Value, "@INC"))
                {
                    PSYMBOL Symbol = NULL;

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = FUNC_ADD;
                    PushSymbol(CodeBuffer, Symbol);

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_NUM_TYPE;
                    Symbol->Value = ((VARIABLE_TYPE *)Op0->VariableType)->Base->Size;
                    PushSymbol(CodeBuffer, Symbol);

                    Op0       = Pop(MatchedStack);
                    Op0Symbol = ToSymbol(Op0, Error);
                    PushSymbol(CodeBuffer, Op0Symbol);
                    PushSymbol(CodeBuffer, Op0Symbol);

                    FreeTemp(Op0);
                    if (*Error != SCRIPT_ENGINE_ERROR_FREE)
                    {
                        break;
                    }
                    Handled = TRUE;
                }

                else if (!strcmp(Operator->Value, "@DEC"))
                {
                    PSYMBOL Symbol = NULL;

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = FUNC_SUB;
                    PushSymbol(CodeBuffer, Symbol);

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_NUM_TYPE;
                    Symbol->Value = ((VARIABLE_TYPE *)Op0->VariableType)->Base->Size;
                    PushSymbol(CodeBuffer, Symbol);

                    Op0       = Pop(MatchedStack);
                    Op0Symbol = ToSymbol(Op0, Error);
                    PushSymbol(CodeBuffer, Op0Symbol);
                    PushSymbol(CodeBuffer, Op0Symbol);

                    FreeTemp(Op0);
                    if (*Error != SCRIPT_ENGINE_ERROR_FREE)
                    {
                        break;
                    }
                    Handled = TRUE;
                }
            }

            if (!Handled)
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
        }
        else if (!strcmp(Operator->Value, "@VARGSTART"))
        {
            PSCRIPT_ENGINE_TOKEN OperatorCopy = CopyToken(Operator);
            Push(MatchedStack, OperatorCopy);
        }
        else if (!strcmp(Operator->Value, "@START_OF_IF"))
        {
            PSCRIPT_ENGINE_TOKEN OperatorCopy = CopyToken(Operator);
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
            PSCRIPT_ENGINE_TOKEN CurrentAddressToken = NewToken(DECIMAL, str);
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
            UINT64               CurrentPointer           = CodeBuffer->Pointer;
            PSCRIPT_ENGINE_TOKEN JumpSemanticAddressToken = Pop(MatchedStack);
            UINT64               JumpSemanticAddress      = DecimalToInt(JumpSemanticAddressToken->Value);
            PSYMBOL              JumpAddressSymbol        = (PSYMBOL)(CodeBuffer->Head + JumpSemanticAddress - 2);
            JumpAddressSymbol->Value                      = CurrentPointer + 2;
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
            PSCRIPT_ENGINE_TOKEN CurrentAddressToken = NewToken(DECIMAL, str);
            Push(MatchedStack, CurrentAddressToken);
        }
        else if (!strcmp(Operator->Value, "@END_OF_IF"))
        {
            UINT64               CurrentPointer           = CodeBuffer->Pointer;
            PSCRIPT_ENGINE_TOKEN JumpSemanticAddressToken = Pop(MatchedStack);
            PSYMBOL              JumpAddressSymbol;
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
            PSCRIPT_ENGINE_TOKEN OperatorCopy = CopyToken(Operator);
            Push(MatchedStack, OperatorCopy);

            char str[20] = {0};
            sprintf(str, "%llu", (UINT64)CodeBuffer->Pointer);
            PSCRIPT_ENGINE_TOKEN CurrentAddressToken = NewToken(DECIMAL, str);
            Push(MatchedStack, CurrentAddressToken);
        }
        else if (!strcmp(Operator->Value, "@START_OF_WHILE_COMMANDS"))
        {
            UINT64               CurrentPointer = CodeBuffer->Pointer;
            PSCRIPT_ENGINE_TOKEN JzToken        = NewToken(SEMANTIC_RULE, "@JZ");

            RemoveSymbol(&OperatorSymbol);
            OperatorSymbol = ToSymbol(JzToken, Error);
            RemoveToken(&JzToken);

            PSYMBOL JumpAddressSymbol = NewSymbol();
            JumpAddressSymbol->Type   = SYMBOL_NUM_TYPE;
            JumpAddressSymbol->Value  = 0xffffffffffffffff;

            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            PSCRIPT_ENGINE_TOKEN StartOfWhileToken = Pop(MatchedStack);

            char str[20];
            sprintf(str, "%llu", CurrentPointer + 1);
            PSCRIPT_ENGINE_TOKEN CurrentAddressToken = NewToken(DECIMAL, str);
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
            PSCRIPT_ENGINE_TOKEN JumpAddressToken  = Pop(MatchedStack);
            UINT64               JumpAddress       = DecimalToInt(JumpAddressToken->Value);
            PSYMBOL              JumpAddressSymbol = ToSymbol(JumpAddressToken, Error);

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
            PSCRIPT_ENGINE_TOKEN OperatorCopy = CopyToken(Operator);
            Push(MatchedStack, OperatorCopy);

            char str[20];
            sprintf(str, "%llu", (UINT64)CodeBuffer->Pointer);
            PSCRIPT_ENGINE_TOKEN CurrentAddressToken = NewToken(DECIMAL, str);
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
            PSCRIPT_ENGINE_TOKEN JumpAddressToken = Pop(MatchedStack);
            UINT64               JumpAddress      = DecimalToInt(JumpAddressToken->Value);

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
                printf("Jz Jump Address = %lld\n", JumpAddress);
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
            PSCRIPT_ENGINE_TOKEN OperatorCopy = CopyToken(Operator);
            Push(MatchedStack, OperatorCopy);

            //
            // Push current pointer into matched stack
            //
            char str[20] = {0};
            sprintf(str, "%llu", (UINT64)CodeBuffer->Pointer);
            PSCRIPT_ENGINE_TOKEN CurrentAddressToken = NewToken(DECIMAL, str);
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
            PSCRIPT_ENGINE_TOKEN StartOfForAddressToken = Pop(MatchedStack);

            //
            // Push current pointer into matched stack
            //
            char str[20] = {0};
            sprintf(str, "%llu", (UINT64)CodeBuffer->Pointer);
            PSCRIPT_ENGINE_TOKEN CurrentAddressToken = NewToken(DECIMAL, str);
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
            PSCRIPT_ENGINE_TOKEN JumpAddressToken = Pop(MatchedStack);
            UINT64               JumpAddress      = DecimalToInt(JumpAddressToken->Value);

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
            PSCRIPT_ENGINE_TOKEN JzAddressToken = NewToken(DECIMAL, str);
            Push(MatchedStack, JzAddressToken);

            //
            // Push @INC_DEC token to matched stack
            //
            PSCRIPT_ENGINE_TOKEN IncDecToken = NewToken(SEMANTIC_RULE, "@INC_DEC");
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
            PSCRIPT_ENGINE_TOKEN JumpAddressToken = Pop(MatchedStack);
            UINT64               JumpAddress      = DecimalToInt(JumpAddressToken->Value);

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

            PSCRIPT_ENGINE_TOKEN_LIST TempStack = NewTokenList();
            PSCRIPT_ENGINE_TOKEN      TempToken;
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
                    PSCRIPT_ENGINE_TOKEN CurrentAddressToken = NewToken(DECIMAL, str);
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
            PSCRIPT_ENGINE_TOKEN_LIST TempStack = NewTokenList();
            PSCRIPT_ENGINE_TOKEN      TempToken;
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

            Temp = NewTemp(Error);
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

            Temp = NewTemp(Error);
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

            Op2       = Pop(MatchedStack);
            Op2Symbol = ToSymbol(Op2, Error);

            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, Op1Symbol);
            PushSymbol(CodeBuffer, Op2Symbol);

            Temp = NewTemp(Error);
            Push(MatchedStack, Temp);
            TempSymbol = ToSymbol(Temp, Error);
            PushSymbol(CodeBuffer, TempSymbol);

            //
            // Free the operand if it is a temp value
            //
            FreeTemp(Op0);
            FreeTemp(Op1);
            FreeTemp(Op2);
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

            Temp = NewTemp(Error);
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

            Temp = NewTemp(Error);
            Push(MatchedStack, Temp);
            TempSymbol = ToSymbol(Temp, Error);
            PushSymbol(CodeBuffer, TempSymbol);

            //
            // Free the operand if it is a temp value
            //
            FreeTemp(Op0);
            FreeTemp(Op1);
        }

        else if (IsType15Func(Operator))
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

            Temp = NewTemp(Error);
            Push(MatchedStack, Temp);
            TempSymbol = ToSymbol(Temp, Error);
            PushSymbol(CodeBuffer, TempSymbol);

            //
            // Free the operand if it is a temp value
            //
            FreeTemp(Op0);
            FreeTemp(Op1);
            FreeTemp(Op2);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                break;
            }
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
    PrintSymbolBuffer((PVOID)CodeBuffer);
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
BooleanExpressionExtractEnd(char * str, BOOL * WaitForWaitStatementBooleanExpression, PSCRIPT_ENGINE_TOKEN CurrentIn)
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
    PSCRIPT_ENGINE_TOKEN      FirstToken,
    PSCRIPT_ENGINE_TOKEN_LIST MatchedStack,
    PSYMBOL_BUFFER            CodeBuffer,
    char *                    str,
    char *                    c,
    PSCRIPT_ENGINE_ERROR_TYPE Error)
{
    PSCRIPT_ENGINE_TOKEN_LIST Stack = NewTokenList();

    PSCRIPT_ENGINE_TOKEN State = NewToken(STATE_ID, "0");
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
    PSCRIPT_ENGINE_TOKEN EndToken = NewToken(END_OF_STACK, "$");

    PSCRIPT_ENGINE_TOKEN CurrentIn = CopyToken(FirstToken);

    PSCRIPT_ENGINE_TOKEN TopToken     = NULL;
    PSCRIPT_ENGINE_TOKEN Lhs          = NULL;
    PSCRIPT_ENGINE_TOKEN Temp         = NULL;
    PSCRIPT_ENGINE_TOKEN Operand      = NULL;
    PSCRIPT_ENGINE_TOKEN SemanticRule = NULL;

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
            Lhs          = (PSCRIPT_ENGINE_TOKEN)&LalrLhs[StateId - 1];
            RhsSize      = LalrGetRhsSize(StateId - 1);
            SemanticRule = (PSCRIPT_ENGINE_TOKEN)&LalrSemanticRules[StateId - 1];

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
                    CodeGen(MatchedStack, CodeBuffer, SemanticRule, Error);
                    if (*Error != SCRIPT_ENGINE_ERROR_FREE)
                    {
                        break;
                    }
                }
            }

            Temp    = Top(Stack);
            StateId = (int)DecimalToSignedInt(Temp->Value);

            Goto = LalrGotoTable[StateId][LalrGetNonTerminalId(Lhs)];

            PSCRIPT_ENGINE_TOKEN LhsCopy = CopyToken(Lhs);

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

    Symbol->Value = 0;
    Symbol->Len   = 0;
    Symbol->Type  = 0;
    return Symbol;
}

/**
 * @brief Allocates a new SYMBOL with string type and returns the reference to it
 *
 * @param value
 * @return PSYMBOL
 */
PSYMBOL
NewStringSymbol(PSCRIPT_ENGINE_TOKEN Token)
{
    PSYMBOL Symbol;
    int     BufferSize = (SIZE_SYMBOL_WITHOUT_LEN + Token->Len) / sizeof(SYMBOL) + 1;
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
    Symbol->Len = Token->Len;
    return Symbol;
}

/**
 * @brief Allocates a new SYMBOL with wstring type and returns the reference to it
 *
 * @param value
 * @return PSYMBOL
 */
PSYMBOL
NewWstringSymbol(PSCRIPT_ENGINE_TOKEN Token)
{
    PSYMBOL Symbol;
    int     BufferSize = (SIZE_SYMBOL_WITHOUT_LEN + Token->Len) / sizeof(SYMBOL) + 1;
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
    Symbol->Len = Token->Len;
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
    int Temp = (SIZE_SYMBOL_WITHOUT_LEN + (int)Symbol->Len) / sizeof(SYMBOL) + 1;
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
 * @param PVOID
 */
void
PrintSymbol(PVOID Symbol)
{
    PSYMBOL Sym = (PSYMBOL)Symbol;

    if (Sym->Type & 0xffffffff00000000)
    {
        printf("Type = @VARGSTART\n");
        return;
    }

    printf("Type = %s, ", SymbolTypeNames[Sym->Type]);

    if (Sym->Type == SYMBOL_SEMANTIC_RULE_TYPE)
    {
        printf("Value = %s\n", FunctionNames[Sym->Value]);
    }
    else if (Sym->Type == SYMBOL_STRING_TYPE)
    {
        printf("Value = %s\n", (char *)&Sym->Value);
    }
    else if (Sym->Type == SYMBOL_WSTRING_TYPE)
    {
        printf("Value = %ls\n", (wchar_t *)&Sym->Value);
    }
    else
    {
        printf("Value = %lld\n", Sym->Value);
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
ToSymbol(PSCRIPT_ENGINE_TOKEN Token, PSCRIPT_ENGINE_ERROR_TYPE Error)
{
    PSYMBOL Symbol = NewSymbol();
    switch (Token->Type)
    {
    case GLOBAL_ID:
        Symbol->Value = GetGlobalIdentifierVal(Token);
        SetType(&Symbol->Type, SYMBOL_GLOBAL_ID_TYPE);
        return Symbol;

    case LOCAL_ID:
    {
        Symbol->Value = GetLocalIdentifierVal(Token);

        if (((VARIABLE_TYPE *)Token->VariableType)->Kind == TY_ARRAY)
        {
            SetType(&Symbol->Type, SYMBOL_REFERENCE_LOCAL_ID_TYPE);
        }
        else
        {
            SetType(&Symbol->Type, SYMBOL_LOCAL_ID_TYPE);
        }

        return Symbol;
    }

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

        if (((VARIABLE_TYPE *)Token->VariableType)->Kind == TY_ARRAY)
        {
            SetType(&Symbol->Type, SYMBOL_REFERENCE_TEMP_TYPE);
        }
        else
        {
            SetType(&Symbol->Type, SYMBOL_TEMP_TYPE);
        }

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

    case DEFERENCE_TEMP:
        Symbol->Value = DecimalToInt(Token->Value);
        SetType(&Symbol->Type, SYMBOL_DEREFERENCE_TEMP_TYPE);
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
RemoveSymbolBuffer(PVOID SymbolBuffer)
{
    PSYMBOL_BUFFER SymBuf = (PSYMBOL_BUFFER)SymbolBuffer;

    free(SymBuf->Message);
    free(SymBuf->Head);
    free(SymBuf);
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
PrintSymbolBuffer(const PVOID SymbolBuffer)
{
    PSYMBOL_BUFFER SymBuff = (PSYMBOL_BUFFER)SymbolBuffer;
    PSYMBOL        Symbol;
    printf("CodeBuffer:\n");
    for (unsigned int i = 0; i < SymBuff->Pointer;)
    {
        Symbol = SymBuff->Head + i;
        printf("Address = %d, ", i);
        PrintSymbol((PVOID)Symbol);
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
    //
    // Check for register names
    //
    for (int i = 0; i < REGISTER_MAP_LIST_LENGTH; i++)
    {
        if (!strcmp(str, RegisterMapList[i].Name))
        {
            return RegisterMapList[i].Type;
        }
    }

    //
    // Check for hwdbg register names
    // Check if the registers start with '@hw_portX' or '@hw_pinX'
    //
    if (g_HwdbgInstanceInfoIsValid)
    {
        const char * Ptr;
        UINT32       Num = 0;

        //
        // Check for "hw_pin"
        //
        if (strncmp(str, "hw_pin", 6) == 0)
        {
            Ptr = str + 6;
            if (*Ptr == '\0')
            {
                return INVALID; // No number present
            }
            while (*Ptr)
            {
                if (!isdigit((unsigned char)*Ptr))
                {
                    return INVALID; // Not a valid decimal number
                }
                Ptr++;
            }
            Num = atoi(str + 6);

            //
            // port numbers start after the latest pin number
            //
            if (Num >= g_HwdbgInstanceInfo.numberOfPins)
            {
                return INVALID; // Invalid "hw_pinX"
            }
            else
            {
                return Num; // Valid "hw_pinX"
            }
        }

        //
        // Check for "hw_port"
        //
        if (strncmp(str, "hw_port", 7) == 0)
        {
            Ptr = str + 7;
            if (*Ptr == '\0')
            {
                return INVALID; // No number present
            }
            while (*Ptr)
            {
                if (!isdigit((unsigned char)*Ptr))
                {
                    return INVALID; // Not a valid decimal number
                }

                Ptr++;
            }

            Num = atoi(str + 7);

            if (Num >= g_HwdbgInstanceInfo.numberOfPorts)
            {
                return INVALID; // Invalid "hw_portX"
            }
            else
            {
                return Num + g_HwdbgInstanceInfo.numberOfPins; // Valid "hw_portX"
            }
        }
    }

    //
    // Not a valid register name
    //
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
GetGlobalIdentifierVal(PSCRIPT_ENGINE_TOKEN Token)
{
    PSCRIPT_ENGINE_TOKEN CurrentToken;
    for (uintptr_t i = 0; i < GlobalIdTable->Pointer; i++)
    {
        CurrentToken = *(GlobalIdTable->Head + i);
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
GetLocalIdentifierVal(PSCRIPT_ENGINE_TOKEN Token)
{
    PSCRIPT_ENGINE_TOKEN CurrentToken;
    for (uintptr_t i = 0; i < ((PSCRIPT_ENGINE_TOKEN_LIST)CurrentUserDefinedFunction->IdTable)->Pointer; i++)
    {
        CurrentToken = *(((PSCRIPT_ENGINE_TOKEN_LIST)CurrentUserDefinedFunction->IdTable)->Head + i);
        if (!strcmp(Token->Value, CurrentToken->Value))
        {
            return (int)CurrentToken->VariableMemoryIdx;
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
NewGlobalIdentifier(PSCRIPT_ENGINE_TOKEN Token)
{
    PSCRIPT_ENGINE_TOKEN CopiedToken = CopyToken(Token);
    GlobalIdTable                    = Push(GlobalIdTable, CopiedToken);
    return GlobalIdTable->Pointer - 1;
}

/**
 * @brief
 *
 * @param Token
 */
VOID
SetGlobalIdentifierVariableType(PSCRIPT_ENGINE_TOKEN Token, unsigned long long VariableType)
{
    PSCRIPT_ENGINE_TOKEN CurrentToken;
    for (uintptr_t i = 0; i < GlobalIdTable->Pointer; i++)
    {
        CurrentToken = *(GlobalIdTable->Head + i);
        if (!strcmp(Token->Value, CurrentToken->Value))
        {
            CurrentToken->VariableType = VariableType;
        }
    }
}

/**
 * @brief
 *
 * @param Token
 */
unsigned long long
GetGlobalIdentifierVariableType(PSCRIPT_ENGINE_TOKEN Token)
{
    PSCRIPT_ENGINE_TOKEN CurrentToken;
    for (uintptr_t i = 0; i < GlobalIdTable->Pointer; i++)
    {
        CurrentToken = *(GlobalIdTable->Head + i);
        if (!strcmp(Token->Value, CurrentToken->Value))
        {
            return CurrentToken->VariableType;
        }
    }
    return 0;
}

/**
 * @brief Allocates a new local variable and returns the integer assigned to it
 *
 * @param Token
 * @return int
 */
unsigned long long
NewLocalIdentifier(PSCRIPT_ENGINE_TOKEN Token, unsigned int VariableSize)
{
    PSCRIPT_ENGINE_TOKEN CopiedToken    = CopyToken(Token);
    unsigned int         VariableNumber = ((VariableSize + 8 - 1) & ~(8 - 1)) / 8;
    CopiedToken->VariableMemoryIdx      = CurrentUserDefinedFunction->LocalVariableNumber;
    CurrentUserDefinedFunction->LocalVariableNumber += VariableNumber;
    Push(((PSCRIPT_ENGINE_TOKEN_LIST)CurrentUserDefinedFunction->IdTable), CopiedToken);
    return CopiedToken->VariableMemoryIdx;
}

/**
 * @brief
 *
 * @param Token
 */
VOID
SetLocalIdentifierVariableType(PSCRIPT_ENGINE_TOKEN Token, unsigned long long VariableType)
{
    PSCRIPT_ENGINE_TOKEN CurrentToken;
    for (uintptr_t i = 0; i < ((PSCRIPT_ENGINE_TOKEN_LIST)CurrentUserDefinedFunction->IdTable)->Pointer; i++)
    {
        CurrentToken = *(((PSCRIPT_ENGINE_TOKEN_LIST)CurrentUserDefinedFunction->IdTable)->Head + i);
        if (!strcmp(Token->Value, CurrentToken->Value))
        {
            CurrentToken->VariableType = VariableType;
        }
    }
}

/**
 * @brief
 *
 * @param Token
 * @return unsigned long long
 */
unsigned long long
GetLocalIdentifierVariableType(PSCRIPT_ENGINE_TOKEN Token)
{
    PSCRIPT_ENGINE_TOKEN CurrentToken;
    for (uintptr_t i = 0; i < ((PSCRIPT_ENGINE_TOKEN_LIST)CurrentUserDefinedFunction->IdTable)->Pointer; i++)
    {
        CurrentToken = *(((PSCRIPT_ENGINE_TOKEN_LIST)CurrentUserDefinedFunction->IdTable)->Head + i);
        if (!strcmp(Token->Value, CurrentToken->Value))
        {
            return CurrentToken->VariableType;
        }
    }
    return 0;
}

/**
 * @brief
 *
 * @param Token
 * @return int
 */
int
NewFunctionParameterIdentifier(PSCRIPT_ENGINE_TOKEN Token)
{
    PSCRIPT_ENGINE_TOKEN CopiedToken = CopyToken(Token);
    Push(((PSCRIPT_ENGINE_TOKEN_LIST)CurrentUserDefinedFunction->FunctionParameterIdTable), CopiedToken);
    return ((PSCRIPT_ENGINE_TOKEN_LIST)CurrentUserDefinedFunction->FunctionParameterIdTable)->Pointer - 1;
}

/**
 * @brief
 *
 * @param Token
 * @return int
 */
int
GetFunctionParameterIdentifier(PSCRIPT_ENGINE_TOKEN Token)
{
    PSCRIPT_ENGINE_TOKEN CurrentToken;
    for (uintptr_t i = 0; i < ((PSCRIPT_ENGINE_TOKEN_LIST)CurrentUserDefinedFunction->FunctionParameterIdTable)->Pointer; i++)
    {
        CurrentToken = *(((PSCRIPT_ENGINE_TOKEN_LIST)CurrentUserDefinedFunction->FunctionParameterIdTable)->Head + i);
        if (!strcmp(Token->Value, CurrentToken->Value))
        {
            return (int)i;
        }
    }
    return -1;
}

/**
 * @brief
 *
 * @param Token
 * @return bool
 */
PUSER_DEFINED_FUNCTION_NODE
GetUserDefinedFunctionNode(PSCRIPT_ENGINE_TOKEN Token)
{
    PUSER_DEFINED_FUNCTION_NODE Node = UserDefinedFunctionHead;
    while (Node)
    {
        if (!strcmp((const char *)Token->Value, Node->Name))
        {
            return Node;
            break;
        }
        Node = Node->NextNode;
    }
    return 0;
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
LalrIsOperandType(PSCRIPT_ENGINE_TOKEN Token)
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
    else if (Token->Type == FUNCTION_ID)
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

/**
 * @brief Set hwdbg instance info for the script engine
 *
 * @param InstancInfo
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineSetHwdbgInstanceInfo(HWDBG_INSTANCE_INFORMATION * InstancInfo)
{
    //
    // Copy the instance info into the global variable
    //
    memcpy(&g_HwdbgInstanceInfo, InstancInfo, sizeof(HWDBG_INSTANCE_INFORMATION));

    //
    // Indicate that the instance info is valid
    //
    g_HwdbgInstanceInfoIsValid = TRUE;

    return TRUE;
}

/**
 * @brief Script Engine get number of operands
 *
 * @param FuncType
 * @param NumberOfGetOperands
 * @param NumberOfSetOperands
 * @param BOOLEAN Whether the function is defined or not
 */
BOOLEAN
FuncGetNumberOfOperands(UINT64 FuncType, UINT32 * NumberOfGetOperands, UINT32 * NumberOfSetOperands)
{
    BOOLEAN Result = FALSE;

    switch (FuncType)
    {
    case FUNC_INC:

        *NumberOfGetOperands = 1;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_DEC:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_OR:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_XOR:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_AND:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_ASL:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_ADD:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_SUB:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_MUL:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_DIV:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_MOD:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_GT:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_LT:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_EGT:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_ELT:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_EQUAL:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_NEQ:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_JMP:

        *NumberOfGetOperands = 1;
        *NumberOfSetOperands = 0;
        Result               = TRUE;

        break;

    case FUNC_JZ:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 0;
        Result               = TRUE;

        break;

    case FUNC_JNZ:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 0;
        Result               = TRUE;

        break;

    case FUNC_MOV:

        *NumberOfGetOperands = 1;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    default:
        //
        // Not defined
        //
        Result = FALSE;
        break;
    }

    return Result;
}
