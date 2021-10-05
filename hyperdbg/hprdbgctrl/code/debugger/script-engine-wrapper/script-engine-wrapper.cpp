/**
 * @file script-engine-wrapper.cpp
 * @author M.H. Gholamrezaei (gholamrezaei.mh@gmail.com)
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Interpret general fields
 * @details
 * @version 0.1
 * @date 2020-10-25
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\hprdbgctrl\pch.h"

//
// Include parser
//
#define SCRIPT_ENGINE_USER_MODE
#include "ScriptEngineEval.h"

//
// Global Variables
//
extern UINT64 * g_ScriptGlobalVariables;

//
// *********************** Pdb parse wrapper ***********************
//

/**
 * @brief ScriptEngineConvertNameToAddress wrapper
 *
 * @param FunctionName
 * @param WasFound
 * 
 * @return UINT64
 */
UINT64
ScriptEngineConvertNameToAddressWrapper(const char * FunctionOrVariableName, PBOOLEAN WasFound)
{
    return ScriptEngineConvertNameToAddress(FunctionOrVariableName, WasFound);
}

/**
 * @brief ScriptEngineLoadFileSymbol wrapper
 *
 * @param BaseAddress
 * @param FileName
 * @param Guid
 * 
 * @return UINT32
 */
UINT32
ScriptEngineLoadFileSymbolWrapper(UINT64 BaseAddress, const char * PdbFileName)
{
    return ScriptEngineLoadFileSymbol(BaseAddress, PdbFileName);
}

/**
 * @brief ScriptEngineSetTextMessageCallback wrapper
 *
 * @param Handler
 * 
 * @return VOID
 */
VOID
ScriptEngineSetTextMessageCallbackWrapper(PVOID Handler)
{
    return ScriptEngineSetTextMessageCallback(Handler);
}

/**
 * @brief ScriptEngineUnloadAllSymbols wrapper
 * 
 * @return UINT32
 */
UINT32
ScriptEngineUnloadAllSymbolsWrapper()
{
    return ScriptEngineUnloadAllSymbols();
}

/**
 * @brief ScriptEngineUnloadModuleSymbol wrapper
 * @param ModuleName
 * 
 * @return UINT32
 */
UINT32
ScriptEngineUnloadModuleSymbolWrapper(char * ModuleName)
{
    return ScriptEngineUnloadModuleSymbol(ModuleName);
}

/**
 * @brief ScriptEngineSearchSymbolForMask wrapper
 *
 * @param SearchMask
 * 
 * @return UINT32
 */
UINT32
ScriptEngineSearchSymbolForMaskWrapper(const char * SearchMask)
{
    return ScriptEngineSearchSymbolForMask(SearchMask);
}

/**
 * @brief ScriptEngineConvertFileToPdbPath wrapper
 *
 * @param LocalFilePath
 * @param ResultPath
 * 
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineConvertFileToPdbPathWrapper(const char * LocalFilePath, char * ResultPath)

{
    return ScriptEngineConvertFileToPdbPath(LocalFilePath, ResultPath);
}

/**
 * @brief ScriptEngineSymbolInitLoad wrapper
 *
 * @param ScriptEngineSymbolInitLoad
 * @param StoredLength
 * @param DownloadIfAvailable
 * @param SymbolPath
 * @param IsSilentLoad
 * 
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineSymbolInitLoadWrapper(PMODULE_SYMBOL_DETAIL BufferToStoreDetails,
                                  UINT32                StoredLength,
                                  BOOLEAN               DownloadIfAvailable,
                                  const char *          SymbolPath,
                                  BOOLEAN               IsSilentLoad)
{
    return ScriptEngineSymbolInitLoad(BufferToStoreDetails, StoredLength, DownloadIfAvailable, SymbolPath, IsSilentLoad);
}

/**
 * @brief SymbolAbortLoading wrapper
 *
 * @return VOID
 */
VOID
ScriptEngineSymbolAbortLoadingWrapper()

{
    return ScriptEngineSymbolAbortLoading();
}

/**
 * @brief ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetails wrapper
 *
 * @param LocalFilePath
 * @param PdbFilePath
 * @param GuidAndAgeDetails
 * 
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetailsWrapper(const char * LocalFilePath,
                                                            char *       PdbFilePath,
                                                            char *       GuidAndAgeDetails)

{
    return ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetails(LocalFilePath, PdbFilePath, GuidAndAgeDetails);
}

//
// *********************** Function links (wrapper) ***********************
//

/**
 * @brief ScriptEngineParse wrapper
 *
 * @param Expr
 * @param ShowErrorMessageIfAny
 * 
 * @return PVOID
 */
PVOID
ScriptEngineParseWrapper(char * Expr, BOOLEAN ShowErrorMessageIfAny)
{
    PSYMBOL_BUFFER SymbolBuffer;
    SymbolBuffer = ScriptEngineParse(Expr);

    //
    // Check if there is an error or not
    //
    if (SymbolBuffer->Message == NULL)
    {
        return SymbolBuffer;
    }
    else
    {
        //
        // Show error message and free the buffer
        //
        if (ShowErrorMessageIfAny)
        {
            ShowMessages("%s\n", SymbolBuffer->Message);
        }
        ScriptEngineWrapperRemoveSymbolBuffer(SymbolBuffer);
        return NULL;
    }
}

/**
 * @brief PrintSymbolBuffer wrapper
 * @details Print symbol buffer wrapper
 * @param SymbolBuffer
 * 
 * @return PVOID
 */
VOID
PrintSymbolBufferWrapper(PVOID SymbolBuffer)
{
    PrintSymbolBuffer((PSYMBOL_BUFFER)SymbolBuffer);
}

/**
 * @brief test function
 * @param GuestRegs
 * @param Expr
 * 
 * @return VOID
 */
VOID
ScriptEngineEvalWrapper(PGUEST_REGS GuestRegs,
                        string      Expr)
{
    //
    // Allocate global variables holder
    //
    if (!g_ScriptGlobalVariables)
    {
        g_ScriptGlobalVariables = (UINT64 *)malloc(MAX_VAR_COUNT * sizeof(UINT64));
        RtlZeroMemory(g_ScriptGlobalVariables, MAX_VAR_COUNT * sizeof(UINT64));
    }

    //
    // Run Parser
    //
    PSYMBOL_BUFFER CodeBuffer = ScriptEngineParse((char *)Expr.c_str());

    //
    // Print symbol buffer
    //
    //PrintSymbolBuffer(CodeBuffer);

    UINT64        g_TempList[MAX_TEMP_COUNT] = {0};
    ACTION_BUFFER ActionBuffer               = {0};
    SYMBOL        ErrorSymbol                = {0};

    if (CodeBuffer->Message == NULL)
    {
        for (int i = 0; i < CodeBuffer->Pointer;)
        {
            //
            // Fill the action buffer but as we're in user-mode here
            // then there is nothing to fill
            //
            ActionBuffer.Context                   = NULL;
            ActionBuffer.CurrentAction             = NULL;
            ActionBuffer.ImmediatelySendTheResults = FALSE;
            ActionBuffer.Tag                       = NULL;

            //
            // If has error, show error message and abort.
            //
            if (ScriptEngineExecute(GuestRegs, ActionBuffer, (UINT64 *)g_TempList, (UINT64 *)g_ScriptGlobalVariables, CodeBuffer, &i, &ErrorSymbol) == TRUE)
            {
                CHAR NameOfOperator[MAX_FUNCTION_NAME_LENGTH] = {0};

                ScriptEngineGetOperatorName(&ErrorSymbol, NameOfOperator);
                ShowMessages("invalid returning address for operator: %s",
                             NameOfOperator);
                g_CurrentExprEvalResultHasError = TRUE;
                g_CurrentExprEvalResult         = NULL;
                break;
            }
        }
    }
    else
    {
        ShowMessages("%s\n", CodeBuffer->Message);
    }

    // RemoveSymbolBuffer(CodeBuffer);

    return;
}

/**
 * @brief massive tests for script engine statements
 * @param Expr The expression to test
 * @param ExpectationValue What value this statements expects (not 
 * used if ExceptError is TRUE)
 * @param ExceptError True if the statement expects an error
 * 
 * @return BOOLEAN whether the test was successful or not
 */
BOOLEAN
ScriptAutomaticStatementsTestWrapper(string Expr, UINT64 ExpectationValue, BOOLEAN ExceptError)
{
    //
    // Call the test parser
    //
    ScriptEngineWrapperTestParser(Expr);

    //
    // Check the global variable to see the results
    //
    if (g_CurrentExprEvalResultHasError && ExceptError)
    {
        return TRUE;
    }
    else if (ExpectationValue == g_CurrentExprEvalResult)
    {
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief test parser
 * @param Expr
 * 
 * @return VOID
 */
VOID
ScriptEngineWrapperTestParser(string Expr)
{
    typedef struct _TEST_STRUCT
    {
        UINT64 Var1;
        UINT64 Var2;
        UINT64 Var3;
        UINT64 Var4;
    } TEST_STRUCT, *PTEST_STRUCT;

    PTEST_STRUCT TestStruct = (PTEST_STRUCT)malloc(sizeof(TEST_STRUCT));
    RtlZeroMemory(TestStruct, sizeof(TEST_STRUCT));

    TestStruct->Var1 = 0x41414141;
    TestStruct->Var3 = 0x4242424242424242;

    GUEST_REGS GuestRegs = {0};

    char    test[] = "Hello world !";
    wchar_t testw[] =
        L"A B C D E F G H I J K L M N O P Q R S T U V W X Y Z 0 1 2 3 4 5 6 7 8 "
        L"9 a b c d e f g h i j k l m n o p q r s t u v w x y z";

    GuestRegs.rax = 0x1;
    GuestRegs.rcx = (UINT64)TestStruct;
    GuestRegs.rdx = 0x3;
    GuestRegs.rbx = 0x4;
    GuestRegs.rsp = 0x5;
    GuestRegs.rbp = 0x6;
    GuestRegs.rsi = 0x7;
    GuestRegs.rdi = 0x8;
    GuestRegs.r8  = 0x9;
    GuestRegs.r9  = 0xa;
    GuestRegs.r10 = 0xb;
    GuestRegs.r11 = 0xc;
    GuestRegs.r12 = 0xd;
    GuestRegs.r13 = 0xe;
    GuestRegs.r14 = (ULONG64)testw;
    GuestRegs.r15 = (ULONG64)test;

    ScriptEngineEvalWrapper(&GuestRegs, Expr);
    free(TestStruct);
}

/**
 * @brief In the local debugging (VMI mode) environment, this function computes the expressions
 * @details for example, if the user u ExAllocatePoolWithTag+0x10 this will evaluate the expr
 * @param Expr
 * @param HasError
 * 
 * @return UINT64
 */
UINT64
ScriptEngineEvalUInt64StyleExpressionWrapper(string Expr, PBOOLEAN HasError)
{
    //
    // In VMI-mode we'll form all registers as zero
    //
    GUEST_REGS GuestRegs = {0};

    ScriptEngineEvalWrapper(&GuestRegs, Expr);

    //
    // Set the results and return the value
    //
    *HasError = g_CurrentExprEvalResultHasError;
    return g_CurrentExprEvalResult;
}

/**
 * @brief wrapper for getting head 
 * @param SymbolBuffer
 * 
 * @return UINT64
 */
UINT64
ScriptEngineWrapperGetHead(PVOID SymbolBuffer)
{
    return (UINT64)((PSYMBOL_BUFFER)SymbolBuffer)->Head;
}

/**
 * @brief wrapper for getting size 
 * @param SymbolBuffer
 * 
 * @return UINT32
 */
UINT32
ScriptEngineWrapperGetSize(PVOID SymbolBuffer)
{
    UINT32 Size =
        (UINT32)((PSYMBOL_BUFFER)SymbolBuffer)->Pointer * sizeof(SYMBOL);
    return Size;
}

/**
 * @brief wrapper for getting pointer 
 * @param SymbolBuffer
 * 
 * @return UINT32
 */
UINT32
ScriptEngineWrapperGetPointer(PVOID SymbolBuffer)
{
    return (UINT32)((PSYMBOL_BUFFER)SymbolBuffer)->Pointer;
}

/**
 * @brief wrapper for removing symbol buffer 
 * @param SymbolBuffer
 * 
 * @return UINT32
 */
VOID
ScriptEngineWrapperRemoveSymbolBuffer(PVOID SymbolBuffer)
{
    RemoveSymbolBuffer((PSYMBOL_BUFFER)SymbolBuffer);
}