/**
 * @file script-engine-wrapper.cpp
 * @author M.H. Gholamrezei (gholamrezaei.mh@gmail.com)
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Interpret general fields
 * @details
 * @version 0.1
 * @date 2020-10-25
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Include parser
//
#define SCRIPT_ENGINE_USER_MODE
#include "ScriptEngineCommon.h"

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
 * @param SymbolPath
 * 
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineSymbolInitLoadWrapper(PMODULE_SYMBOL_DETAIL BufferToStoreDetails, UINT32 StoredLength, const char * SymbolPath)
{
    return ScriptEngineSymbolInitLoad(BufferToStoreDetails, StoredLength, SymbolPath);
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
 * @param str
 * 
 * @return PVOID
 */
PVOID
ScriptEngineParseWrapper(char * str)
{
    PSYMBOL_BUFFER SymbolBuffer;
    SymbolBuffer = ScriptEngineParse(str);

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
        ShowMessages("syntax error:\n %s\n", SymbolBuffer->Message);
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
ScriptEngineWrapperTestPerformAction(PGUEST_REGS GuestRegs,
                                     string      Expr)
{
    //
    // Test Parser
    //
    PSYMBOL_BUFFER CodeBuffer = ScriptEngineParse((char *)Expr.c_str());

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

                break;
            }
        }
    }
    else
    {
        ShowMessages("%s\n", CodeBuffer->Message);
    }

    RemoveSymbolBuffer(CodeBuffer);

    return;
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

    //
    // Allocate global variables holder
    //
    if (!g_ScriptGlobalVariables)
    {
        g_ScriptGlobalVariables = (UINT64 *)malloc(MAX_VAR_COUNT * sizeof(UINT64));
        RtlZeroMemory(g_ScriptGlobalVariables, MAX_VAR_COUNT * sizeof(UINT64));
    }

    ScriptEngineWrapperTestPerformAction(&GuestRegs, Expr);
    free(TestStruct);
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
