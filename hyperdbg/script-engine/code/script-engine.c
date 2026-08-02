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
#include "platform/user/header/platform-lib-calls.h"

#include <errno.h>
#include <locale.h>

// #define _SCRIPT_ENGINE_LALR_DBG_EN
// #define _SCRIPT_ENGINE_LL1_DBG_EN
// #define _SCRIPT_ENGINE_CODEGEN_DBG_EN

//
// Global Variables
//
extern HWDBG_INSTANCE_INFORMATION g_HwdbgInstanceInfo;
extern BOOLEAN                    g_HwdbgInstanceInfoIsValid;
extern PVOID                      g_MessageHandler;

typedef struct _STRUCT_DECLARATOR_STATE
{
    char *                            Name;
    unsigned int                      PointerDepth;
    unsigned int                      Dimensions[16];
    unsigned int                      DimensionCount;
    struct _STRUCT_DECLARATOR_STATE * Next;
} STRUCT_DECLARATOR_STATE, *PSTRUCT_DECLARATOR_STATE;

static PSTRUCT_DECLARATOR_STATE StructDeclarators;
static PSTRUCT_DECLARATOR_STATE StructDeclaratorsTail;
static unsigned int             StructPointerDepth;
static PVARIABLE_TYPE           CurrentStructDefinition;
static PSCRIPT_ENGINE_TOKEN     LastStructObject;
static PVARIABLE_TYPE           LastStructObjectType;

typedef struct _SIZEOF_COMPILATION_CONTEXT
{
    UINT32 CodePointer;
    UINT64 MaxTempNumber;
    CHAR   TempMap[MAX_TEMP_COUNT];
} SIZEOF_COMPILATION_CONTEXT;

static SIZEOF_COMPILATION_CONTEXT SizeofContexts[16];
static UINT32                     SizeofContextCount;

typedef struct _LOGICAL_COMPILATION_CONTEXT
{
    BOOLEAN              IsOr;
    UINT32               BeginJumpTargetIndex;
    PSCRIPT_ENGINE_TOKEN ResultToken;
} LOGICAL_COMPILATION_CONTEXT;

static LOGICAL_COMPILATION_CONTEXT LogicalContexts[16];
static UINT32                      LogicalContextCount;

static UINT64
GetScriptScalarTypeId(PVARIABLE_TYPE VariableType);

static PVARIABLE_TYPE
ResolveIdentifierVariableType(PSCRIPT_ENGINE_TOKEN Token)
{
    PVARIABLE_TYPE VariableType = Token ? (PVARIABLE_TYPE)Token->VariableType : NULL;

    if (!Token)
    {
        return NULL;
    }

    if (Token->Type == LOCAL_ID)
    {
        VariableType          = GetLocalIdentifierVariableType(Token);
        Token->IsImplicitType = GetLocalIdentifierIsImplicitType(Token);
    }
    else if (Token->Type == GLOBAL_ID)
    {
        VariableType          = GetGlobalIdentifierVariableType(Token);
        Token->IsImplicitType = GetGlobalIdentifierIsImplicitType(Token);
    }

    if (VariableType)
    {
        Token->VariableType = VariableType;
    }

    return VariableType;
}

static PVARIABLE_TYPE
ResolveTypeNameFromStack(PSCRIPT_ENGINE_TOKEN_LIST MatchedStack,
                         PSCRIPT_ENGINE_ERROR_TYPE Error)
{
    UINT32         PointerDepth = 0;
    PVARIABLE_TYPE BaseType;

    while (MatchedStack->Pointer && !strcmp(Top(MatchedStack)->Value, "@DECLARE_POINTER_TYPE"))
    {
        PSCRIPT_ENGINE_TOKEN PointerMarker = Pop(MatchedStack);
        RemoveToken(&PointerMarker);
        PointerDepth++;
    }

    if (!MatchedStack->Pointer)
    {
        *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
        return VARIABLE_TYPE_UNKNOWN;
    }

    if (Top(MatchedStack)->Type == SCRIPT_VARIABLE_TYPE)
    {
        BaseType = HandleType(MatchedStack);
    }
    else
    {
        PSCRIPT_ENGINE_TOKEN TagToken = Pop(MatchedStack);
        BaseType                      = FindStructType(TagToken->Value);
        RemoveToken(&TagToken);
    }

    // LALR reductions execute their semantic action after the complete RHS,
    // so abstract-declarator markers are below the reduced base-type token.
    // The LL(1) grammar emits the same markers above it.  Accept both stack
    // layouts while preserving one serialized type representation.
    while (MatchedStack->Pointer && !strcmp(Top(MatchedStack)->Value, "@DECLARE_POINTER_TYPE"))
    {
        PSCRIPT_ENGINE_TOKEN PointerMarker = Pop(MatchedStack);
        RemoveToken(&PointerMarker);
        PointerDepth++;
    }

    if (!BaseType || BaseType->Kind == TY_UNKNOWN)
    {
        *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
        return VARIABLE_TYPE_UNKNOWN;
    }

    while (PointerDepth--)
    {
        BaseType = CreatePointerType(BaseType);
        if (!BaseType)
        {
            *Error = SCRIPT_ENGINE_ERROR_TEMP_LIST_FULL;
            return VARIABLE_TYPE_UNKNOWN;
        }
    }
    if (MatchedStack->Pointer && !strcmp(Top(MatchedStack)->Value, "@TYPE_NAME_BEGIN"))
    {
        PSCRIPT_ENGINE_TOKEN TypeMarker = Pop(MatchedStack);
        RemoveToken(&TypeMarker);
    }
    return BaseType;
}

static PSCRIPT_ENGINE_TOKEN
EmitTruthValue(PSYMBOL_BUFFER            CodeBuffer,
               PSCRIPT_ENGINE_TOKEN      Operand,
               PSCRIPT_ENGINE_ERROR_TYPE Error)
{
    PVARIABLE_TYPE       OperandType = (PVARIABLE_TYPE)Operand->VariableType;
    UINT64               TypeId      = GetScriptScalarTypeId(OperandType);
    PSCRIPT_ENGINE_TOKEN FirstTemp;
    PSCRIPT_ENGINE_TOKEN ResultTemp;
    PSYMBOL              Symbol;
    PSYMBOL              OperandSymbol;
    PSYMBOL              FirstTempSymbol;
    PSYMBOL              ResultTempSymbol;

    if (TypeId == SCRIPT_SCALAR_TYPE_INVALID || TypeId == SCRIPT_SCALAR_TYPE_F80)
    {
        *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
        return NULL;
    }

    FirstTemp  = NewTemp(Error);
    ResultTemp = NewTemp(Error);
    if (!FirstTemp || !ResultTemp || *Error != SCRIPT_ENGINE_ERROR_FREE)
        return NULL;
    FirstTemp->VariableType  = VARIABLE_TYPE_INT;
    ResultTemp->VariableType = VARIABLE_TYPE_INT;
    OperandSymbol            = ToSymbol(Operand, Error);
    FirstTempSymbol          = ToSymbol(FirstTemp, Error);
    ResultTempSymbol         = ToSymbol(ResultTemp, Error);
    Symbol                   = NewSymbol();

    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
    Symbol->Value = FUNC_LOGICAL_NOT_TYPED;
    PushSymbol(CodeBuffer, Symbol);
    PushSymbol(CodeBuffer, OperandSymbol);
    PushSymbol(CodeBuffer, FirstTempSymbol);
    Symbol->Type  = SYMBOL_NUM_TYPE;
    Symbol->Value = TypeId;
    PushSymbol(CodeBuffer, Symbol);

    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
    Symbol->Value = FUNC_LOGICAL_NOT_TYPED;
    PushSymbol(CodeBuffer, Symbol);
    PushSymbol(CodeBuffer, FirstTempSymbol);
    PushSymbol(CodeBuffer, ResultTempSymbol);
    Symbol->Type  = SYMBOL_NUM_TYPE;
    Symbol->Value = SCRIPT_SCALAR_TYPE_I32;
    PushSymbol(CodeBuffer, Symbol);
    RemoveSymbol(&Symbol);
    FreeTemp(FirstTemp);
    RemoveToken(&FirstTemp);
    return ResultTemp;
}

static PVARIABLE_TYPE
GetUnsignedTypeForAccessWidth(UINT32 AccessWidth)
{
    if (AccessWidth == 1)
        return VARIABLE_TYPE_UCHAR;
    if (AccessWidth == 2)
        return VARIABLE_TYPE_USHORT;
    if (AccessWidth == 4)
        return VARIABLE_TYPE_UINT;
    if (AccessWidth == 8)
        return VARIABLE_TYPE_ULLONG;
    return VARIABLE_TYPE_UNKNOWN;
}

static PSCRIPT_ENGINE_TOKEN
EmitTypedScalarLoad(PSYMBOL_BUFFER            CodeBuffer,
                    PSCRIPT_ENGINE_TOKEN      AddressToken,
                    UINT32                    AddressSpace,
                    PVARIABLE_TYPE            DeclaredType,
                    PSCRIPT_ENGINE_ERROR_TYPE Error)
{
    PVARIABLE_TYPE       RawType;
    PSCRIPT_ENGINE_TOKEN RawTemp = NULL;
    PSCRIPT_ENGINE_TOKEN ValueTemp;
    PSCRIPT_ENGINE_TOKEN LoadDestination;
    PSYMBOL              Symbol;
    BOOLEAN              NeedsIntegerNormalization;

    RawType = GetUnsignedTypeForAccessWidth((UINT32)DeclaredType->Size);
    if (RawType == VARIABLE_TYPE_UNKNOWN)
    {
        *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
        return NULL;
    }

    ValueTemp = NewTemp(Error);
    if (!ValueTemp || *Error != SCRIPT_ENGINE_ERROR_FREE)
        return NULL;
    ValueTemp->VariableType   = DeclaredType;
    NeedsIntegerNormalization = IsIntegerVariableType(DeclaredType) &&
                                GetScriptScalarTypeId(RawType) != GetScriptScalarTypeId(DeclaredType);
    if (NeedsIntegerNormalization)
    {
        RawTemp = NewTemp(Error);
        if (!RawTemp || *Error != SCRIPT_ENGINE_ERROR_FREE)
        {
            FreeTemp(ValueTemp);
            RemoveToken(&ValueTemp);
            return NULL;
        }
        RawTemp->VariableType = RawType;
    }
    LoadDestination = RawTemp ? RawTemp : ValueTemp;

    Symbol        = NewSymbol();
    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
    Symbol->Value = FUNC_TYPED_LOAD;
    PushSymbol(CodeBuffer, Symbol);
    RemoveSymbol(&Symbol);
    Symbol = ToSymbol(AddressToken, Error);
    PushSymbol(CodeBuffer, Symbol);
    RemoveSymbol(&Symbol);
    Symbol        = NewSymbol();
    Symbol->Type  = SYMBOL_NUM_TYPE;
    Symbol->Value = AddressSpace;
    PushSymbol(CodeBuffer, Symbol);
    Symbol->Value = DeclaredType->Size;
    PushSymbol(CodeBuffer, Symbol);
    RemoveSymbol(&Symbol);
    Symbol = ToSymbol(LoadDestination, Error);
    PushSymbol(CodeBuffer, Symbol);
    RemoveSymbol(&Symbol);

    if (RawTemp)
    {
        Symbol        = NewSymbol();
        Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
        Symbol->Value = FUNC_CAST_SCALAR;
        PushSymbol(CodeBuffer, Symbol);
        RemoveSymbol(&Symbol);
        Symbol = ToSymbol(RawTemp, Error);
        PushSymbol(CodeBuffer, Symbol);
        RemoveSymbol(&Symbol);
        Symbol = ToSymbol(ValueTemp, Error);
        PushSymbol(CodeBuffer, Symbol);
        RemoveSymbol(&Symbol);
        Symbol        = NewSymbol();
        Symbol->Type  = SYMBOL_NUM_TYPE;
        Symbol->Value = GetScriptScalarTypeId(RawType);
        PushSymbol(CodeBuffer, Symbol);
        Symbol->Value = GetScriptScalarTypeId(DeclaredType);
        PushSymbol(CodeBuffer, Symbol);
        RemoveSymbol(&Symbol);
        FreeTemp(RawTemp);
        RemoveToken(&RawTemp);
    }

    return ValueTemp;
}

static UINT64
GetFloatingValueKind(PVARIABLE_TYPE VariableType)
{
    if (VariableType && VariableType->Kind == TY_FLOAT)
    {
        return SYMBOL_VALUE_KIND_FLOAT32;
    }

    if (VariableType && VariableType->Kind == TY_DOUBLE)
    {
        return SYMBOL_VALUE_KIND_FLOAT64;
    }

    return SYMBOL_VALUE_KIND_INTEGER;
}

static BOOLEAN
IsFloatingVariableType(PVARIABLE_TYPE VariableType)
{
    return VariableType && (VariableType->Kind == TY_FLOAT || VariableType->Kind == TY_DOUBLE);
}

static UINT64
GetScriptScalarTypeId(PVARIABLE_TYPE VariableType)
{
    if (!VariableType)
        return SCRIPT_SCALAR_TYPE_INVALID;
    if (VariableType->Kind == TY_BOOL)
        return SCRIPT_SCALAR_TYPE_BOOL;
    if (VariableType->Kind == TY_CHAR)
        return VariableType->IsUnsigned ? SCRIPT_SCALAR_TYPE_U8 : SCRIPT_SCALAR_TYPE_I8;
    if (VariableType->Kind == TY_SHORT)
        return VariableType->IsUnsigned ? SCRIPT_SCALAR_TYPE_U16 : SCRIPT_SCALAR_TYPE_I16;
    if (VariableType->Kind == TY_INT || VariableType->Kind == TY_ENUM)
        return VariableType->IsUnsigned ? SCRIPT_SCALAR_TYPE_U32 : SCRIPT_SCALAR_TYPE_I32;
    if (VariableType->Kind == TY_LONG || VariableType->Kind == TY_LLONG)
        return VariableType->IsUnsigned ? SCRIPT_SCALAR_TYPE_U64 : SCRIPT_SCALAR_TYPE_I64;
    if (VariableType->Kind == TY_FLOAT)
        return SCRIPT_SCALAR_TYPE_F32;
    if (VariableType->Kind == TY_DOUBLE)
        return SCRIPT_SCALAR_TYPE_F64;
    if (VariableType->Kind == TY_PTR)
        return SCRIPT_SCALAR_TYPE_POINTER;
    if (VariableType->Kind == TY_LDOUBLE)
        return SCRIPT_SCALAR_TYPE_F80;
    return SCRIPT_SCALAR_TYPE_INVALID;
}

static UINT64
GetTypedBinaryOpcode(const CHAR * Operator)
{
    if (!strcmp(Operator, "@ADD"))
        return FUNC_ADD_TYPED;
    if (!strcmp(Operator, "@SUB"))
        return FUNC_SUB_TYPED;
    if (!strcmp(Operator, "@MUL"))
        return FUNC_MUL_TYPED;
    if (!strcmp(Operator, "@DIV"))
        return FUNC_DIV_TYPED;
    if (!strcmp(Operator, "@MOD"))
        return FUNC_MOD_TYPED;
    if (!strcmp(Operator, "@AND"))
        return FUNC_BITWISE_AND_TYPED;
    if (!strcmp(Operator, "@OR"))
        return FUNC_BITWISE_OR_TYPED;
    if (!strcmp(Operator, "@XOR"))
        return FUNC_BITWISE_XOR_TYPED;
    if (!strcmp(Operator, "@ASL"))
        return FUNC_SHIFT_LEFT_TYPED;
    if (!strcmp(Operator, "@ASR"))
        return FUNC_SHIFT_RIGHT_TYPED;
    if (!strcmp(Operator, "@GT"))
        return FUNC_GT_TYPED;
    if (!strcmp(Operator, "@LT"))
        return FUNC_LT_TYPED;
    if (!strcmp(Operator, "@EGT"))
        return FUNC_EGT_TYPED;
    if (!strcmp(Operator, "@ELT"))
        return FUNC_ELT_TYPED;
    if (!strcmp(Operator, "@EQUAL"))
        return FUNC_EQUAL_TYPED;
    if (!strcmp(Operator, "@NEQ"))
        return FUNC_NEQ_TYPED;
    return FUNC_UNDEFINED;
}

static UINT64
GetTypedAssignmentOpcode(const CHAR * Operator)
{
    if (!strcmp(Operator, "@ADD_ASSIGNMENT"))
        return FUNC_ADD_TYPED;
    if (!strcmp(Operator, "@SUB_ASSIGNMENT"))
        return FUNC_SUB_TYPED;
    if (!strcmp(Operator, "@MUL_ASSIGNMENT"))
        return FUNC_MUL_TYPED;
    if (!strcmp(Operator, "@DIV_ASSIGNMENT"))
        return FUNC_DIV_TYPED;
    if (!strcmp(Operator, "@MOD_ASSIGNMENT"))
        return FUNC_MOD_TYPED;
    if (!strcmp(Operator, "@AND_ASSIGNMENT"))
        return FUNC_BITWISE_AND_TYPED;
    if (!strcmp(Operator, "@OR_ASSIGNMENT"))
        return FUNC_BITWISE_OR_TYPED;
    if (!strcmp(Operator, "@XOR_ASSIGNMENT"))
        return FUNC_BITWISE_XOR_TYPED;
    if (!strcmp(Operator, "@ASL_ASSIGNMENT"))
        return FUNC_SHIFT_LEFT_TYPED;
    if (!strcmp(Operator, "@ASR_ASSIGNMENT"))
        return FUNC_SHIFT_RIGHT_TYPED;
    return FUNC_UNDEFINED;
}

static BOOLEAN
IsFloatingComparisonOperator(const CHAR * Operator)
{
    return !strcmp(Operator, "@GT") || !strcmp(Operator, "@LT") ||
           !strcmp(Operator, "@EGT") || !strcmp(Operator, "@ELT") ||
           !strcmp(Operator, "@EQUAL") || !strcmp(Operator, "@NEQ");
}

static UINT64
GetFloatingBinaryOpcode(const CHAR * Operator)
{
    if (!strcmp(Operator, "@ADD"))
    {
        return FUNC_ADD_FLOAT;
    }
    if (!strcmp(Operator, "@SUB"))
    {
        return FUNC_SUB_FLOAT;
    }
    if (!strcmp(Operator, "@MUL"))
    {
        return FUNC_MUL_FLOAT;
    }
    if (!strcmp(Operator, "@DIV"))
    {
        return FUNC_DIV_FLOAT;
    }
    if (!strcmp(Operator, "@GT"))
    {
        return FUNC_GT_FLOAT;
    }
    if (!strcmp(Operator, "@LT"))
    {
        return FUNC_LT_FLOAT;
    }
    if (!strcmp(Operator, "@EGT"))
    {
        return FUNC_EGT_FLOAT;
    }
    if (!strcmp(Operator, "@ELT"))
    {
        return FUNC_ELT_FLOAT;
    }
    if (!strcmp(Operator, "@EQUAL"))
    {
        return FUNC_EQUAL_FLOAT;
    }
    if (!strcmp(Operator, "@NEQ"))
    {
        return FUNC_NEQ_FLOAT;
    }
    return 0;
}

static BOOLEAN
FloatingLiteralHasNonzeroDigit(const CHAR * Text)
{
    while (*Text)
    {
        if (*Text >= '1' && *Text <= '9')
        {
            return TRUE;
        }
        Text++;
    }
    return FALSE;
}

static BOOLEAN
ParseFloatingLiteral(const CHAR * Text, UINT64 ValueKind, PUINT64 RawBits, PSCRIPT_ENGINE_ERROR_TYPE Error)
{
    CHAR * End = NULL;

    if (!Text || !RawBits || (ValueKind != SYMBOL_VALUE_KIND_FLOAT32 && ValueKind != SYMBOL_VALUE_KIND_FLOAT64))
    {
        *Error = SCRIPT_ENGINE_ERROR_INVALID_FLOAT_LITERAL;
        return FALSE;
    }

#ifdef _WIN32
    _locale_t Locale = _create_locale(LC_NUMERIC, "C");
    if (!Locale)
    {
        *Error = SCRIPT_ENGINE_ERROR_INVALID_FLOAT_LITERAL;
        return FALSE;
    }

    if (ValueKind == SYMBOL_VALUE_KIND_FLOAT32)
    {
        float  Value = _strtof_l(Text, &End, Locale);
        UINT32 Bits  = 0;
        memcpy(&Bits, &Value, sizeof(Bits));
        *RawBits = Bits;
    }
    else
    {
        double Value = _strtod_l(Text, &End, Locale);
        memcpy(RawBits, &Value, sizeof(Value));
    }
    _free_locale(Locale);
#else
    locale_t Locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
    if (!Locale)
    {
        *Error = SCRIPT_ENGINE_ERROR_INVALID_FLOAT_LITERAL;
        return FALSE;
    }

    if (ValueKind == SYMBOL_VALUE_KIND_FLOAT32)
    {
        float  Value = strtof_l(Text, &End, Locale);
        UINT32 Bits  = 0;
        memcpy(&Bits, &Value, sizeof(Bits));
        *RawBits = Bits;
    }
    else
    {
        double Value = strtod_l(Text, &End, Locale);
        memcpy(RawBits, &Value, sizeof(Value));
    }
    freelocale(Locale);
#endif

    if (End == Text || *End != '\0')
    {
        *Error = SCRIPT_ENGINE_ERROR_INVALID_FLOAT_LITERAL;
        return FALSE;
    }

    if ((ValueKind == SYMBOL_VALUE_KIND_FLOAT32 && ((*RawBits & 0x7f800000ULL) == 0x7f800000ULL)) ||
        (ValueKind == SYMBOL_VALUE_KIND_FLOAT64 && ((*RawBits & 0x7ff0000000000000ULL) == 0x7ff0000000000000ULL)) ||
        ((*RawBits & (ValueKind == SYMBOL_VALUE_KIND_FLOAT32 ? 0x7fffffffULL : 0x7fffffffffffffffULL)) == 0 &&
         FloatingLiteralHasNonzeroDigit(Text)))
    {
        *Error = SCRIPT_ENGINE_ERROR_FLOAT_OUT_OF_RANGE;
        return FALSE;
    }

    return TRUE;
}

static unsigned int
GetStructPointerAddressSpace(PVARIABLE_TYPE PointerType, PSCRIPT_ENGINE_TOKEN PointerToken)
{
    if (PointerToken && PointerToken->AddressSpace)
    {
        return PointerToken->AddressSpace;
    }

    if (PointerType && PointerType->PointerProvenance == POINTER_PROVENANCE_LOCAL)
    {
        return SCRIPT_ENGINE_ADDRESS_SPACE_LOCAL;
    }

    return SCRIPT_ENGINE_ADDRESS_SPACE_REMOTE;
}

static PVARIABLE_TYPE
CreateStructPointerType(PVARIABLE_TYPE BaseType, POINTER_PROVENANCE Provenance)
{
    PVARIABLE_TYPE PointerType = CreatePointerType(BaseType);
    if (PointerType)
    {
        PointerType->PointerProvenance = Provenance;
    }
    return PointerType;
}

static VOID
ResetStructDeclarators(VOID)
{
    while (StructDeclarators)
    {
        PSTRUCT_DECLARATOR_STATE Next = StructDeclarators->Next;
        free(StructDeclarators->Name);
        free(StructDeclarators);
        StructDeclarators = Next;
    }
    StructDeclaratorsTail = NULL;
    StructPointerDepth    = 0;
}

static PVARIABLE_TYPE
ApplyStructDeclarator(PVARIABLE_TYPE BaseType, PSTRUCT_DECLARATOR_STATE Declarator)
{
    PVARIABLE_TYPE Type = BaseType;
    unsigned int   Index;

    for (Index = 0; Index < Declarator->PointerDepth; Index++)
    {
        Type = CreatePointerType(Type);
        if (!Type)
        {
            return NULL;
        }
    }

    for (Index = Declarator->DimensionCount; Index > 0; Index--)
    {
        Type = CreateArrayType(Type, Declarator->Dimensions[Index - 1]);
        if (!Type)
        {
            return NULL;
        }
    }
    return Type;
}

static PVARIABLE_TYPE
PopStructBaseType(PSCRIPT_ENGINE_TOKEN_LIST MatchedStack, PSCRIPT_ENGINE_ERROR_TYPE Error)
{
    PVARIABLE_TYPE       Type;
    PSCRIPT_ENGINE_TOKEN TagToken;

    if (MatchedStack->Pointer && Top(MatchedStack)->Type == SCRIPT_VARIABLE_TYPE)
    {
        Type = HandleType(MatchedStack);
        if (Type->Kind == TY_UNKNOWN)
        {
            *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
            return NULL;
        }
        return Type;
    }

    if (!MatchedStack->Pointer)
    {
        *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
        return NULL;
    }

    TagToken = Pop(MatchedStack);
    Type     = FindStructType(TagToken->Value);
    RemoveToken(&TagToken);
    if (!Type)
    {
        *Error = SCRIPT_ENGINE_ERROR_UNKNOWN_STRUCT_TAG;
    }
    return Type;
}

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
        INT SprintfResult = PlatformVsnprintf(TempMessage, COMMUNICATION_BUFFER_SIZE + TCP_END_OF_BUFFER_CHARS_COUNT, Fmt, ArgList);
        va_end(ArgList);

        if (SprintfResult != -1)
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
ScriptEngineConvertFileToPdbPath(const char * LocalFilePath, char * ResultPath, SIZE_T ResultPathSize)
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
 * @brief Convert loaded module bytes to pdb attributes for symbols
 *
 * @param LoadedImageBytes
 * @param LoadedImageSize
 * @param LocalFilePath
 * @param PdbFilePath
 * @param GuidAndAgeDetails
 * @param Is32BitModule
 *
 * @return BOOLEAN
 */
BOOLEAN
ScriptEngineConvertLoadedModuleToPdbFileAndGuidAndAgeDetails(const BYTE * LoadedImageBytes,
                                                             SIZE_T       LoadedImageSize,
                                                             const char * LocalFilePath,
                                                             char *       PdbFilePath,
                                                             char *       GuidAndAgeDetails,
                                                             BOOLEAN      Is32BitModule)
{
    //
    // A wrapper for loaded module pdb to path file and guid and age detail converter
    //
    return SymConvertLoadedModuleToPdbFileAndGuidAndAgeDetails(
        LoadedImageBytes,
        LoadedImageSize,
        LocalFilePath,
        PdbFilePath,
        GuidAndAgeDetails,
        Is32BitModule);
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
    char * ScriptSource = PlatformStrDup(str);

    InitializeTypeContext();
    ResetStructDeclarators();
    CurrentStructDefinition = NULL;
    LastStructObject        = NULL;
    LastStructObjectType    = NULL;
    SizeofContextCount      = 0;
    LogicalContextCount     = 0;

    PSCRIPT_ENGINE_TOKEN_LIST Stack        = NewTokenList();
    PSCRIPT_ENGINE_TOKEN_LIST MatchedStack = NewTokenList();
    PSYMBOL_BUFFER            CodeBuffer   = NewSymbolBuffer();

    UserDefinedFunctionHead = malloc(sizeof(USER_DEFINED_FUNCTION_NODE));
    PlatformZeroMemory(UserDefinedFunctionHead, sizeof(USER_DEFINED_FUNCTION_NODE));
    UserDefinedFunctionHead->Name                     = PlatformStrDup("main");
    UserDefinedFunctionHead->IdTable                  = (unsigned long long)NewTokenList();
    UserDefinedFunctionHead->FunctionParameterIdTable = (unsigned long long)NewTokenList();
    UserDefinedFunctionHead->TempMap                  = calloc(MAX_TEMP_COUNT, 1);
    UserDefinedFunctionHead->VariableType             = (unsigned long long)VARIABLE_TYPE_VOID;

    CurrentUserDefinedFunction = UserDefinedFunctionHead;

    SCRIPT_ENGINE_ERROR_TYPE Error        = SCRIPT_ENGINE_ERROR_FREE;
    char *                   ErrorMessage = NULL;

    static INT FirstCall = 1;
    if (FirstCall)
    {
        GlobalIdTable = NewTokenList();
        FirstCall     = 0;
    }

    PSCRIPT_ENGINE_TOKEN TopToken = NewUnknownToken();

    int  NonTerminalId;
    int  TerminalId;
    int  RuleId;
    CHAR C;
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

    C = sgetc(ScriptSource);

    PSCRIPT_ENGINE_TOKEN CurrentIn = Scan(ScriptSource, &C);
    if (CurrentIn->Type == UNKNOWN)
    {
        Error               = SCRIPT_ENGINE_ERROR_SYNTAX;
        ErrorMessage        = HandleError(&Error, ScriptSource);
        CodeBuffer->Message = ErrorMessage;

        RemoveTokenList(Stack);
        RemoveTokenList(MatchedStack);
        RemoveToken(&CurrentIn);
        UninitializeTypeContext();
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
                UINT64 BooleanExpressionSize = BooleanExpressionExtractEnd(ScriptSource, &WaitForWaitStatementBooleanExpression, CurrentIn);

                ScriptEngineBooleanExpresssionParse(BooleanExpressionSize, CurrentIn, MatchedStack, CodeBuffer, ScriptSource, &C, &Error);
                if (Error != SCRIPT_ENGINE_ERROR_FREE)
                {
                    break;
                }

                RemoveToken(&CurrentIn);
                CurrentIn = Scan(ScriptSource, &C);
                if (CurrentIn->Type == UNKNOWN)
                {
                    Error = SCRIPT_ENGINE_ERROR_UNKNOWN_TOKEN;
                    break;
                }

                RemoveToken(&CurrentIn);
                CurrentIn = Scan(ScriptSource, &C);
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

                CurrentIn = Scan(ScriptSource, &C);
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
                CodeGen(MatchedStack, CodeBuffer, TopToken, &Error, &ScriptSource);
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
                CurrentIn = Scan(ScriptSource, &C);

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
        ErrorMessage = HandleError(&Error, ScriptSource);
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

    if (IncludeHead)
    {
        PINCLUDE_NODE Node = IncludeHead;
        while (Node)
        {
            if (Node->FilePath)
                free(Node->FilePath);

            PINCLUDE_NODE Temp = Node;
            Node               = Node->NextNode;
            free(Temp);
        }
        IncludeHead = 0;
    }

    if (CurrentIn)
        RemoveToken(&CurrentIn);

    if (TopToken)
        RemoveToken(&TopToken);

    ResetStructDeclarators();
    if (LastStructObject)
        RemoveToken(&LastStructObject);
    UninitializeTypeContext();
    free(ScriptSource);

    return (PVOID)CodeBuffer;
}

/**
 * @brief Script Engine code generator
 *
 * @param MatchedStack
 * @param CodeBuffer
 * @param Operator
 * @param Error
 * @param ScriptSource the script source string
 * @return VOID
 */
void
CodeGen(PSCRIPT_ENGINE_TOKEN_LIST MatchedStack, PSYMBOL_BUFFER CodeBuffer, PSCRIPT_ENGINE_TOKEN Operator, PSCRIPT_ENGINE_ERROR_TYPE Error, char ** ScriptSource)
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
        if (!strcmp(Operator->Value, "@TYPE_NAME_BEGIN"))
        {
            Push(MatchedStack, CopyToken(Operator));
        }
        else if (!strcmp(Operator->Value, "@SIZEOF_BEGIN"))
        {
            if (SizeofContextCount >= 16)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }
            SizeofContexts[SizeofContextCount].CodePointer   = CodeBuffer->Pointer;
            SizeofContexts[SizeofContextCount].MaxTempNumber = CurrentUserDefinedFunction->MaxTempNumber;
            memcpy(SizeofContexts[SizeofContextCount].TempMap,
                   CurrentUserDefinedFunction->TempMap,
                   MAX_TEMP_COUNT);
            SizeofContextCount++;
        }
        else if (!strcmp(Operator->Value, "@SIZEOF_EXPRESSION"))
        {
            CHAR                 SizeText[32];
            PVARIABLE_TYPE       OperandType;
            PSCRIPT_ENGINE_TOKEN SizeToken;
            if (!SizeofContextCount || !MatchedStack->Pointer)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }
            Op0         = Pop(MatchedStack);
            OperandType = (PVARIABLE_TYPE)Op0->VariableType;
            SizeofContextCount--;
            CodeBuffer->Pointer                       = SizeofContexts[SizeofContextCount].CodePointer;
            CurrentUserDefinedFunction->MaxTempNumber = SizeofContexts[SizeofContextCount].MaxTempNumber;
            memcpy(CurrentUserDefinedFunction->TempMap,
                   SizeofContexts[SizeofContextCount].TempMap,
                   MAX_TEMP_COUNT);
            if (!OperandType || OperandType->Kind == TY_VOID || OperandType->Kind == TY_FUNC ||
                (OperandType->Kind == TY_STRUCT && !OperandType->IsComplete) || OperandType->Size <= 0)
            {
                RemoveToken(&Op0);
                *Error = SCRIPT_ENGINE_ERROR_INCOMPLETE_TYPE;
                break;
            }
            PlatformSnprintf(SizeText, sizeof(SizeText), "%d", OperandType->Size);
            SizeToken               = NewToken(DECIMAL, SizeText);
            SizeToken->VariableType = VARIABLE_TYPE_ULLONG;
            RemoveToken(&Op0);
            Push(MatchedStack, SizeToken);
        }
        else if (!strcmp(Operator->Value, "@SIZEOF_TYPE"))
        {
            CHAR                 SizeText[32];
            PSCRIPT_ENGINE_TOKEN SizeToken;
            PVARIABLE_TYPE       ResolvedType = ResolveTypeNameFromStack(MatchedStack, Error);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
                break;
            if (ResolvedType->Kind == TY_VOID || ResolvedType->Kind == TY_FUNC ||
                (ResolvedType->Kind == TY_STRUCT && !ResolvedType->IsComplete) || ResolvedType->Size <= 0)
            {
                *Error = SCRIPT_ENGINE_ERROR_INCOMPLETE_TYPE;
                break;
            }
            PlatformSnprintf(SizeText, sizeof(SizeText), "%d", ResolvedType->Size);
            SizeToken               = NewToken(DECIMAL, SizeText);
            SizeToken->VariableType = VARIABLE_TYPE_ULLONG;
            Push(MatchedStack, SizeToken);
        }
        else if (!strcmp(Operator->Value, "@CAST_SCALAR"))
        {
            PVARIABLE_TYPE SourceType;
            PVARIABLE_TYPE DestinationType;
            UINT64         SourceTypeId;
            UINT64         DestinationTypeId;
            PSYMBOL        TypeSymbol;

            Op0               = Pop(MatchedStack);
            DestinationType   = ResolveTypeNameFromStack(MatchedStack, Error);
            SourceType        = (PVARIABLE_TYPE)Op0->VariableType;
            SourceTypeId      = GetScriptScalarTypeId(SourceType);
            DestinationTypeId = GetScriptScalarTypeId(DestinationType);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE || SourceTypeId == SCRIPT_SCALAR_TYPE_INVALID ||
                DestinationTypeId == SCRIPT_SCALAR_TYPE_INVALID || SourceTypeId == SCRIPT_SCALAR_TYPE_F80 ||
                DestinationTypeId == SCRIPT_SCALAR_TYPE_F80 ||
                ((SourceTypeId == SCRIPT_SCALAR_TYPE_POINTER || DestinationTypeId == SCRIPT_SCALAR_TYPE_POINTER) &&
                 (IsFloatingVariableType(SourceType) || IsFloatingVariableType(DestinationType))))
            {
                RemoveToken(&Op0);
                *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                break;
            }

            if (DestinationType->Kind == TY_PTR)
            {
                DestinationType->PointerProvenance = SourceType->Kind == TY_PTR ? SourceType->PointerProvenance : POINTER_PROVENANCE_REMOTE;
            }

            Op0Symbol = ToSymbol(Op0, Error);
            if (Op0->Type == FLOAT_LITERAL)
            {
                UINT64 SourceKind = GetFloatingValueKind(SourceType);
                if (!ParseFloatingLiteral(Op0->Value, SourceKind, &Op0Symbol->Value, Error))
                {
                    RemoveToken(&Op0);
                    break;
                }
                Op0Symbol->Len = SourceKind;
            }
            Temp                  = NewTemp(Error);
            Temp->VariableType    = DestinationType;
            TempSymbol            = ToSymbol(Temp, Error);
            TempSymbol->Len       = GetFloatingValueKind(DestinationType);
            OperatorSymbol->Value = FUNC_CAST_SCALAR;
            PushSymbol(CodeBuffer, OperatorSymbol);
            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, TempSymbol);
            TypeSymbol        = NewSymbol();
            TypeSymbol->Type  = SYMBOL_NUM_TYPE;
            TypeSymbol->Value = SourceTypeId;
            PushSymbol(CodeBuffer, TypeSymbol);
            TypeSymbol->Value = DestinationTypeId;
            PushSymbol(CodeBuffer, TypeSymbol);
            RemoveSymbol(&TypeSymbol);
            FreeTemp(Op0);
            RemoveToken(&Op0);
            Push(MatchedStack, Temp);
        }
        else if (!strcmp(Operator->Value, "@LOGICAL_NOT_TYPED"))
        {
            PSYMBOL TypeSymbol;
            Op0          = Pop(MatchedStack);
            VariableType = (PVARIABLE_TYPE)Op0->VariableType;
            if (GetScriptScalarTypeId(VariableType) == SCRIPT_SCALAR_TYPE_INVALID || VariableType->Kind == TY_LDOUBLE)
            {
                RemoveToken(&Op0);
                *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                break;
            }
            Op0Symbol             = ToSymbol(Op0, Error);
            Temp                  = NewTemp(Error);
            Temp->VariableType    = VARIABLE_TYPE_INT;
            TempSymbol            = ToSymbol(Temp, Error);
            OperatorSymbol->Value = FUNC_LOGICAL_NOT_TYPED;
            PushSymbol(CodeBuffer, OperatorSymbol);
            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, TempSymbol);
            TypeSymbol        = NewSymbol();
            TypeSymbol->Type  = SYMBOL_NUM_TYPE;
            TypeSymbol->Value = GetScriptScalarTypeId(VariableType);
            PushSymbol(CodeBuffer, TypeSymbol);
            RemoveSymbol(&TypeSymbol);
            FreeTemp(Op0);
            RemoveToken(&Op0);
            Push(MatchedStack, Temp);
        }
        else if (!strcmp(Operator->Value, "@LOGICAL_OR_BEGIN") ||
                 !strcmp(Operator->Value, "@LOGICAL_AND_BEGIN"))
        {
            PSCRIPT_ENGINE_TOKEN TruthToken;
            PSYMBOL              TruthSymbol;
            PSYMBOL              ResultSymbol;
            PSYMBOL              Symbol;
            if (!MatchedStack->Pointer || LogicalContextCount >= 16)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }
            TruthToken = EmitTruthValue(CodeBuffer, Top(MatchedStack), Error);
            if (!TruthToken || *Error != SCRIPT_ENGINE_ERROR_FREE)
                break;
            TruthSymbol                                                    = ToSymbol(TruthToken, Error);
            LogicalContexts[LogicalContextCount].IsOr                      = !strcmp(Operator->Value, "@LOGICAL_OR_BEGIN");
            LogicalContexts[LogicalContextCount].ResultToken               = NewTemp(Error);
            LogicalContexts[LogicalContextCount].ResultToken->VariableType = VARIABLE_TYPE_INT;
            ResultSymbol                                                   = ToSymbol(LogicalContexts[LogicalContextCount].ResultToken, Error);
            Symbol                                                         = NewSymbol();

            Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            Symbol->Value = FUNC_MOV;
            PushSymbol(CodeBuffer, Symbol);
            Symbol->Type  = SYMBOL_NUM_TYPE;
            Symbol->Value = LogicalContexts[LogicalContextCount].IsOr ? 1 : 0;
            PushSymbol(CodeBuffer, Symbol);
            PushSymbol(CodeBuffer, ResultSymbol);

            Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            Symbol->Value = LogicalContexts[LogicalContextCount].IsOr ? FUNC_JNZ : FUNC_JZ;
            PushSymbol(CodeBuffer, Symbol);
            LogicalContexts[LogicalContextCount].BeginJumpTargetIndex = CodeBuffer->Pointer;
            Symbol->Type                                              = SYMBOL_NUM_TYPE;
            Symbol->Value                                             = 0;
            PushSymbol(CodeBuffer, Symbol);
            PushSymbol(CodeBuffer, TruthSymbol);
            RemoveSymbol(&Symbol);
            FreeTemp(TruthToken);
            RemoveToken(&TruthToken);
            LogicalContextCount++;
        }
        else if (!strcmp(Operator->Value, "@LOGICAL_OR_END") ||
                 !strcmp(Operator->Value, "@LOGICAL_AND_END"))
        {
            LOGICAL_COMPILATION_CONTEXT * Context;
            PSCRIPT_ENGINE_TOKEN          TruthToken;
            PSYMBOL                       TruthSymbol;
            PSYMBOL                       ResultSymbol;
            PSYMBOL                       Symbol;
            UINT32                        EndJumpTargetIndex;
            if (!LogicalContextCount || MatchedStack->Pointer < 2)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }
            LogicalContextCount--;
            Context    = &LogicalContexts[LogicalContextCount];
            Op0        = Pop(MatchedStack);
            Op1        = Pop(MatchedStack);
            TruthToken = EmitTruthValue(CodeBuffer, Op0, Error);
            if (!TruthToken || *Error != SCRIPT_ENGINE_ERROR_FREE)
                break;
            TruthSymbol  = ToSymbol(TruthToken, Error);
            ResultSymbol = ToSymbol(Context->ResultToken, Error);
            Symbol       = NewSymbol();

            Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            Symbol->Value = Context->IsOr ? FUNC_JNZ : FUNC_JZ;
            PushSymbol(CodeBuffer, Symbol);
            EndJumpTargetIndex = CodeBuffer->Pointer;
            Symbol->Type       = SYMBOL_NUM_TYPE;
            Symbol->Value      = 0;
            PushSymbol(CodeBuffer, Symbol);
            PushSymbol(CodeBuffer, TruthSymbol);

            Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            Symbol->Value = FUNC_MOV;
            PushSymbol(CodeBuffer, Symbol);
            Symbol->Type  = SYMBOL_NUM_TYPE;
            Symbol->Value = Context->IsOr ? 0 : 1;
            PushSymbol(CodeBuffer, Symbol);
            PushSymbol(CodeBuffer, ResultSymbol);
            RemoveSymbol(&Symbol);

            CodeBuffer->Head[Context->BeginJumpTargetIndex].Value = CodeBuffer->Pointer;
            CodeBuffer->Head[EndJumpTargetIndex].Value            = CodeBuffer->Pointer;
            FreeTemp(TruthToken);
            RemoveToken(&TruthToken);
            FreeTemp(Op0);
            FreeTemp(Op1);
            RemoveToken(&Op0);
            RemoveToken(&Op1);
            Push(MatchedStack, Context->ResultToken);
        }
        else if (!strcmp(Operator->Value, "@STRUCT_POINTER"))
        {
            StructPointerDepth++;
        }
        else if (!strcmp(Operator->Value, "@STRUCT_ARRAY_DIMENSION"))
        {
            unsigned long long Dimension;
            Op0       = Pop(MatchedStack);
            Dimension = strtoull(Op0->Value, NULL, 0);
            RemoveToken(&Op0);
            if (!Dimension || Dimension > UINT32_MAX)
            {
                *Error = SCRIPT_ENGINE_ERROR_INVALID_ARRAY_SIZE;
                break;
            }
            if (!StructDeclaratorsTail || StructDeclaratorsTail->DimensionCount >= 16)
            {
                *Error = SCRIPT_ENGINE_ERROR_INVALID_ARRAY_SIZE;
                break;
            }
            StructDeclaratorsTail->Dimensions[StructDeclaratorsTail->DimensionCount++] = (unsigned int)Dimension;
        }
        else if (!strcmp(Operator->Value, "@STRUCT_DECLARATOR_COMPLETE"))
        {
            PSTRUCT_DECLARATOR_STATE Declarator;
            Op0        = Pop(MatchedStack);
            Declarator = (PSTRUCT_DECLARATOR_STATE)calloc(1, sizeof(STRUCT_DECLARATOR_STATE));
            if (!Declarator)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }
            Declarator->Name         = PlatformStrDup(Op0->Value);
            Declarator->PointerDepth = StructPointerDepth;
            RemoveToken(&Op0);

            if (StructDeclaratorsTail)
            {
                StructDeclaratorsTail->Next = Declarator;
            }
            else
            {
                StructDeclarators = Declarator;
            }
            StructDeclaratorsTail = Declarator;
            StructPointerDepth    = 0;
        }
        else if (!strcmp(Operator->Value, "@STRUCT_FORWARD_DECLARATION"))
        {
            Op0 = Pop(MatchedStack);
            if (!DeclareStructType(Op0->Value))
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
            }
            RemoveToken(&Op0);
        }
        else if (!strcmp(Operator->Value, "@STRUCT_DEFINITION_BEGIN"))
        {
            Op0                     = Pop(MatchedStack);
            CurrentStructDefinition = FindStructType(Op0->Value);
            if (CurrentStructDefinition && CurrentStructDefinition->IsComplete)
            {
                *Error = SCRIPT_ENGINE_ERROR_DUPLICATE_STRUCT_DEFINITION;
            }
            else if (!CurrentStructDefinition)
            {
                CurrentStructDefinition = DeclareStructType(Op0->Value);
            }
            RemoveToken(&Op0);
            if (!CurrentStructDefinition)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
            }
        }
        else if (!strcmp(Operator->Value, "@STRUCT_MEMBER_DECLARATION"))
        {
            PVARIABLE_TYPE           BaseType = PopStructBaseType(MatchedStack, Error);
            PSTRUCT_DECLARATOR_STATE Declarator;
            if (!BaseType)
            {
                break;
            }

            for (Declarator = StructDeclarators; Declarator; Declarator = Declarator->Next)
            {
                PVARIABLE_TYPE MemberType = ApplyStructDeclarator(BaseType, Declarator);
                if (!MemberType)
                {
                    *Error = (BaseType->Kind == TY_STRUCT && !BaseType->IsComplete) ? SCRIPT_ENGINE_ERROR_INCOMPLETE_TYPE : SCRIPT_ENGINE_ERROR_INVALID_ARRAY_SIZE;
                    break;
                }
                if (MemberType->Kind == TY_STRUCT && !MemberType->IsComplete)
                {
                    *Error = SCRIPT_ENGINE_ERROR_INCOMPLETE_TYPE;
                    break;
                }
                if (!AddStructMember(CurrentStructDefinition, Declarator->Name, MemberType))
                {
                    *Error = SCRIPT_ENGINE_ERROR_DUPLICATE_STRUCT_MEMBER;
                    break;
                }
            }
            ResetStructDeclarators();
        }
        else if (!strcmp(Operator->Value, "@STRUCT_DEFINITION_END"))
        {
            if (!CompleteStructType(CurrentStructDefinition))
            {
                *Error = SCRIPT_ENGINE_ERROR_INCOMPLETE_TYPE;
            }
        }
        else if (!strcmp(Operator->Value, "@STRUCT_VARIABLE_DECLARATION"))
        {
            PVARIABLE_TYPE           BaseType;
            PSTRUCT_DECLARATOR_STATE Declarator;

            if (CurrentStructDefinition && CurrentStructDefinition->IsComplete && !MatchedStack->Pointer)
            {
                BaseType = CurrentStructDefinition;
            }
            else
            {
                BaseType = PopStructBaseType(MatchedStack, Error);
            }
            if (!BaseType)
            {
                break;
            }

            for (Declarator = StructDeclarators; Declarator; Declarator = Declarator->Next)
            {
                PVARIABLE_TYPE       ObjectType = ApplyStructDeclarator(BaseType, Declarator);
                PSCRIPT_ENGINE_TOKEN IdToken;
                if (!ObjectType || (ObjectType->Kind == TY_STRUCT && !ObjectType->IsComplete))
                {
                    *Error = SCRIPT_ENGINE_ERROR_INCOMPLETE_TYPE;
                    break;
                }
                IdToken = NewToken(LOCAL_UNRESOLVED_ID, Declarator->Name);
                if (GetLocalIdentifierVal(IdToken) != -1)
                {
                    RemoveToken(&IdToken);
                    *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                    break;
                }
                NewLocalIdentifier(IdToken, (unsigned int)ObjectType->Size);
                SetLocalIdentifierVariableType(IdToken, ObjectType);
                if (LastStructObject)
                    RemoveToken(&LastStructObject);
                IdToken->Type         = LOCAL_ID;
                IdToken->VariableType = ObjectType;
                LastStructObject      = CopyToken(IdToken);
                LastStructObjectType  = ObjectType;
                RemoveToken(&IdToken);
            }
            ResetStructDeclarators();
            CurrentStructDefinition = NULL;
        }
        else if (!strcmp(Operator->Value, "@STRUCT_INITIALIZER_BEGIN"))
        {
            Push(MatchedStack, CopyToken(Operator));
        }
        else if (!strcmp(Operator->Value, "@STRUCT_INITIALIZER_END"))
        {
            PSCRIPT_ENGINE_TOKEN Values[64];
            unsigned int         Count = 0;
            PSTRUCT_MEMBER       Member;
            PSYMBOL              Symbol;

            while (MatchedStack->Pointer && strcmp(Top(MatchedStack)->Value, "@STRUCT_INITIALIZER_BEGIN"))
            {
                if (Count >= 64)
                {
                    *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                    break;
                }
                Values[Count++] = Pop(MatchedStack);
            }
            if (*Error != SCRIPT_ENGINE_ERROR_FREE || !MatchedStack->Pointer || !LastStructObject ||
                !LastStructObjectType || LastStructObjectType->Kind != TY_STRUCT)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }
            Op0 = Pop(MatchedStack);
            RemoveToken(&Op0);

            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            Symbol->Value = FUNC_AGGREGATE_ZERO;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);
            Symbol = ToSymbol(LastStructObject, Error);
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);
            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_NUM_TYPE;
            Symbol->Value = SCRIPT_ENGINE_ADDRESS_SPACE_LOCAL;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);
            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_NUM_TYPE;
            Symbol->Value = LastStructObjectType->Size;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);

            Member = LastStructObjectType->Members;
            while (Count && Member)
            {
                PSCRIPT_ENGINE_TOKEN ValueToken    = Values[--Count];
                PSCRIPT_ENGINE_TOKEN AddressToken  = LastStructObject;
                BOOLEAN              AddressIsTemp = FALSE;
                if (Member->Type->Kind == TY_STRUCT || Member->Type->Kind == TY_ARRAY ||
                    (Member->Type->Size != 1 && Member->Type->Size != 2 && Member->Type->Size != 4 && Member->Type->Size != 8))
                {
                    *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                    RemoveToken(&ValueToken);
                    break;
                }
                if (Member->Offset)
                {
                    Temp               = NewTemp(Error);
                    Temp->VariableType = CreatePointerType(Member->Type);
                    Symbol             = NewSymbol();
                    Symbol->Type       = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value      = FUNC_ADD;
                    PushSymbol(CodeBuffer, Symbol);
                    RemoveSymbol(&Symbol);
                    Symbol = ToSymbol(LastStructObject, Error);
                    PushSymbol(CodeBuffer, Symbol);
                    RemoveSymbol(&Symbol);
                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_NUM_TYPE;
                    Symbol->Value = Member->Offset;
                    PushSymbol(CodeBuffer, Symbol);
                    RemoveSymbol(&Symbol);
                    Symbol = ToSymbol(Temp, Error);
                    PushSymbol(CodeBuffer, Symbol);
                    RemoveSymbol(&Symbol);
                    AddressToken  = Temp;
                    AddressIsTemp = TRUE;
                }
                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                Symbol->Value = FUNC_TYPED_STORE;
                PushSymbol(CodeBuffer, Symbol);
                RemoveSymbol(&Symbol);
                Symbol = ToSymbol(ValueToken, Error);
                PushSymbol(CodeBuffer, Symbol);
                RemoveSymbol(&Symbol);
                Symbol = ToSymbol(AddressToken, Error);
                PushSymbol(CodeBuffer, Symbol);
                RemoveSymbol(&Symbol);
                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_NUM_TYPE;
                Symbol->Value = SCRIPT_ENGINE_ADDRESS_SPACE_LOCAL;
                PushSymbol(CodeBuffer, Symbol);
                RemoveSymbol(&Symbol);
                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_NUM_TYPE;
                Symbol->Value = Member->Type->Size;
                PushSymbol(CodeBuffer, Symbol);
                RemoveSymbol(&Symbol);
                if (AddressIsTemp)
                    FreeTemp(Temp);
                RemoveToken(&ValueToken);
                Member = Member->Next;
            }
            if (Count)
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
        }
        else if (!strcmp(Operator->Value, "@STRUCT_POINTER_CAST"))
        {
            PVARIABLE_TYPE RemotePointerType;
            PSYMBOL        Symbol;
            Op0 = Pop(MatchedStack);
            while (MatchedStack->Pointer && Top(MatchedStack)->Type != LOCAL_UNRESOLVED_ID && Top(MatchedStack)->Type != LOCAL_ID)
            {
                Op1 = Pop(MatchedStack);
                RemoveToken(&Op1);
            }
            if (!LastStructObject || !LastStructObjectType || LastStructObjectType->Kind != TY_PTR)
            {
                *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                break;
            }
            RemotePointerType = CreateStructPointerType(LastStructObjectType->Base, POINTER_PROVENANCE_REMOTE);
            if (!RemotePointerType)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                RemoveToken(&Op0);
                break;
            }
            LastStructObjectType           = RemotePointerType;
            LastStructObject->VariableType = RemotePointerType;
            SetLocalIdentifierVariableType(LastStructObject, RemotePointerType);
            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            Symbol->Value = FUNC_MOV;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);
            Symbol = ToSymbol(Op0, Error);
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);
            Symbol = ToSymbol(LastStructObject, Error);
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);
            RemoveToken(&Op0);
        }
        else if (!strcmp(Operator->Value, "@MEMBER_DOT_LVALUE") ||
                 !strcmp(Operator->Value, "@MEMBER_ARROW_LVALUE") ||
                 !strcmp(Operator->Value, "@MEMBER_DOT_READ") ||
                 !strcmp(Operator->Value, "@MEMBER_ARROW_READ"))
        {
            BOOLEAN              IsArrow = strstr(Operator->Value, "ARROW") != NULL;
            BOOLEAN              IsRead  = strstr(Operator->Value, "READ") != NULL;
            PVARIABLE_TYPE       BaseType;
            PVARIABLE_TYPE       PointerType = NULL;
            PSTRUCT_MEMBER       Member;
            PSCRIPT_ENGINE_TOKEN AddressToken;
            PSYMBOL              Symbol;

            Op0      = Pop(MatchedStack); /* member name */
            Op1      = Pop(MatchedStack); /* object, pointer, or prior member address */
            BaseType = (PVARIABLE_TYPE)Op1->VariableType;
            if (!BaseType && (Op1->Type == LOCAL_ID || Op1->Type == LOCAL_UNRESOLVED_ID))
                BaseType = GetLocalIdentifierVariableType(Op1);
            if (!BaseType && (Op1->Type == GLOBAL_ID || Op1->Type == GLOBAL_UNRESOLVED_ID))
                BaseType = GetGlobalIdentifierVariableType(Op1);

            if (IsArrow)
            {
                if (!BaseType || BaseType->Kind != TY_PTR || !BaseType->Base || BaseType->Base->Kind != TY_STRUCT)
                {
                    *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                    RemoveToken(&Op0);
                    RemoveToken(&Op1);
                    break;
                }
                AddressToken = Op1;
                PointerType  = BaseType;
                BaseType     = BaseType->Base;
            }
            else
            {
                if (!BaseType || BaseType->Kind != TY_STRUCT)
                {
                    *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                    RemoveToken(&Op0);
                    RemoveToken(&Op1);
                    break;
                }
                AddressToken = Op1;
            }

            if (!BaseType->IsComplete || !(Member = FindStructMember(BaseType, Op0->Value)))
            {
                *Error = BaseType->IsComplete ? SCRIPT_ENGINE_ERROR_UNRESOLVED_VARIABLE : SCRIPT_ENGINE_ERROR_INCOMPLETE_TYPE;
                RemoveToken(&Op0);
                RemoveToken(&Op1);
                break;
            }

            Temp = NewTemp(Error);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                RemoveToken(&Op0);
                RemoveToken(&Op1);
                break;
            }
            Temp->VariableType = Member->Type;
            Temp->IsAddress    = TRUE;
            Temp->AddressSpace = IsArrow ? GetStructPointerAddressSpace(PointerType, Op1) : (Op1->AddressSpace ? Op1->AddressSpace : SCRIPT_ENGINE_ADDRESS_SPACE_LOCAL);

            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
            Symbol->Value = FUNC_ADD;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);
            Symbol = ToSymbol(AddressToken, Error);
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);
            Symbol        = NewSymbol();
            Symbol->Type  = SYMBOL_NUM_TYPE;
            Symbol->Value = Member->Offset;
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);
            Symbol = ToSymbol(Temp, Error);
            PushSymbol(CodeBuffer, Symbol);
            RemoveSymbol(&Symbol);

            RemoveToken(&Op0);
            RemoveToken(&Op1);

            if (IsRead && Member->Type->Kind != TY_STRUCT && Member->Type->Kind != TY_ARRAY)
            {
                PSCRIPT_ENGINE_TOKEN ValueTemp;
                if (Member->Type->Size != 1 && Member->Type->Size != 2 && Member->Type->Size != 4 && Member->Type->Size != 8)
                {
                    *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                    FreeTemp(Temp);
                    break;
                }
                ValueTemp = EmitTypedScalarLoad(CodeBuffer, Temp, Temp->AddressSpace, Member->Type, Error);
                if (!ValueTemp || *Error != SCRIPT_ENGINE_ERROR_FREE)
                {
                    FreeTemp(Temp);
                    break;
                }
                FreeTemp(Temp);
                Push(MatchedStack, ValueTemp);
            }
            else
            {
                Push(MatchedStack, Temp);
            }
        }
        else if (!strcmp(Operator->Value, "@TYPEDEF_DECLARATION"))
        {
            PVARIABLE_TYPE BaseType;
            unsigned int   Index;
            Op0      = Pop(MatchedStack);
            BaseType = PopStructBaseType(MatchedStack, Error);
            if (!BaseType)
            {
                RemoveToken(&Op0);
                break;
            }
            for (Index = 0; Index < StructPointerDepth; Index++)
            {
                BaseType = CreatePointerType(BaseType);
            }
            StructPointerDepth = 0;
            if (!BaseType || !AddTypedefType(Op0->Value, BaseType))
            {
                *Error = SCRIPT_ENGINE_ERROR_DUPLICATE_TYPEDEF;
            }
            RemoveToken(&Op0);
        }
        else if (!strcmp(Operator->Value, "@INCLUDE"))
        {
            char *  IncludeFilePath;
            char    FullPath[MAX_PATH_LEN];
            char *  IncludeFileBuffer;
            BOOLEAN IncludedPath = FALSE;

            Temp            = Pop(MatchedStack);
            IncludeFilePath = Temp->Value;

            ResolveIncludePath(IncludeFilePath, FullPath);

            if (!FileExists(FullPath))
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }

            if (!ParseIncludeFile(FullPath, &IncludeFileBuffer))
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                break;
            }

            if (!IncludeHead)
            {
                IncludeHead           = calloc(sizeof(INCLUDE_NODE), 1);
                IncludeHead->FilePath = PlatformStrDup(FullPath);
            }
            else
            {
                PINCLUDE_NODE Node, PrevNode = NULL;

                for (Node = IncludeHead; Node; PrevNode = Node, Node = Node->NextNode)
                {
                    if (!strcmp(Node->FilePath, FullPath))
                    {
                        IncludedPath = TRUE;
                        break;
                    }
                }

                if (!IncludedPath && PrevNode)
                {
                    PrevNode->NextNode           = calloc(sizeof(INCLUDE_NODE), 1);
                    PrevNode->NextNode->FilePath = PlatformStrDup(FullPath);
                }
            }

            if (!IncludedPath)
            {
                *ScriptSource = InsertStrNew(*ScriptSource, InputIdx, IncludeFileBuffer);
            }
        }

        else if (!strcmp(Operator->Value, "@START_OF_USER_DEFINED_FUNCTION"))
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
            PlatformZeroMemory(Node->NextNode, sizeof(USER_DEFINED_FUNCTION_NODE));
            CurrentUserDefinedFunction = Node->NextNode;

            CurrentUserDefinedFunction->Name                     = PlatformStrDup(Op0->Value);
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
                Temp               = NewTemp(Error);
                Temp->VariableType = (PVARIABLE_TYPE)Node->VariableType;
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
            int                    Op1Capacity                = 8;
            int                    Op1Count                   = 0;
            BOOLEAN                HasExplicitDestinationType = FALSE;
            PSCRIPT_ENGINE_TOKEN * Op1Array                   = (PSCRIPT_ENGINE_TOKEN *)malloc(sizeof(PSCRIPT_ENGINE_TOKEN) * Op1Capacity);
            PSYMBOL                Symbol                     = NewSymbol();
            Symbol->Type                                      = SYMBOL_SEMANTIC_RULE_TYPE;
            Symbol->Value                                     = FUNC_MOV;

            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            for (unsigned int i = 0; i < MatchedStack->Pointer; i++)
            {
                PSCRIPT_ENGINE_TOKEN Candidate = *(MatchedStack->Head + i);
                if (Candidate->Type == SCRIPT_VARIABLE_TYPE ||
                    !strcmp(Candidate->Value, "@DECLARE_POINTER_TYPE"))
                {
                    HasExplicitDestinationType = TRUE;
                    break;
                }
            }

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
                    Op1->VariableType   = GetDefaultImplicitVariableType();
                    Op1->IsImplicitType = !HasExplicitDestinationType;
                    PushSymbol(CodeBuffer, Symbol);
                    PushSymbol(CodeBuffer, Op0Symbol);

                    Op1Symbol = NewSymbol();
                    free((void *)Op1Symbol->Value);
                    Op1Symbol->Value = NewGlobalIdentifier(Op1);
                    SetType(&Op1Symbol->Type, SYMBOL_GLOBAL_ID_TYPE);
                    SetGlobalIdentifierVariableType(Op1, GetDefaultImplicitVariableType());
                    Pop(MatchedStack);
                    PushSymbol(CodeBuffer, Op1Symbol);
                }
                else if (Op1->Type == LOCAL_UNRESOLVED_ID)
                {
                    Op1->VariableType   = GetDefaultImplicitVariableType();
                    Op1->IsImplicitType = !HasExplicitDestinationType;
                    PushSymbol(CodeBuffer, Symbol);
                    PushSymbol(CodeBuffer, Op0Symbol);

                    Op1Symbol = NewSymbol();
                    free((void *)Op1Symbol->Value);
                    Op1Symbol->Value = NewLocalIdentifier(Op1, 8);
                    SetType(&Op1Symbol->Type, SYMBOL_LOCAL_ID_TYPE);
                    SetLocalIdentifierVariableType(Op1, GetDefaultImplicitVariableType());
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
                            SetLocalIdentifierVariableType(Op1, VariableType);
                        }
                        else if (Op1->Type == GLOBAL_UNRESOLVED_ID || Op1->Type == GLOBAL_ID)
                        {
                            SetGlobalIdentifierVariableType(Op1, VariableType);
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
            Temp         = NewTemp(Error);
            TempSymbol   = ToSymbol(Temp, Error);
            OffsetToken  = NewTemp(Error);
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
                PSCRIPT_ENGINE_TOKEN ValueToken = EmitTypedScalarLoad(
                    CodeBuffer,
                    OffsetToken,
                    IdToken->AddressSpace ? IdToken->AddressSpace : SCRIPT_ENGINE_ADDRESS_SPACE_LOCAL,
                    VariableType,
                    Error);
                if (!ValueToken || *Error != SCRIPT_ENGINE_ERROR_FREE)
                {
                    break;
                }
                FreeTemp(OffsetToken);
                RemoveToken(&OffsetToken);
                OffsetToken = ValueToken;
            }
            else if (!strcmp(Operator->Value, "@ARRAY_INDEX_WRITE"))
            {
                OffsetToken->Type         = DEFERENCE_TEMP;
                OffsetToken->VariableType = VariableType;
                OffsetToken->IsAddress    = TRUE;
                OffsetToken->AddressSpace = IdToken->AddressSpace ? IdToken->AddressSpace : SCRIPT_ENGINE_ADDRESS_SPACE_LOCAL;
            }

            Push(MatchedStack, OffsetToken);

            FreeTemp(Temp);
        }
        else if (!strcmp(Operator->Value, "@MOV"))
        {
            BOOLEAN              IsFloatingInitialization   = FALSE;
            BOOLEAN              ScalarAssignmentConverted  = FALSE;
            BOOLEAN              HasExplicitDestinationType = FALSE;
            PSCRIPT_ENGINE_TOKEN ConvertedTemp              = NULL;
            PSYMBOL              ConvertedTempSymbol        = NULL;

            PushSymbol(CodeBuffer, OperatorSymbol);
            Op0       = Pop(MatchedStack);
            Op0Symbol = ToSymbol(Op0, Error);

            Op1                        = Pop(MatchedStack);
            HasExplicitDestinationType = MatchedStack->Pointer > 0 &&
                                         (!strcmp(Top(MatchedStack)->Value, "@DECLARE_POINTER_TYPE") ||
                                          Top(MatchedStack)->Type == SCRIPT_VARIABLE_TYPE);
            if (Op1->Type == GLOBAL_UNRESOLVED_ID)
            {
                Op1->VariableType   = GetDefaultImplicitVariableType();
                Op1->IsImplicitType = !HasExplicitDestinationType;
                Op1Symbol           = NewSymbol();
                free((void *)Op1Symbol->Value);
                Op1Symbol->Value = NewGlobalIdentifier(Op1);
                SetType(&Op1Symbol->Type, SYMBOL_GLOBAL_ID_TYPE);
                SetGlobalIdentifierVariableType(Op1, GetDefaultImplicitVariableType());
            }
            else if (Op1->Type == LOCAL_UNRESOLVED_ID)
            {
                Op1->VariableType   = GetDefaultImplicitVariableType();
                Op1->IsImplicitType = !HasExplicitDestinationType;
                Op1Symbol           = NewSymbol();
                free((void *)Op1Symbol->Value);
                Op1Symbol->Value = NewLocalIdentifier(Op1, 8);
                SetType(&Op1Symbol->Type, SYMBOL_LOCAL_ID_TYPE);
                SetLocalIdentifierVariableType(Op1, GetDefaultImplicitVariableType());
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

                    Op1->VariableType        = VariableType;
                    IsFloatingInitialization = VariableType->Kind == TY_FLOAT || VariableType->Kind == TY_DOUBLE;

                    if (Op1->Type == LOCAL_UNRESOLVED_ID || Op1->Type == LOCAL_ID)
                    {
                        SetLocalIdentifierVariableType(Op1, VariableType);
                    }
                    else if (Op1->Type == GLOBAL_UNRESOLVED_ID || Op1->Type == GLOBAL_ID)
                    {
                        SetGlobalIdentifierVariableType(Op1, VariableType);
                    }
                }
            }

            if (Op0->VariableType && Op1->VariableType)
            {
                UINT64  SourceTypeId             = GetScriptScalarTypeId((PVARIABLE_TYPE)Op0->VariableType);
                UINT64  DestinationTypeId        = GetScriptScalarTypeId((PVARIABLE_TYPE)Op1->VariableType);
                BOOLEAN PointerConversionAllowed = TRUE;

                if (SourceTypeId == SCRIPT_SCALAR_TYPE_POINTER && DestinationTypeId != SCRIPT_SCALAR_TYPE_POINTER &&
                    !(Op1->IsImplicitType && DestinationTypeId == SCRIPT_SCALAR_TYPE_U64))
                    PointerConversionAllowed = FALSE;
                if (DestinationTypeId == SCRIPT_SCALAR_TYPE_POINTER && SourceTypeId != SCRIPT_SCALAR_TYPE_POINTER &&
                    !((Op0->Type == HEX || Op0->Type == OCTAL || Op0->Type == BINARY || Op0->Type == DECIMAL) &&
                      Op0Symbol->Value == 0))
                    PointerConversionAllowed = FALSE;

                if (SourceTypeId != SCRIPT_SCALAR_TYPE_INVALID && DestinationTypeId != SCRIPT_SCALAR_TYPE_INVALID &&
                    SourceTypeId != SCRIPT_SCALAR_TYPE_F80 && DestinationTypeId != SCRIPT_SCALAR_TYPE_F80 &&
                    PointerConversionAllowed && SourceTypeId != DestinationTypeId &&
                    !(Op0->Type == FLOAT_LITERAL && IsFloatingVariableType((PVARIABLE_TYPE)Op1->VariableType)))
                {
                    PSYMBOL TypeSymbol;
                    if (Op0->Type == FLOAT_LITERAL)
                    {
                        UINT64 SourceKind = GetFloatingValueKind((PVARIABLE_TYPE)Op0->VariableType);
                        if (!ParseFloatingLiteral(Op0->Value, SourceKind, &Op0Symbol->Value, Error))
                            break;
                        Op0Symbol->Len = SourceKind;
                    }

                    ConvertedTemp               = NewTemp(Error);
                    ConvertedTemp->VariableType = (PVARIABLE_TYPE)Op1->VariableType;
                    ConvertedTempSymbol         = ToSymbol(ConvertedTemp, Error);
                    ConvertedTempSymbol->Len    = GetFloatingValueKind((PVARIABLE_TYPE)Op1->VariableType);

                    (CodeBuffer->Head + CodeBuffer->Pointer - 1)->Value = FUNC_CAST_SCALAR;
                    PushSymbol(CodeBuffer, Op0Symbol);
                    PushSymbol(CodeBuffer, ConvertedTempSymbol);
                    TypeSymbol        = NewSymbol();
                    TypeSymbol->Type  = SYMBOL_NUM_TYPE;
                    TypeSymbol->Value = SourceTypeId;
                    PushSymbol(CodeBuffer, TypeSymbol);
                    TypeSymbol->Value = DestinationTypeId;
                    PushSymbol(CodeBuffer, TypeSymbol);
                    RemoveSymbol(&TypeSymbol);
                    ScalarAssignmentConverted = TRUE;
                }
                else if (!PointerConversionAllowed)
                {
                    *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                    break;
                }
            }

            if (!ScalarAssignmentConverted && IsFloatingInitialization)
            {
                UINT64  ValueKind          = GetFloatingValueKind(VariableType);
                BOOLEAN RequiresConversion = FALSE;

                if (Op1->Type != LOCAL_UNRESOLVED_ID && Op1->Type != LOCAL_ID)
                {
                    *Error = SCRIPT_ENGINE_ERROR_UNSUPPORTED_FLOAT_OPERATION;
                    break;
                }

                if (Op0->Type == FLOAT_LITERAL)
                {
                    if (!ParseFloatingLiteral(Op0->Value, ValueKind, &Op0Symbol->Value, Error))
                    {
                        break;
                    }
                    Op0Symbol->Len = ValueKind;
                }
                else if (!IsFloatingVariableType((PVARIABLE_TYPE)Op0->VariableType))
                {
                    *Error = SCRIPT_ENGINE_ERROR_UNSUPPORTED_FLOAT_OPERATION;
                    break;
                }
                else
                {
                    RequiresConversion = Op0Symbol->Len != ValueKind;
                }

                Op1Symbol->Len = ValueKind;
                (CodeBuffer->Head + CodeBuffer->Pointer - 1)->Value =
                    RequiresConversion ? FUNC_CONVERT_FLOAT : FUNC_MOV_FLOAT;
            }

            if (Op0->VariableType && Op1->VariableType &&
                ((PVARIABLE_TYPE)Op0->VariableType)->Kind == TY_PTR &&
                ((PVARIABLE_TYPE)Op1->VariableType)->Kind == TY_PTR)
            {
                PVARIABLE_TYPE SourcePointerType      = (PVARIABLE_TYPE)Op0->VariableType;
                PVARIABLE_TYPE DestinationPointerType = (PVARIABLE_TYPE)Op1->VariableType;
                PVARIABLE_TYPE AssignedPointerType    = CreateStructPointerType(DestinationPointerType->Base,
                                                                                SourcePointerType->PointerProvenance);
                if (!AssignedPointerType)
                {
                    *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                    break;
                }

                Op1->VariableType = AssignedPointerType;
                if (Op1->Type == LOCAL_ID || Op1->Type == LOCAL_UNRESOLVED_ID)
                {
                    SetLocalIdentifierVariableType(Op1, AssignedPointerType);
                }
                else if (Op1->Type == GLOBAL_ID || Op1->Type == GLOBAL_UNRESOLVED_ID)
                {
                    SetGlobalIdentifierVariableType(Op1, AssignedPointerType);
                }
            }

            if (Op0->VariableType && Op1->VariableType &&
                ((PVARIABLE_TYPE)Op0->VariableType)->Kind == TY_STRUCT &&
                ((PVARIABLE_TYPE)Op1->VariableType)->Kind == TY_STRUCT)
            {
                PSYMBOL Symbol;
                if (Op0->VariableType != Op1->VariableType || !((PVARIABLE_TYPE)Op0->VariableType)->IsComplete)
                {
                    *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                    break;
                }
                (CodeBuffer->Head + CodeBuffer->Pointer - 1)->Value = FUNC_AGGREGATE_COPY;
                PushSymbol(CodeBuffer, Op1Symbol);
                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_NUM_TYPE;
                Symbol->Value = Op1->AddressSpace ? Op1->AddressSpace : SCRIPT_ENGINE_ADDRESS_SPACE_LOCAL;
                PushSymbol(CodeBuffer, Symbol);
                RemoveSymbol(&Symbol);
                PushSymbol(CodeBuffer, Op0Symbol);
                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_NUM_TYPE;
                Symbol->Value = Op0->AddressSpace ? Op0->AddressSpace : SCRIPT_ENGINE_ADDRESS_SPACE_LOCAL;
                PushSymbol(CodeBuffer, Symbol);
                RemoveSymbol(&Symbol);
                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_NUM_TYPE;
                Symbol->Value = ((PVARIABLE_TYPE)Op0->VariableType)->Size;
                PushSymbol(CodeBuffer, Symbol);
                RemoveSymbol(&Symbol);
            }
            else if (ScalarAssignmentConverted)
            {
                PSYMBOL Symbol = NewSymbol();
                if (Op1->IsAddress)
                {
                    PVARIABLE_TYPE DestinationType = (PVARIABLE_TYPE)Op1->VariableType;
                    Symbol->Type                   = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value                  = FUNC_TYPED_STORE;
                    PushSymbol(CodeBuffer, Symbol);
                    PushSymbol(CodeBuffer, ConvertedTempSymbol);
                    PushSymbol(CodeBuffer, Op1Symbol);
                    Symbol->Type  = SYMBOL_NUM_TYPE;
                    Symbol->Value = Op1->AddressSpace;
                    PushSymbol(CodeBuffer, Symbol);
                    Symbol->Value = DestinationType->Size;
                    PushSymbol(CodeBuffer, Symbol);
                }
                else
                {
                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = IsFloatingVariableType((PVARIABLE_TYPE)Op1->VariableType) ? FUNC_MOV_FLOAT : FUNC_MOV;
                    PushSymbol(CodeBuffer, Symbol);
                    PushSymbol(CodeBuffer, ConvertedTempSymbol);
                    Op1Symbol->Len = GetFloatingValueKind((PVARIABLE_TYPE)Op1->VariableType);
                    PushSymbol(CodeBuffer, Op1Symbol);
                }
                RemoveSymbol(&Symbol);
                FreeTemp(ConvertedTemp);
            }
            else if (Op1->IsAddress)
            {
                PSYMBOL        Symbol;
                PVARIABLE_TYPE DestinationType = (PVARIABLE_TYPE)Op1->VariableType;
                if (!DestinationType || (DestinationType->Size != 1 && DestinationType->Size != 2 && DestinationType->Size != 4 && DestinationType->Size != 8))
                {
                    *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                    break;
                }
                (CodeBuffer->Head + CodeBuffer->Pointer - 1)->Value = FUNC_TYPED_STORE;
                PushSymbol(CodeBuffer, Op0Symbol);
                PushSymbol(CodeBuffer, Op1Symbol);
                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_NUM_TYPE;
                Symbol->Value = Op1->AddressSpace;
                PushSymbol(CodeBuffer, Symbol);
                RemoveSymbol(&Symbol);
                Symbol        = NewSymbol();
                Symbol->Type  = SYMBOL_NUM_TYPE;
                Symbol->Value = DestinationType->Size;
                PushSymbol(CodeBuffer, Symbol);
                RemoveSymbol(&Symbol);
            }
            else
            {
                PushSymbol(CodeBuffer, Op0Symbol);
                PushSymbol(CodeBuffer, Op1Symbol);
            }

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

            for (SIZE_T i = 0; i < TokenCount / 2; i++)
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
                VariableType = GetDefaultImplicitVariableType();
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
                SetLocalIdentifierVariableType(IdToken, VariableType);
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

                    Temp       = NewTemp(Error);
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

                Temp       = NewTemp(Error);
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
        else if (!strcmp(Operator->Value, "@REFERENCE"))
        {
            POINTER_PROVENANCE Provenance;
            unsigned int       AddressSpace;

            Op0 = Pop(MatchedStack);
            if (!ResolveIdentifierVariableType(Op0))
            {
                *Error = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                RemoveToken(&Op0);
                break;
            }

            AddressSpace = Op0->AddressSpace ? Op0->AddressSpace : SCRIPT_ENGINE_ADDRESS_SPACE_LOCAL;
            Provenance   = AddressSpace == SCRIPT_ENGINE_ADDRESS_SPACE_REMOTE ? POINTER_PROVENANCE_REMOTE : POINTER_PROVENANCE_LOCAL;

            Temp = NewTemp(Error);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                RemoveToken(&Op0);
                break;
            }
            Temp->VariableType = CreateStructPointerType((PVARIABLE_TYPE)Op0->VariableType, Provenance);
            Temp->AddressSpace = AddressSpace;
            if (!Temp->VariableType)
            {
                *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                FreeTemp(Temp);
                RemoveToken(&Op0);
                break;
            }

            Op0Symbol  = ToSymbol(Op0, Error);
            TempSymbol = ToSymbol(Temp, Error);
            PushSymbol(CodeBuffer, OperatorSymbol);
            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, TempSymbol);
            FreeTemp(Op0);
            Push(MatchedStack, Temp);
        }
        else if (!strcmp(Operator->Value, "@POI") &&
                 MatchedStack->Pointer &&
                 Top(MatchedStack)->VariableType &&
                 ((PVARIABLE_TYPE)Top(MatchedStack)->VariableType)->Kind == TY_PTR &&
                 ((PVARIABLE_TYPE)Top(MatchedStack)->VariableType)->Base &&
                 ((PVARIABLE_TYPE)Top(MatchedStack)->VariableType)->Base->Kind == TY_STRUCT)
        {
            PVARIABLE_TYPE PointerType;
            PSYMBOL        MoveSymbol;

            Op0         = Pop(MatchedStack);
            PointerType = (PVARIABLE_TYPE)Op0->VariableType;
            Temp        = NewTemp(Error);
            if (*Error != SCRIPT_ENGINE_ERROR_FREE)
            {
                RemoveToken(&Op0);
                break;
            }

            Temp->VariableType = PointerType->Base;
            Temp->AddressSpace = GetStructPointerAddressSpace(PointerType, Op0);
            Temp->IsAddress    = TRUE;
            Op0Symbol          = ToSymbol(Op0, Error);
            TempSymbol         = ToSymbol(Temp, Error);
            MoveSymbol         = NewSymbol();
            MoveSymbol->Type   = SYMBOL_SEMANTIC_RULE_TYPE;
            MoveSymbol->Value  = FUNC_MOV;
            PushSymbol(CodeBuffer, MoveSymbol);
            PushSymbol(CodeBuffer, Op0Symbol);
            PushSymbol(CodeBuffer, TempSymbol);
            RemoveSymbol(&MoveSymbol);
            FreeTemp(Op0);
            Push(MatchedStack, Temp);
        }
        else if (IsType1Func(Operator))
        {
            Op0 = Pop(MatchedStack);

            if (!strcmp(Operator->Value, "@NEG") && Op0->Type == FLOAT_LITERAL)
            {
                PSCRIPT_ENGINE_TOKEN FoldedToken;
                CHAR *               FoldedValue;
                SIZE_T               ValueLength = strlen(Op0->Value);

                if (Op0->Value[0] == '-')
                {
                    FoldedToken = NewToken(FLOAT_LITERAL, Op0->Value + 1);
                }
                else
                {
                    FoldedValue = (CHAR *)malloc(ValueLength + 2);
                    if (!FoldedValue)
                    {
                        *Error = SCRIPT_ENGINE_ERROR_TEMP_LIST_FULL;
                        RemoveToken(&Op0);
                        break;
                    }
                    FoldedValue[0] = '-';
                    memcpy(FoldedValue + 1, Op0->Value, ValueLength + 1);
                    FoldedToken = NewToken(FLOAT_LITERAL, FoldedValue);
                    free(FoldedValue);
                }

                FoldedToken->VariableType = (VARIABLE_TYPE *)VARIABLE_TYPE_DOUBLE;
                RemoveToken(&Op0);
                Push(MatchedStack, FoldedToken);
            }
            else
            {
                VariableType                = (VARIABLE_TYPE *)Op0->VariableType;
                Op0Symbol                   = ToSymbol(Op0, Error);
                BOOLEAN IsTypedIntegerUnary = FALSE;

                if (!strcmp(Operator->Value, "@NEG") &&
                    VariableType &&
                    (VariableType->Kind == TY_FLOAT || VariableType->Kind == TY_DOUBLE))
                {
                    OperatorSymbol->Value = FUNC_NEG_FLOAT;
                }
                else if (IsIntegerVariableType(VariableType) &&
                         (!strcmp(Operator->Value, "@NEG") || !strcmp(Operator->Value, "@NOT")))
                {
                    VariableType          = PromoteIntegerVariableType(VariableType);
                    OperatorSymbol->Value = !strcmp(Operator->Value, "@NEG") ? FUNC_NEG_TYPED : FUNC_BITWISE_NOT_TYPED;
                    IsTypedIntegerUnary   = TRUE;
                }

                PushSymbol(CodeBuffer, OperatorSymbol);
                Temp               = NewTemp(Error);
                Temp->VariableType = (!strcmp(Operator->Value, "@NEG") ||
                                      !strcmp(Operator->Value, "@NOT"))
                                         ? VariableType
                                         : GetDefaultImplicitVariableType();
                Push(MatchedStack, Temp);
                TempSymbol = ToSymbol(Temp, Error);

                PushSymbol(CodeBuffer, Op0Symbol);
                PushSymbol(CodeBuffer, TempSymbol);
                if (IsTypedIntegerUnary)
                {
                    PSYMBOL TypeSymbol = NewSymbol();
                    TypeSymbol->Type   = SYMBOL_NUM_TYPE;
                    TypeSymbol->Value  = GetScriptScalarTypeId(VariableType);
                    PushSymbol(CodeBuffer, TypeSymbol);
                    RemoveSymbol(&TypeSymbol);
                }

                FreeTemp(Op0);
                if (*Error != SCRIPT_ENGINE_ERROR_FREE)
                {
                    break;
                }
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
                        Temp == 'x' || Temp == 'c' || Temp == 'p' || Temp == 's' || Temp == 'f' ||

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

            Temp               = NewTemp(Error);
            Temp->VariableType = GetDefaultImplicitVariableType();
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

            Temp               = NewTemp(Error);
            Temp->VariableType = GetDefaultImplicitVariableType();
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

            Temp               = NewTemp(Error);
            Temp->VariableType = GetDefaultImplicitVariableType();
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

            if (Op1->IsAddress && ((VARIABLE_TYPE *)Op1->VariableType)->Kind != TY_PTR)
            {
                PVARIABLE_TYPE       LValueType = (PVARIABLE_TYPE)Op1->VariableType;
                PVARIABLE_TYPE       CommonType;
                UINT64               TypedOpcode;
                PSCRIPT_ENGINE_TOKEN LoadedValue;
                PSYMBOL              Symbol;

                if (LValueType->Size != 1 && LValueType->Size != 2 &&
                    LValueType->Size != 4 && LValueType->Size != 8)
                {
                    *Error  = SCRIPT_ENGINE_ERROR_UNDEFINED_VARIABLE_TYPE;
                    Op0     = Pop(MatchedStack);
                    Op1     = Pop(MatchedStack);
                    Handled = TRUE;
                    break;
                }

                Op0         = Pop(MatchedStack);
                Op1         = Pop(MatchedStack);
                LoadedValue = EmitTypedScalarLoad(CodeBuffer,
                                                  Op1,
                                                  Op1->AddressSpace,
                                                  LValueType,
                                                  Error);
                if (!LoadedValue || *Error != SCRIPT_ENGINE_ERROR_FREE)
                {
                    Handled = TRUE;
                    break;
                }

                CommonType  = GetCommonVariableType((PVARIABLE_TYPE)Op0->VariableType, LValueType);
                TypedOpcode = GetTypedAssignmentOpcode(Operator->Value);
                Op0Symbol   = ToSymbol(Op0, Error);
                Op1Symbol   = ToSymbol(Op1, Error);

                if (IsIntegerVariableType(CommonType) && TypedOpcode != FUNC_UNDEFINED &&
                    IsIntegerVariableType(LValueType))
                {
                    PSCRIPT_ENGINE_TOKEN OperationTemp = NewTemp(Error);
                    PSCRIPT_ENGINE_TOKEN CastTemp      = NewTemp(Error);
                    PSYMBOL              OperationTempSymbol;
                    PSYMBOL              CastTempSymbol;
                    PSYMBOL              LoadedValueSymbol;

                    OperationTemp->VariableType = CommonType;
                    CastTemp->VariableType      = LValueType;
                    OperationTempSymbol         = ToSymbol(OperationTemp, Error);
                    CastTempSymbol              = ToSymbol(CastTemp, Error);
                    LoadedValueSymbol           = ToSymbol(LoadedValue, Error);

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = TypedOpcode;
                    PushSymbol(CodeBuffer, Symbol);
                    PushSymbol(CodeBuffer, Op0Symbol);
                    PushSymbol(CodeBuffer, LoadedValueSymbol);
                    PushSymbol(CodeBuffer, OperationTempSymbol);
                    Symbol->Type  = SYMBOL_NUM_TYPE;
                    Symbol->Value = GetScriptScalarTypeId(CommonType);
                    PushSymbol(CodeBuffer, Symbol);

                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = FUNC_CAST_SCALAR;
                    PushSymbol(CodeBuffer, Symbol);
                    PushSymbol(CodeBuffer, OperationTempSymbol);
                    PushSymbol(CodeBuffer, CastTempSymbol);
                    Symbol->Type  = SYMBOL_NUM_TYPE;
                    Symbol->Value = GetScriptScalarTypeId(CommonType);
                    PushSymbol(CodeBuffer, Symbol);
                    Symbol->Value = GetScriptScalarTypeId(LValueType);
                    PushSymbol(CodeBuffer, Symbol);

                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = FUNC_TYPED_STORE;
                    PushSymbol(CodeBuffer, Symbol);
                    PushSymbol(CodeBuffer, CastTempSymbol);
                    PushSymbol(CodeBuffer, Op1Symbol);
                    Symbol->Type  = SYMBOL_NUM_TYPE;
                    Symbol->Value = Op1->AddressSpace;
                    PushSymbol(CodeBuffer, Symbol);
                    Symbol->Value = LValueType->Size;
                    PushSymbol(CodeBuffer, Symbol);
                    RemoveSymbol(&Symbol);

                    FreeTemp(OperationTemp);
                    FreeTemp(CastTemp);
                    RemoveToken(&OperationTemp);
                    RemoveToken(&CastTemp);
                }
                else
                {
                    TempSymbol = ToSymbol(LoadedValue, Error);
                    PushSymbol(CodeBuffer, OperatorSymbol);
                    PushSymbol(CodeBuffer, Op0Symbol);
                    PushSymbol(CodeBuffer, TempSymbol);
                    PushSymbol(CodeBuffer, TempSymbol);

                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = FUNC_TYPED_STORE;
                    PushSymbol(CodeBuffer, Symbol);
                    RemoveSymbol(&Symbol);
                    PushSymbol(CodeBuffer, TempSymbol);
                    PushSymbol(CodeBuffer, Op1Symbol);
                    Symbol        = NewSymbol();
                    Symbol->Type  = SYMBOL_NUM_TYPE;
                    Symbol->Value = Op1->AddressSpace;
                    PushSymbol(CodeBuffer, Symbol);
                    Symbol->Value = LValueType->Size;
                    PushSymbol(CodeBuffer, Symbol);
                    RemoveSymbol(&Symbol);
                }

                FreeTemp(Op0);
                FreeTemp(Op1);
                FreeTemp(LoadedValue);
                RemoveToken(&LoadedValue);
                Handled = TRUE;
                if (*Error != SCRIPT_ENGINE_ERROR_FREE)
                {
                    break;
                }
            }

            if (!Handled && ((VARIABLE_TYPE *)Op1->VariableType)->Kind == TY_PTR)
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
                PVARIABLE_TYPE CommonType;
                UINT64         TypedOpcode;
                Op0         = Pop(MatchedStack);
                Op1         = Pop(MatchedStack);
                Op0Symbol   = ToSymbol(Op0, Error);
                Op1Symbol   = ToSymbol(Op1, Error);
                CommonType  = GetCommonVariableType((PVARIABLE_TYPE)Op0->VariableType,
                                                    (PVARIABLE_TYPE)Op1->VariableType);
                TypedOpcode = GetTypedAssignmentOpcode(Operator->Value);

                if (IsIntegerVariableType(CommonType) && TypedOpcode != FUNC_UNDEFINED &&
                    IsIntegerVariableType((PVARIABLE_TYPE)Op1->VariableType))
                {
                    PSCRIPT_ENGINE_TOKEN OperationTemp = NewTemp(Error);
                    PSCRIPT_ENGINE_TOKEN CastTemp      = NewTemp(Error);
                    PSYMBOL              OperationTempSymbol;
                    PSYMBOL              CastTempSymbol;
                    PSYMBOL              Symbol = NewSymbol();
                    OperationTemp->VariableType = CommonType;
                    CastTemp->VariableType      = (PVARIABLE_TYPE)Op1->VariableType;
                    OperationTempSymbol         = ToSymbol(OperationTemp, Error);
                    CastTempSymbol              = ToSymbol(CastTemp, Error);

                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = TypedOpcode;
                    PushSymbol(CodeBuffer, Symbol);
                    PushSymbol(CodeBuffer, Op0Symbol);
                    PushSymbol(CodeBuffer, Op1Symbol);
                    PushSymbol(CodeBuffer, OperationTempSymbol);
                    Symbol->Type  = SYMBOL_NUM_TYPE;
                    Symbol->Value = GetScriptScalarTypeId(CommonType);
                    PushSymbol(CodeBuffer, Symbol);

                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = FUNC_CAST_SCALAR;
                    PushSymbol(CodeBuffer, Symbol);
                    PushSymbol(CodeBuffer, OperationTempSymbol);
                    PushSymbol(CodeBuffer, CastTempSymbol);
                    Symbol->Type  = SYMBOL_NUM_TYPE;
                    Symbol->Value = GetScriptScalarTypeId(CommonType);
                    PushSymbol(CodeBuffer, Symbol);
                    Symbol->Value = GetScriptScalarTypeId((PVARIABLE_TYPE)Op1->VariableType);
                    PushSymbol(CodeBuffer, Symbol);

                    Symbol->Type  = SYMBOL_SEMANTIC_RULE_TYPE;
                    Symbol->Value = FUNC_MOV;
                    PushSymbol(CodeBuffer, Symbol);
                    PushSymbol(CodeBuffer, CastTempSymbol);
                    PushSymbol(CodeBuffer, Op1Symbol);
                    RemoveSymbol(&Symbol);
                    FreeTemp(OperationTemp);
                    FreeTemp(CastTemp);
                    RemoveToken(&OperationTemp);
                    RemoveToken(&CastTemp);
                }
                else
                {
                    PushSymbol(CodeBuffer, OperatorSymbol);
                    PushSymbol(CodeBuffer, Op0Symbol);
                    PushSymbol(CodeBuffer, Op1Symbol);
                    PushSymbol(CodeBuffer, Op1Symbol);
                }

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

            if (IsFloatingVariableType((PVARIABLE_TYPE)Op0->VariableType) ||
                IsFloatingVariableType((PVARIABLE_TYPE)Op1->VariableType))
            {
                UINT64 FloatingOpcode = GetFloatingBinaryOpcode(Operator->Value);
                if (!FloatingOpcode ||
                    (!IsFloatingVariableType((PVARIABLE_TYPE)Op0->VariableType) &&
                     !IsIntegerVariableType((PVARIABLE_TYPE)Op0->VariableType)) ||
                    (!IsFloatingVariableType((PVARIABLE_TYPE)Op1->VariableType) &&
                     !IsIntegerVariableType((PVARIABLE_TYPE)Op1->VariableType)))
                {
                    *Error = SCRIPT_ENGINE_ERROR_UNSUPPORTED_FLOAT_OPERATION;
                    Op0    = Pop(MatchedStack);
                    Op1    = Pop(MatchedStack);
                    RemoveToken(&Op0);
                    RemoveToken(&Op1);
                    Handled = TRUE;
                }
                else
                {
                    PVARIABLE_TYPE ResultType =
                        ((PVARIABLE_TYPE)Op0->VariableType)->Kind == TY_DOUBLE ||
                                ((PVARIABLE_TYPE)Op1->VariableType)->Kind == TY_DOUBLE
                            ? VARIABLE_TYPE_DOUBLE
                            : VARIABLE_TYPE_FLOAT;
                    PSCRIPT_ENGINE_TOKEN ConvertedOp0 = NULL;
                    PSCRIPT_ENGINE_TOKEN ConvertedOp1 = NULL;
                    PSYMBOL              Symbol;

                    Op0       = Pop(MatchedStack);
                    Op1       = Pop(MatchedStack);
                    Op0Symbol = ToSymbol(Op0, Error);
                    Op1Symbol = ToSymbol(Op1, Error);

                    if ((PVARIABLE_TYPE)Op0->VariableType != ResultType)
                    {
                        ConvertedOp0               = NewTemp(Error);
                        ConvertedOp0->VariableType = ResultType;
                        PSYMBOL ConvertedSymbol    = ToSymbol(ConvertedOp0, Error);
                        Symbol                     = NewSymbol();
                        Symbol->Type               = SYMBOL_SEMANTIC_RULE_TYPE;
                        Symbol->Value              = FUNC_CAST_SCALAR;
                        PushSymbol(CodeBuffer, Symbol);
                        PushSymbol(CodeBuffer, Op0Symbol);
                        PushSymbol(CodeBuffer, ConvertedSymbol);
                        Symbol->Type  = SYMBOL_NUM_TYPE;
                        Symbol->Value = GetScriptScalarTypeId((PVARIABLE_TYPE)Op0->VariableType);
                        PushSymbol(CodeBuffer, Symbol);
                        Symbol->Value = GetScriptScalarTypeId(ResultType);
                        PushSymbol(CodeBuffer, Symbol);
                        RemoveSymbol(&Symbol);
                        Op0Symbol = ConvertedSymbol;
                    }
                    if ((PVARIABLE_TYPE)Op1->VariableType != ResultType)
                    {
                        ConvertedOp1               = NewTemp(Error);
                        ConvertedOp1->VariableType = ResultType;
                        PSYMBOL ConvertedSymbol    = ToSymbol(ConvertedOp1, Error);
                        Symbol                     = NewSymbol();
                        Symbol->Type               = SYMBOL_SEMANTIC_RULE_TYPE;
                        Symbol->Value              = FUNC_CAST_SCALAR;
                        PushSymbol(CodeBuffer, Symbol);
                        PushSymbol(CodeBuffer, Op1Symbol);
                        PushSymbol(CodeBuffer, ConvertedSymbol);
                        Symbol->Type  = SYMBOL_NUM_TYPE;
                        Symbol->Value = GetScriptScalarTypeId((PVARIABLE_TYPE)Op1->VariableType);
                        PushSymbol(CodeBuffer, Symbol);
                        Symbol->Value = GetScriptScalarTypeId(ResultType);
                        PushSymbol(CodeBuffer, Symbol);
                        RemoveSymbol(&Symbol);
                        Op1Symbol = ConvertedSymbol;
                    }
                    Temp               = NewTemp(Error);
                    Temp->VariableType = IsFloatingComparisonOperator(Operator->Value) ? VARIABLE_TYPE_INT : ResultType;
                    TempSymbol         = ToSymbol(Temp, Error);

                    OperatorSymbol->Value = FloatingOpcode;
                    PushSymbol(CodeBuffer, OperatorSymbol);
                    PushSymbol(CodeBuffer, Op0Symbol);
                    PushSymbol(CodeBuffer, Op1Symbol);
                    PushSymbol(CodeBuffer, TempSymbol);
                    Push(MatchedStack, Temp);

                    FreeTemp(Op0);
                    FreeTemp(Op1);
                    if (ConvertedOp0)
                    {
                        FreeTemp(ConvertedOp0);
                        RemoveToken(&ConvertedOp0);
                    }
                    if (ConvertedOp1)
                    {
                        FreeTemp(ConvertedOp1);
                        RemoveToken(&ConvertedOp1);
                    }
                    Handled = TRUE;
                }
            }

            if (!Handled && (!strcmp(Operator->Value, "@ADD") || !strcmp(Operator->Value, "@SUB")))
            {
                PVARIABLE_TYPE Op0Type = (PVARIABLE_TYPE)Op0->VariableType;
                PVARIABLE_TYPE Op1Type = (PVARIABLE_TYPE)Op1->VariableType;

                if (Op0Type && Op1Type && Op0Type->Kind == TY_PTR && Op1Type->Kind == TY_PTR)
                {
                    Op0 = Pop(MatchedStack);
                    Op1 = Pop(MatchedStack);
                    if (!strcmp(Operator->Value, "@SUB") &&
                        ((PVARIABLE_TYPE)Op0->VariableType)->Base == ((PVARIABLE_TYPE)Op1->VariableType)->Base &&
                        ((PVARIABLE_TYPE)Op0->VariableType)->Base &&
                        ((PVARIABLE_TYPE)Op0->VariableType)->Base->Size > 0)
                    {
                        PSYMBOL TypeSymbol;
                        OperatorSymbol->Value = FUNC_POINTER_DIFF;
                        PushSymbol(CodeBuffer, OperatorSymbol);
                        Op0Symbol          = ToSymbol(Op0, Error);
                        Op1Symbol          = ToSymbol(Op1, Error);
                        Temp               = NewTemp(Error);
                        Temp->VariableType = VARIABLE_TYPE_LONG;
                        TempSymbol         = ToSymbol(Temp, Error);
                        PushSymbol(CodeBuffer, Op0Symbol);
                        PushSymbol(CodeBuffer, Op1Symbol);
                        PushSymbol(CodeBuffer, TempSymbol);
                        TypeSymbol        = NewSymbol();
                        TypeSymbol->Type  = SYMBOL_NUM_TYPE;
                        TypeSymbol->Value = ((PVARIABLE_TYPE)Op0->VariableType)->Base->Size;
                        PushSymbol(CodeBuffer, TypeSymbol);
                        RemoveSymbol(&TypeSymbol);
                        Push(MatchedStack, Temp);
                        FreeTemp(Op0);
                        FreeTemp(Op1);
                    }
                    else
                    {
                        *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                    }
                    Handled = TRUE;
                }
                else if (Op0Type && Op0Type->Kind == TY_PTR && (!Op1Type || Op1Type->Kind != TY_PTR))
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

                        Temp               = NewTemp(Error);
                        Temp->VariableType = (PVARIABLE_TYPE)Op0->VariableType;
                        TempSymbol         = ToSymbol(Temp, Error);
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
                else if ((!Op0Type || Op0Type->Kind != TY_PTR) && Op1Type && Op1Type->Kind == TY_PTR)
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

                    Temp               = NewTemp(Error);
                    Temp->VariableType = (PVARIABLE_TYPE)Op1->VariableType;
                    TempSymbol         = ToSymbol(Temp, Error);
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

                else if (Op0Type && Op1Type && Op0Type->Kind == TY_ARRAY && Op1Type->Kind == TY_ARRAY)
                {
                    *Error  = SCRIPT_ENGINE_ERROR_SYNTAX;
                    Op0     = Pop(MatchedStack);
                    Op1     = Pop(MatchedStack);
                    Handled = TRUE;
                }
                else if (Op0Type && Op0Type->Kind == TY_ARRAY && (!Op1Type || Op1Type->Kind != TY_ARRAY))
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

                        Temp               = NewTemp(Error);
                        Temp->VariableType = CreatePointerType(((PVARIABLE_TYPE)Op0->VariableType)->Base);
                        TempSymbol         = ToSymbol(Temp, Error);
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
                else if ((!Op0Type || Op0Type->Kind != TY_ARRAY) && Op1Type && Op1Type->Kind == TY_ARRAY)
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

                    Temp               = NewTemp(Error);
                    Temp->VariableType = CreatePointerType(((PVARIABLE_TYPE)Op1->VariableType)->Base);
                    TempSymbol         = ToSymbol(Temp, Error);
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

            if (!Handled && IsFloatingComparisonOperator(Operator->Value) &&
                Op0->VariableType && Op1->VariableType &&
                ((PVARIABLE_TYPE)Op0->VariableType)->Kind == TY_PTR &&
                ((PVARIABLE_TYPE)Op1->VariableType)->Kind == TY_PTR)
            {
                PSYMBOL TypeSymbol;
                if (((PVARIABLE_TYPE)Op0->VariableType)->Base != ((PVARIABLE_TYPE)Op1->VariableType)->Base &&
                    ((PVARIABLE_TYPE)Op0->VariableType)->Base != VARIABLE_TYPE_VOID &&
                    ((PVARIABLE_TYPE)Op1->VariableType)->Base != VARIABLE_TYPE_VOID)
                {
                    *Error = SCRIPT_ENGINE_ERROR_SYNTAX;
                    Op0    = Pop(MatchedStack);
                    Op1    = Pop(MatchedStack);
                }
                else
                {
                    Op0                   = Pop(MatchedStack);
                    Op1                   = Pop(MatchedStack);
                    OperatorSymbol->Value = GetTypedBinaryOpcode(Operator->Value);
                    PushSymbol(CodeBuffer, OperatorSymbol);
                    Op0Symbol          = ToSymbol(Op0, Error);
                    Op1Symbol          = ToSymbol(Op1, Error);
                    Temp               = NewTemp(Error);
                    Temp->VariableType = VARIABLE_TYPE_INT;
                    TempSymbol         = ToSymbol(Temp, Error);
                    PushSymbol(CodeBuffer, Op0Symbol);
                    PushSymbol(CodeBuffer, Op1Symbol);
                    PushSymbol(CodeBuffer, TempSymbol);
                    TypeSymbol        = NewSymbol();
                    TypeSymbol->Type  = SYMBOL_NUM_TYPE;
                    TypeSymbol->Value = SCRIPT_SCALAR_TYPE_POINTER;
                    PushSymbol(CodeBuffer, TypeSymbol);
                    RemoveSymbol(&TypeSymbol);
                    Push(MatchedStack, Temp);
                    FreeTemp(Op0);
                    FreeTemp(Op1);
                }
                Handled = TRUE;
            }

            if (!Handled)
            {
                Op0       = Pop(MatchedStack);
                Op0Symbol = ToSymbol(Op0, Error);

                Op1       = Pop(MatchedStack);
                Op1Symbol = ToSymbol(Op1, Error);

                PVARIABLE_TYPE CommonType            = GetCommonVariableType((PVARIABLE_TYPE)Op0->VariableType,
                                                                             (PVARIABLE_TYPE)Op1->VariableType);
                UINT64         TypedOpcode           = GetTypedBinaryOpcode(Operator->Value);
                BOOLEAN        TypedIntegerOperation = IsIntegerVariableType(CommonType) && TypedOpcode != FUNC_UNDEFINED;
                if (TypedIntegerOperation)
                    OperatorSymbol->Value = TypedOpcode;
                PushSymbol(CodeBuffer, OperatorSymbol);

                Temp               = NewTemp(Error);
                Temp->VariableType = IsFloatingComparisonOperator(Operator->Value) ? VARIABLE_TYPE_INT : CommonType;
                Push(MatchedStack, Temp);
                TempSymbol = ToSymbol(Temp, Error);

                PushSymbol(CodeBuffer, Op0Symbol);
                PushSymbol(CodeBuffer, Op1Symbol);
                PushSymbol(CodeBuffer, TempSymbol);
                if (TypedIntegerOperation)
                {
                    PSYMBOL TypeSymbol = NewSymbol();
                    TypeSymbol->Type   = SYMBOL_NUM_TYPE;
                    TypeSymbol->Value  = GetScriptScalarTypeId(CommonType);
                    PushSymbol(CodeBuffer, TypeSymbol);
                    RemoveSymbol(&TypeSymbol);
                }

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

            Temp               = NewTemp(Error);
            Temp->VariableType = GetDefaultImplicitVariableType();
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

            Temp               = NewTemp(Error);
            Temp->VariableType = GetDefaultImplicitVariableType();
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

            Temp               = NewTemp(Error);
            Temp->VariableType = GetDefaultImplicitVariableType();
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

            Temp               = NewTemp(Error);
            Temp->VariableType = GetDefaultImplicitVariableType();
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

            Temp               = NewTemp(Error);
            Temp->VariableType = GetDefaultImplicitVariableType();
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

            Temp               = NewTemp(Error);
            Temp->VariableType = GetDefaultImplicitVariableType();
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
    CHAR         CTemp;

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
            CTemp        = *c;

            CurrentIn = Scan(str, c);
            if (InputIdx - 1 > BooleanExpressionSize)
            {
                InputIdx = InputIdxTemp;
                *c       = CTemp;

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
                    CodeGen(MatchedStack, CodeBuffer, SemanticRule, Error, &str);
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
    ResolveIdentifierVariableType(Token);
    switch (Token->Type)
    {
    case GLOBAL_ID:
        Symbol->Value = GetGlobalIdentifierVal(Token);
        SetType(&Symbol->Type, SYMBOL_GLOBAL_ID_TYPE);
        return Symbol;

    case LOCAL_ID:
    {
        Symbol->Value = GetLocalIdentifierVal(Token);
        Symbol->Len   = GetFloatingValueKind((PVARIABLE_TYPE)Token->VariableType);

        if (((VARIABLE_TYPE *)Token->VariableType)->Kind == TY_ARRAY ||
            ((VARIABLE_TYPE *)Token->VariableType)->Kind == TY_STRUCT)
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

    case FLOAT_LITERAL:
        if (!ParseFloatingLiteral(Token->Value, SYMBOL_VALUE_KIND_FLOAT64, &Symbol->Value, Error))
        {
            Symbol->Type = INVALID;
            return Symbol;
        }
        Symbol->Len = SYMBOL_VALUE_KIND_FLOAT64;
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
        Symbol->Len   = GetFloatingValueKind((PVARIABLE_TYPE)Token->VariableType);

        if (Token->IsAddress)
        {
            SetType(&Symbol->Type, SYMBOL_TEMP_TYPE);
        }
        else if (((VARIABLE_TYPE *)Token->VariableType)->Kind == TY_ARRAY ||
                 ((VARIABLE_TYPE *)Token->VariableType)->Kind == TY_STRUCT)
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
            INT Temp = GetSymbolHeapSize(Symbol);
            i += Temp;
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
    case SCRIPT_ENGINE_ERROR_UNKNOWN_STRUCT_TAG:
        strcat(Message, "Unknown struct tag");
        return Message;
    case SCRIPT_ENGINE_ERROR_INCOMPLETE_TYPE:
        strcat(Message, "Incomplete struct type cannot be used by value");
        return Message;
    case SCRIPT_ENGINE_ERROR_DUPLICATE_STRUCT_DEFINITION:
        strcat(Message, "Duplicate struct definition");
        return Message;
    case SCRIPT_ENGINE_ERROR_DUPLICATE_STRUCT_MEMBER:
        strcat(Message, "Duplicate struct member");
        return Message;
    case SCRIPT_ENGINE_ERROR_DUPLICATE_TYPEDEF:
        strcat(Message, "Duplicate typedef name");
        return Message;
    case SCRIPT_ENGINE_ERROR_INVALID_ARRAY_SIZE:
        strcat(Message, "Invalid or overflowing array size");
        return Message;
    case SCRIPT_ENGINE_ERROR_INVALID_FLOAT_LITERAL:
        strcat(Message, "Invalid floating-point literal");
        return Message;
    case SCRIPT_ENGINE_ERROR_FLOAT_OUT_OF_RANGE:
        strcat(Message, "Floating-point literal is out of range");
        return Message;
    case SCRIPT_ENGINE_ERROR_UNSUPPORTED_FLOAT_OPERATION:
        strcat(Message, "Unsupported floating-point operation");
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
SetGlobalIdentifierVariableType(PSCRIPT_ENGINE_TOKEN Token, VARIABLE_TYPE * VariableType)
{
    PSCRIPT_ENGINE_TOKEN CurrentToken;
    for (uintptr_t i = 0; i < GlobalIdTable->Pointer; i++)
    {
        CurrentToken = *(GlobalIdTable->Head + i);
        if (!strcmp(Token->Value, CurrentToken->Value))
        {
            CurrentToken->VariableType = (VARIABLE_TYPE *)VariableType;
        }
    }
}

/**
 * @brief
 *
 * @param Token
 */
VARIABLE_TYPE *
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

BOOLEAN
GetGlobalIdentifierIsImplicitType(PSCRIPT_ENGINE_TOKEN Token)
{
    PSCRIPT_ENGINE_TOKEN CurrentToken;
    for (uintptr_t i = 0; i < GlobalIdTable->Pointer; i++)
    {
        CurrentToken = *(GlobalIdTable->Head + i);
        if (!strcmp(Token->Value, CurrentToken->Value))
        {
            return CurrentToken->IsImplicitType;
        }
    }
    return FALSE;
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
SetLocalIdentifierVariableType(PSCRIPT_ENGINE_TOKEN Token, VARIABLE_TYPE * VariableType)
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
 * @return VARIABLE_TYPE*
 */
VARIABLE_TYPE *
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

BOOLEAN
GetLocalIdentifierIsImplicitType(PSCRIPT_ENGINE_TOKEN Token)
{
    PSCRIPT_ENGINE_TOKEN CurrentToken;
    for (uintptr_t i = 0; i < ((PSCRIPT_ENGINE_TOKEN_LIST)CurrentUserDefinedFunction->IdTable)->Pointer; i++)
    {
        CurrentToken = *(((PSCRIPT_ENGINE_TOKEN_LIST)CurrentUserDefinedFunction->IdTable)->Head + i);
        if (!strcmp(Token->Value, CurrentToken->Value))
        {
            return CurrentToken->IsImplicitType;
        }
    }
    return FALSE;
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
 * @return PUSER_DEFINED_FUNCTION_NODE
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
    else if (Token->Type == SCRIPT_VARIABLE_TYPE)
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
    else if (Token->Type == FLOAT_LITERAL)
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
    case FUNC_CAST_SCALAR:

        *NumberOfGetOperands = 3;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_ADD_TYPED:
    case FUNC_SUB_TYPED:
    case FUNC_MUL_TYPED:
    case FUNC_DIV_TYPED:
    case FUNC_MOD_TYPED:
    case FUNC_BITWISE_AND_TYPED:
    case FUNC_BITWISE_OR_TYPED:
    case FUNC_BITWISE_XOR_TYPED:
    case FUNC_SHIFT_LEFT_TYPED:
    case FUNC_SHIFT_RIGHT_TYPED:
    case FUNC_GT_TYPED:
    case FUNC_LT_TYPED:
    case FUNC_EGT_TYPED:
    case FUNC_ELT_TYPED:
    case FUNC_EQUAL_TYPED:
    case FUNC_NEQ_TYPED:
    case FUNC_POINTER_DIFF:

        *NumberOfGetOperands = 3;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_NEG_TYPED:
    case FUNC_BITWISE_NOT_TYPED:
    case FUNC_LOGICAL_NOT_TYPED:

        *NumberOfGetOperands = 2;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_TYPED_LOAD:

        *NumberOfGetOperands = 3;
        *NumberOfSetOperands = 1;
        Result               = TRUE;

        break;

    case FUNC_TYPED_STORE:

        *NumberOfGetOperands = 4;
        *NumberOfSetOperands = 0;
        Result               = TRUE;

        break;

    case FUNC_AGGREGATE_COPY:

        *NumberOfGetOperands = 5;
        *NumberOfSetOperands = 0;
        Result               = TRUE;

        break;

    case FUNC_AGGREGATE_ZERO:

        *NumberOfGetOperands = 3;
        *NumberOfSetOperands = 0;
        Result               = TRUE;

        break;

        //
        // This code is not tested yet, so they are commented out for now
        //
        // case FUNC_MOV_FLOAT:
        // case FUNC_NEG_FLOAT:
        // case FUNC_CONVERT_FLOAT:
        //    *NumberOfGetOperands = 1;
        //    *NumberOfSetOperands = 1;
        //    Result               = TRUE;
        //    break;

        // case FUNC_ADD_FLOAT:
        // case FUNC_SUB_FLOAT:
        // case FUNC_MUL_FLOAT:
        // case FUNC_DIV_FLOAT:
        // case FUNC_GT_FLOAT:
        // case FUNC_LT_FLOAT:
        // case FUNC_EGT_FLOAT:
        // case FUNC_ELT_FLOAT:
        // case FUNC_EQUAL_FLOAT:
        // case FUNC_NEQ_FLOAT:
        //     *NumberOfGetOperands = 2;
        //     *NumberOfSetOperands = 1;
        //     Result               = TRUE;
        //     break;

        // case FUNC_TYPED_LOAD:
        //     *NumberOfGetOperands = 3;
        //     *NumberOfSetOperands = 1;
        //     Result               = TRUE;
        //     break;

        // case FUNC_TYPED_STORE:
        //     *NumberOfGetOperands = 4;
        //     *NumberOfSetOperands = 0;
        //     Result               = TRUE;
        //     break;

        // case FUNC_AGGREGATE_COPY:
        //     *NumberOfGetOperands = 5;
        //     *NumberOfSetOperands = 0;
        //     Result               = TRUE;
        //     break;

        // case FUNC_AGGREGATE_ZERO:
        //     *NumberOfGetOperands = 3;
        //     *NumberOfSetOperands = 0;
        //     Result               = TRUE;
        //     break;

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
