/**
 * @file script-engine-wrapper.cpp
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Interpret general fields
 * @details
 * @version 0.1
 * @date 2020-10-25
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

// #define _SCRIPT_ENGINE_IR_PRINT_EN
// #define _SCRIPT_ENGINE_CODEEXEC_DBG_EN

//
// Global Variables
//
extern UINT64 * g_ScriptGlobalVariables;
extern UINT64 * g_ScriptStackBuffer;
extern UINT64   g_CurrentExprEvalResult;
extern BOOLEAN  g_CurrentExprEvalResultHasError;
extern UINT64 * g_HwdbgPinsStatus;
extern BOOLEAN  g_HwdbgInstanceInfoIsValid;

//
// Temporary structures used only for testing
//
typedef struct _ALLOCATED_MEMORY_FOR_SCRIPT_ENGINE_CASTING
{
    CHAR * Buff1;
    CHAR * Buff2;
    CHAR * Buff3;
    CHAR * Buff4;
    CHAR * Buff5;
    CHAR * Buff6;

} ALLOCATED_MEMORY_FOR_SCRIPT_ENGINE_CASTING, *PALLOCATED_MEMORY_FOR_SCRIPT_ENGINE_CASTING;

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
 *
 * @return UINT32
 */
UINT32
ScriptEngineLoadFileSymbolWrapper(UINT64 BaseAddress, const char * PdbFileName, const char * CustomModuleName)
{
    return ScriptEngineLoadFileSymbol(BaseAddress, PdbFileName, CustomModuleName);
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
 * @brief ScriptEngineGetFieldOffset wrapper
 *
 * @param TypeName
 * @param FieldName
 * @param FieldOffset
 *
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineGetFieldOffsetWrapper(CHAR * TypeName, CHAR * FieldName, UINT32 * FieldOffset)

{
    return ScriptEngineGetFieldOffset(TypeName, FieldName, FieldOffset);
}

/**
 * @brief ScriptEngineGetDataTypeSize wrapper
 *
 * @param TypeName
 * @param TypeSize
 *
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineGetDataTypeSizeWrapper(CHAR * TypeName, UINT64 * TypeSize)
{
    return ScriptEngineGetDataTypeSize(TypeName, TypeSize);
}

/**
 * @brief ScriptEngineCreateSymbolTableForDisassembler wrapper
 *
 * @param CallbackFunction
 *
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineCreateSymbolTableForDisassemblerWrapper(void * CallbackFunction)
{
    return ScriptEngineCreateSymbolTableForDisassembler(CallbackFunction);
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
ScriptEngineConvertFileToPdbPathWrapper(const char * LocalFilePath, char * ResultPath, size_t ResultPathSize)

{
    return ScriptEngineConvertFileToPdbPath(LocalFilePath, ResultPath, ResultPathSize);
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
 * @brief ScriptEngineShowDataBasedOnSymbolTypes wrapper
 *
 * @param TypeName
 * @param Address
 * @param IsStruct
 * @param BufferAddress
 * @param AdditionalParameters
 *
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineShowDataBasedOnSymbolTypesWrapper(
    const char * TypeName,
    UINT64       Address,
    BOOLEAN      IsStruct,
    PVOID        BufferAddress,
    const char * AdditionalParameters)
{
    return ScriptEngineShowDataBasedOnSymbolTypes(TypeName, Address, IsStruct, BufferAddress, AdditionalParameters);
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
 * @param Is32BitModule
 *
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetailsWrapper(const char * LocalFilePath,
                                                            char *       PdbFilePath,
                                                            char *       GuidAndAgeDetails,
                                                            BOOLEAN      Is32BitModule)

{
    return ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetails(LocalFilePath, PdbFilePath, GuidAndAgeDetails, Is32BitModule);
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
    SymbolBuffer = (PSYMBOL_BUFFER)ScriptEngineParse(Expr);

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
    PrintSymbolBuffer(SymbolBuffer);
}

/**
 * @brief Script engine evaluation wrapper
 * @param GuestRegs
 * @param Expr
 *
 * @return VOID
 */
VOID
ScriptEngineEvalWrapper(PGUEST_REGS GuestRegs,
                        string      Expr)
{
    SCRIPT_ENGINE_GENERAL_REGISTERS ScriptGeneralRegisters = {0};

    //
    // Allocate global variables holder
    //
    if (!g_ScriptGlobalVariables)
    {
        g_ScriptGlobalVariables = (UINT64 *)malloc(MAX_VAR_COUNT * sizeof(UINT64));

        if (g_ScriptGlobalVariables == NULL)
        {
            ShowMessages("err, could not allocate memory for user-mode global variables");

            return;
        }

        RtlZeroMemory(g_ScriptGlobalVariables, MAX_VAR_COUNT * sizeof(UINT64));
    }

    //
    // Allocate stack buffer holder, actually in reality each core should
    // have its own set of stack buffer but as we never run multi-core scripts
    // in user-mode, thus, it's okay to just have one buffer for stack buffer
    //
    if (!g_ScriptStackBuffer)
    {
        g_ScriptStackBuffer = (UINT64 *)malloc(MAX_STACK_BUFFER_COUNT * sizeof(UINT64));

        if (g_ScriptStackBuffer == NULL)
        {
            free(g_ScriptGlobalVariables);

            ShowMessages("err, could not allocate memory for user-mode stack buffer");

            return;
        }
    }

    //
    // Run Parser
    //
    PSYMBOL_BUFFER CodeBuffer = (PSYMBOL_BUFFER)ScriptEngineParse((char *)Expr.c_str());

#ifdef _SCRIPT_ENGINE_IR_PRINT_EN
    //
    // Print symbol buffer
    //
    PrintSymbolBuffer((PVOID)CodeBuffer);
#endif

    ACTION_BUFFER ActionBuffer = {0};
    SYMBOL        ErrorSymbol  = {0};

    UINT64 EXECUTENUMBER = 0;

    ScriptGeneralRegisters.StackBuffer         = g_ScriptStackBuffer;
    ScriptGeneralRegisters.GlobalVariablesList = g_ScriptGlobalVariables;
    RtlZeroMemory(g_ScriptStackBuffer, MAX_STACK_BUFFER_COUNT * sizeof(UINT64));

    if (CodeBuffer->Message == NULL)
    {
#ifdef _SCRIPT_ENGINE_CODEEXEC_DBG_EN
        printf("\nScriptEngineExecute:\n");
#endif
        UINT64 i = 0;
        for (; i < CodeBuffer->Pointer;)
        {
            //
            // Fill the action buffer but as we're in user-mode here
            // then there is nothing to fill
            //
            ActionBuffer.Context                   = NULL;
            ActionBuffer.CurrentAction             = NULL;
            ActionBuffer.ImmediatelySendTheResults = FALSE;
            ActionBuffer.Tag                       = NULL;

#ifdef _SCRIPT_ENGINE_CODEEXEC_DBG_EN
            printf("Address = %lld, StackIndx = %lld, StackBaseIndx = %lld\n", i, ScriptGeneralRegisters.StackIndx, ScriptGeneralRegisters.StackBaseIndx);
            PSYMBOL Operator = (PSYMBOL)((unsigned long long)CodeBuffer->Head +
                                         (unsigned long long)(i * sizeof(SYMBOL)));
            printf("Function = %s\n", FunctionNames[Operator->Value]);
            printf("Stack Buffer:\n");
            for (UINT64 j = 0; j < ScriptGeneralRegisters.StackIndx; j++)
            {
                printf("StackIndx = %lld, Value = %lld", j, ScriptGeneralRegisters.StackBuffer[j]);

                if (j == ScriptGeneralRegisters.StackBaseIndx)
                {
                    printf("   <===== StackBaseIndx");
                }
                printf("\n");
            }
            printf("\n");
#endif

            //
            // If has error, show error message and abort
            //
//            if (ScriptEngineExecute(GuestRegs,//todo
//                                    &ActionBuffer,
//                                    &ScriptGeneralRegisters,
//                                    CodeBuffer,
//                                    &i,
//                                    &ErrorSymbol) == TRUE)
//            {
//                ShowMessages("err, ScriptEngineExecute, function = %s\n",
//                             FunctionNames[ErrorSymbol.Value]);
//                g_CurrentExprEvalResultHasError = TRUE;
//                g_CurrentExprEvalResult         = NULL;
//                break;
//            }
//            else
                if (ScriptGeneralRegisters.StackIndx >= MAX_STACK_BUFFER_COUNT)
            {
                ShowMessages("err, stack buffer overflow (more information: https://docs.hyperdbg.org/tips-and-tricks/misc/customize-build/change-script-engine-limitations)\n");
                g_CurrentExprEvalResultHasError = TRUE;
                g_CurrentExprEvalResult         = NULL;
                break;
            }
            else if (EXECUTENUMBER >= MAX_EXECUTION_COUNT)
            {
                ShowMessages("err, exceeding the max execution count (more information: https://docs.hyperdbg.org/tips-and-tricks/misc/customize-build/change-script-engine-limitations)\n");
                g_CurrentExprEvalResultHasError = TRUE;
                g_CurrentExprEvalResult         = NULL;
                break;
            }

            EXECUTENUMBER++;
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
 * @brief massive tests for script engine statements
 * @param Expr The expression to test
 * @param ExpectationValue What value this statements expects (not
 * used if ExceptError is TRUE)
 * @param ExceptError True if the statement expects an error
 *
 * @return BOOLEAN whether the test was successful or not
 */
BOOLEAN
ScriptAutomaticStatementsTestWrapper(const string & Expr, UINT64 ExpectationValue, BOOLEAN ExceptError)
{
    //
    // Set the global variable indicator of test_statement to 0
    //
    g_CurrentExprEvalResult = 0;

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
 * @brief allocate memory and build structure for casting
 * @param AllocationsForCastings Memory details for future deallocations
 *
 * @return PVOID
 */
PVOID
AllocateStructForCasting(PALLOCATED_MEMORY_FOR_SCRIPT_ENGINE_CASTING AllocationsForCastings)
{
    typedef struct _UNICODE_STRING
    {
        UINT16 Length;        // +0x000
        UINT16 MaximumLength; // +0x002
        PWSTR  Buffer;        // +0x004
    } UNICODE_STRING, *PUNICODE_STRING;

    typedef struct _STUPID_STRUCT1
    {
        UINT32          Flag32;      // +0x000
        UINT64          Flag64;      // +0x004
        PVOID           Context;     // +0x00c
        PUNICODE_STRING StringValue; // +0x014
    } STUPID_STRUCT1, *PSTUPID_STRUCT1;

    typedef struct _STUPID_STRUCT2
    {
        UINT32          Sina32;        // +0x000
        UINT64          Sina64;        // +0x004
        PVOID           AghaaSina;     // +0x00c
        PUNICODE_STRING UnicodeStr;    // +0x014
        PSTUPID_STRUCT1 StupidStruct1; // +0x01c

    } STUPID_STRUCT2, *PSTUPID_STRUCT2;

    //
    // Allocate UNICODE_STRING 1
    //
    WCHAR           MyString1[40]   = L"Hi come from stupid struct 1!";
    UINT32          SizeOfMyString1 = (UINT32)wcslen(MyString1) * sizeof(WCHAR) + 2;
    PUNICODE_STRING UnicodeStr1     = (PUNICODE_STRING)malloc(sizeof(UNICODE_STRING));

    if (UnicodeStr1 == NULL)
    {
        return NULL;
    }

    AllocationsForCastings->Buff1 = (CHAR *)UnicodeStr1;
    WCHAR * Buff1                 = (WCHAR *)malloc(SizeOfMyString1);

    if (Buff1 == NULL)
    {
        free(UnicodeStr1);
        return NULL;
    }

    AllocationsForCastings->Buff2 = (CHAR *)Buff1;
    RtlZeroMemory(Buff1, SizeOfMyString1);
    UnicodeStr1->Buffer = Buff1;
    UnicodeStr1->Length = UnicodeStr1->MaximumLength = SizeOfMyString1;
    memcpy(UnicodeStr1->Buffer, MyString1, SizeOfMyString1);

    //
    // Allocate UNICODE_STRING 2
    //
    WCHAR           MyString2[40]   = L"Goodbye I'm at stupid struct 2!";
    UINT32          SizeOfMyString2 = (UINT32)wcslen(MyString2) * sizeof(WCHAR) + 2;
    PUNICODE_STRING UnicodeStr2     = (PUNICODE_STRING)malloc(sizeof(UNICODE_STRING));

    if (UnicodeStr2 == NULL)
    {
        free(UnicodeStr1);
        free(Buff1);

        return NULL;
    }

    AllocationsForCastings->Buff3 = (CHAR *)UnicodeStr2;
    WCHAR * Buff2                 = (WCHAR *)malloc(SizeOfMyString2);

    if (Buff2 == NULL)
    {
        free(UnicodeStr1);
        free(Buff1);
        free(UnicodeStr2);

        return NULL;
    }

    AllocationsForCastings->Buff4 = (CHAR *)Buff2;
    RtlZeroMemory(Buff2, SizeOfMyString2);
    UnicodeStr2->Buffer = Buff2;
    UnicodeStr2->Length = UnicodeStr2->MaximumLength = SizeOfMyString2;
    memcpy(UnicodeStr2->Buffer, MyString2, SizeOfMyString2);

    //
    // Allocate STUPID_STRUCT1
    //
    PSTUPID_STRUCT1 StupidStruct1 = (PSTUPID_STRUCT1)malloc(sizeof(STUPID_STRUCT1));

    if (StupidStruct1 == NULL)
    {
        free(UnicodeStr1);
        free(Buff1);
        free(UnicodeStr2);
        free(Buff2);

        return NULL;
    }

    AllocationsForCastings->Buff5 = (CHAR *)StupidStruct1;
    StupidStruct1->Flag32         = 0x3232;
    StupidStruct1->Flag64         = 0x6464;
    StupidStruct1->Context        = (PVOID)0x85;
    StupidStruct1->StringValue    = UnicodeStr1;

    //
    // Allocate STUPID_STRUCT2
    //
    PSTUPID_STRUCT2 StupidStruct2 = (PSTUPID_STRUCT2)malloc(sizeof(STUPID_STRUCT2));

    if (StupidStruct2 == NULL)
    {
        free(UnicodeStr1);
        free(Buff1);
        free(UnicodeStr2);
        free(Buff2);
        free(StupidStruct1);

        return NULL;
    }

    AllocationsForCastings->Buff6 = (CHAR *)StupidStruct2;

    StupidStruct2->Sina32        = 0x32;
    StupidStruct2->Sina64        = 0x64;
    StupidStruct2->AghaaSina     = (PVOID)0x55;
    StupidStruct2->UnicodeStr    = UnicodeStr2;
    StupidStruct2->StupidStruct1 = StupidStruct1;

    //_CrtDbgBreak();
    return StupidStruct2;
}

/**
 * @brief test parser
 * @param Expr
 *
 * @return VOID
 */
VOID
ScriptEngineWrapperTestParser(const string & Expr)
{
    ALLOCATED_MEMORY_FOR_SCRIPT_ENGINE_CASTING AllocationsForCastings = {0};

    typedef struct _TEST_STRUCT
    {
        UINT64 Var1;
        UINT64 Var2;
        UINT64 Var3;
        UINT64 Var4;
    } TEST_STRUCT, *PTEST_STRUCT;

    PTEST_STRUCT TestStruct = (PTEST_STRUCT)malloc(sizeof(TEST_STRUCT));

    if (TestStruct == NULL)
    {
        return;
    }

    RtlZeroMemory(TestStruct, sizeof(TEST_STRUCT));

    TestStruct->Var1 = 0x41414141;
    TestStruct->Var3 = 0x4242424242424242;

    GUEST_REGS GuestRegs = {0};

    char    test[] = "Hello world !";
    wchar_t testw[] =
        L"A B C D E F G H I J K L M N O P Q R S T U V W X Y Z 0 1 2 3 4 5 6 7 8 "
        L"9 a b c d e f g h i j k l m n o p q r s t u v w x y z";

    char * RspReg = (char *)malloc(0x100);

    if (RspReg == NULL)
    {
        ShowMessages("err, unable to allocate stack for script engine tests");
        free(TestStruct);
        return;
    }

    memcpy(RspReg, testw, sizeof(testw));

    GuestRegs.rax = 0x1;
    GuestRegs.rcx = (UINT64)AllocateStructForCasting(&AllocationsForCastings); // TestStruct
    GuestRegs.rdx = 0x3;
    GuestRegs.rbx = 0x4;
    GuestRegs.rsp = (UINT64)RspReg + 0x50;
    GuestRegs.rbp = 0x6;
    GuestRegs.rsi = 0x7;
    GuestRegs.rdi = 0x8;
    GuestRegs.r8  = 0x9;
    GuestRegs.r9  = 0xa;
    GuestRegs.r10 = 0xb;
    GuestRegs.r11 = 0xc;
    GuestRegs.r12 = 0xd;
    GuestRegs.r13 = 0xe;
    GuestRegs.r14 = (UINT64)testw;
    GuestRegs.r15 = (UINT64)test;

    ScriptEngineEvalWrapper(&GuestRegs, Expr);

    free(RspReg);
    free(TestStruct);
    free(AllocationsForCastings.Buff1);
    free(AllocationsForCastings.Buff2);
    free(AllocationsForCastings.Buff3);
    free(AllocationsForCastings.Buff4);
    free(AllocationsForCastings.Buff5);
    free(AllocationsForCastings.Buff6);
}

/**
 * @brief test parser for hwdbg
 * @param Expr
 *
 * @return VOID
 */
VOID
ScriptEngineWrapperTestParserForHwdbg(const string & Expr)
{
    if (!g_HwdbgPinsStatus)
    {
        g_HwdbgPinsStatus = (UINT64 *)malloc(MAX_HWDBG_TESTING_PIN_COUNT * sizeof(UINT64));

        if (g_HwdbgPinsStatus == NULL)
        {
            ShowMessages("err, could not allocate memory for hwdbg pins status");

            return;
        }

        RtlZeroMemory(g_HwdbgPinsStatus, MAX_HWDBG_TESTING_PIN_COUNT * sizeof(UINT64));
    }

    ScriptEngineEvalWrapper((PGUEST_REGS)g_HwdbgPinsStatus, Expr);
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
ScriptEngineEvalUInt64StyleExpressionWrapper(const string & Expr, PBOOLEAN HasError)
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
