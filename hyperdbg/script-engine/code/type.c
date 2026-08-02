/**
 * @file type.c
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 *
 * @brief Routines for handling variable types
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

typedef struct _TYPE_ALLOCATION_NODE
{
    PVARIABLE_TYPE                Type;
    struct _TYPE_ALLOCATION_NODE * Next;
} TYPE_ALLOCATION_NODE, *PTYPE_ALLOCATION_NODE;

typedef struct _STRUCT_TAG_NODE
{
    char *                    Name;
    PVARIABLE_TYPE            Type;
    struct _STRUCT_TAG_NODE * Next;
} STRUCT_TAG_NODE, *PSTRUCT_TAG_NODE;

typedef struct _TYPEDEF_NODE
{
    char *                Name;
    PVARIABLE_TYPE        Type;
    struct _TYPEDEF_NODE * Next;
} TYPEDEF_NODE, *PTYPEDEF_NODE;

static PTYPE_ALLOCATION_NODE TypeAllocations;
static PSTRUCT_TAG_NODE      StructTags;
static PTYPEDEF_NODE         Typedefs;

static PVARIABLE_TYPE
AllocateType(VOID)
{
    PVARIABLE_TYPE Type = (PVARIABLE_TYPE)calloc(1, sizeof(VARIABLE_TYPE));
    PTYPE_ALLOCATION_NODE Node;

    if (!Type)
    {
        return NULL;
    }

    Node = (PTYPE_ALLOCATION_NODE)calloc(1, sizeof(TYPE_ALLOCATION_NODE));
    if (!Node)
    {
        free(Type);
        return NULL;
    }

    Node->Type      = Type;
    Node->Next      = TypeAllocations;
    TypeAllocations = Node;
    return Type;
}

static unsigned int
AlignTo(unsigned int Value, unsigned int Alignment)
{
    return (Value + Alignment - 1) & ~(Alignment - 1);
}

VOID
InitializeTypeContext(VOID)
{
    TypeAllocations = NULL;
    StructTags      = NULL;
    Typedefs        = NULL;
}

VOID
UninitializeTypeContext(VOID)
{
    while (Typedefs)
    {
        PTYPEDEF_NODE Next = Typedefs->Next;
        free(Typedefs->Name);
        free(Typedefs);
        Typedefs = Next;
    }

    while (StructTags)
    {
        PSTRUCT_TAG_NODE Next = StructTags->Next;
        free(StructTags->Name);
        free(StructTags);
        StructTags = Next;
    }

    while (TypeAllocations)
    {
        PTYPE_ALLOCATION_NODE Next = TypeAllocations->Next;
        PSTRUCT_MEMBER Member = TypeAllocations->Type->Members;
        while (Member)
        {
            PSTRUCT_MEMBER NextMember = Member->Next;
            free(Member->Name);
            free(Member);
            Member = NextMember;
        }
        free(TypeAllocations->Type->TagName);
        free(TypeAllocations->Type);
        free(TypeAllocations);
        TypeAllocations = Next;
    }
}

PVARIABLE_TYPE
FindStructType(const char * TagName)
{
    PSTRUCT_TAG_NODE Node;
    for (Node = StructTags; Node; Node = Node->Next)
    {
        if (!strcmp(Node->Name, TagName))
        {
            return Node->Type;
        }
    }
    return NULL;
}

PVARIABLE_TYPE
DeclareStructType(const char * TagName)
{
    PVARIABLE_TYPE Existing = FindStructType(TagName);
    PSTRUCT_TAG_NODE Node;
    PVARIABLE_TYPE Type;

    if (Existing)
    {
        return Existing;
    }

    Type = AllocateType();
    Node = (PSTRUCT_TAG_NODE)calloc(1, sizeof(STRUCT_TAG_NODE));
    if (!Type || !Node)
    {
        free(Node);
        return NULL;
    }

    Type->Kind    = TY_STRUCT;
    Type->TagName = PlatformStrDup(TagName);
    Node->Name    = PlatformStrDup(TagName);
    Node->Type    = Type;
    Node->Next    = StructTags;
    StructTags    = Node;
    return Type;
}

BOOLEAN
AddStructMember(PVARIABLE_TYPE StructType, const char * Name, PVARIABLE_TYPE MemberType)
{
    PSTRUCT_MEMBER Member;
    PSTRUCT_MEMBER * Tail;
    unsigned int Order = 0;

    if (!StructType || StructType->Kind != TY_STRUCT || StructType->IsComplete)
    {
        return FALSE;
    }

    Tail = &StructType->Members;
    while (*Tail)
    {
        if (!strcmp((*Tail)->Name, Name))
        {
            return FALSE;
        }
        Order++;
        Tail = &(*Tail)->Next;
    }

    Member = (PSTRUCT_MEMBER)calloc(1, sizeof(STRUCT_MEMBER));
    if (!Member)
    {
        return FALSE;
    }
    Member->Name             = PlatformStrDup(Name);
    Member->Type             = MemberType;
    Member->DeclarationOrder = Order;
    *Tail                    = Member;
    return TRUE;
}

PSTRUCT_MEMBER
FindStructMember(PVARIABLE_TYPE StructType, const char * Name)
{
    PSTRUCT_MEMBER Member;

    if (!StructType || StructType->Kind != TY_STRUCT)
    {
        return NULL;
    }

    for (Member = StructType->Members; Member; Member = Member->Next)
    {
        if (!strcmp(Member->Name, Name))
        {
            return Member;
        }
    }

    return NULL;
}

BOOLEAN
CompleteStructType(PVARIABLE_TYPE StructType)
{
    unsigned int Offset = 0;
    unsigned int Alignment = 1;
    PSTRUCT_MEMBER Member;

    if (!StructType || StructType->Kind != TY_STRUCT || StructType->IsComplete)
    {
        return FALSE;
    }

    for (Member = StructType->Members; Member; Member = Member->Next)
    {
        if (!Member->Type || !Member->Type->Align ||
            (Member->Type->Kind == TY_STRUCT && !Member->Type->IsComplete))
        {
            return FALSE;
        }
        if (Offset > 0x7fffffffU - ((unsigned int)Member->Type->Align - 1))
        {
            return FALSE;
        }
        Offset         = AlignTo(Offset, (unsigned int)Member->Type->Align);
        Member->Offset = Offset;
        if ((unsigned int)Member->Type->Size > 0x7fffffffU - Offset)
        {
            return FALSE;
        }
        Offset += (unsigned int)Member->Type->Size;
        if ((unsigned int)Member->Type->Align > Alignment)
        {
            Alignment = (unsigned int)Member->Type->Align;
        }
    }

    if (StructType->Members && Offset > 0x7fffffffU - (Alignment - 1))
    {
        return FALSE;
    }
    StructType->Align      = (int)Alignment;
    StructType->Size       = StructType->Members ? (int)AlignTo(Offset, Alignment) : 1;
    StructType->IsComplete = TRUE;
    return TRUE;
}

PVARIABLE_TYPE
CreatePointerType(PVARIABLE_TYPE BaseType)
{
    PVARIABLE_TYPE Type = AllocateType();
    if (Type)
    {
        Type->Kind       = TY_PTR;
        Type->Size       = 8;
        Type->Align      = 8;
        Type->IsUnsigned = TRUE;
        Type->Base       = BaseType;
        Type->IsComplete = TRUE;
    }
    return Type;
}

PVARIABLE_TYPE
CreateArrayType(PVARIABLE_TYPE BaseType, unsigned int ArrayLength)
{
    PVARIABLE_TYPE Type;
    if (!BaseType || !ArrayLength ||
        (BaseType->Kind == TY_STRUCT && !BaseType->IsComplete) ||
        (unsigned int)BaseType->Size > 0x7fffffffU / ArrayLength)
    {
        return NULL;
    }

    Type = AllocateType();
    if (Type)
    {
        Type->Kind       = TY_ARRAY;
        Type->Size       = BaseType->Size * (int)ArrayLength;
        Type->Align      = BaseType->Align;
        Type->Base       = BaseType;
        Type->ArrayLen   = (int)ArrayLength;
        Type->IsComplete = TRUE;
    }
    return Type;
}

BOOLEAN
AddTypedefType(const char * Name, PVARIABLE_TYPE Type)
{
    PTYPEDEF_NODE Node;
    for (Node = Typedefs; Node; Node = Node->Next)
    {
        if (!strcmp(Node->Name, Name))
        {
            return FALSE;
        }
    }

    Node = (PTYPEDEF_NODE)calloc(1, sizeof(TYPEDEF_NODE));
    if (!Node)
    {
        return FALSE;
    }
    Node->Name = PlatformStrDup(Name);
    Node->Type = Type;
    Node->Next = Typedefs;
    Typedefs   = Node;
    return TRUE;
}

PVARIABLE_TYPE
FindTypedefType(const char * Name)
{
    PTYPEDEF_NODE Node;
    for (Node = Typedefs; Node; Node = Node->Next)
    {
        if (!strcmp(Node->Name, Name))
        {
            return Node->Type;
        }
    }
    return NULL;
}

VARIABLE_TYPE * VARIABLE_TYPE_UNKNOWN = &(VARIABLE_TYPE) {.Kind = TY_UNKNOWN};

VARIABLE_TYPE * VARIABLE_TYPE_VOID = &(VARIABLE_TYPE) {.Kind = TY_VOID, .Size = 1, .Align = 1, .IsComplete = TRUE};
VARIABLE_TYPE * VARIABLE_TYPE_BOOL = &(VARIABLE_TYPE) {.Kind = TY_BOOL, .Size = 1, .Align = 1, .IsComplete = TRUE, .IntegerRank = INTEGER_RANK_BOOL};

VARIABLE_TYPE * VARIABLE_TYPE_CHAR   = &(VARIABLE_TYPE) {.Kind = TY_CHAR, .Size = 1, .Align = 1, .IsComplete = TRUE, .IntegerRank = INTEGER_RANK_CHAR};
VARIABLE_TYPE * VARIABLE_TYPE_SHORT  = &(VARIABLE_TYPE) {.Kind = TY_SHORT, .Size = 2, .Align = 2, .IsComplete = TRUE, .IntegerRank = INTEGER_RANK_SHORT};
VARIABLE_TYPE * VARIABLE_TYPE_INT    = &(VARIABLE_TYPE) {.Kind = TY_INT, .Size = 4, .Align = 4, .IsComplete = TRUE, .IntegerRank = INTEGER_RANK_INT};
VARIABLE_TYPE * VARIABLE_TYPE_LONG   = &(VARIABLE_TYPE) {.Kind = TY_LONG, .Size = 8, .Align = 8, .IsComplete = TRUE, .IntegerRank = INTEGER_RANK_LONG};
VARIABLE_TYPE * VARIABLE_TYPE_LLONG  = &(VARIABLE_TYPE) {.Kind = TY_LLONG, .Size = 8, .Align = 8, .IsComplete = TRUE, .IntegerRank = INTEGER_RANK_LONG_LONG};

VARIABLE_TYPE * VARIABLE_TYPE_UCHAR  = &(VARIABLE_TYPE) {.Kind = TY_CHAR, .Size = 1, .Align = 1, .IsUnsigned = TRUE, .IsComplete = TRUE, .IntegerRank = INTEGER_RANK_CHAR};
VARIABLE_TYPE * VARIABLE_TYPE_USHORT = &(VARIABLE_TYPE) {.Kind = TY_SHORT, .Size = 2, .Align = 2, .IsUnsigned = TRUE, .IsComplete = TRUE, .IntegerRank = INTEGER_RANK_SHORT};
VARIABLE_TYPE * VARIABLE_TYPE_UINT   = &(VARIABLE_TYPE) {.Kind = TY_INT, .Size = 4, .Align = 4, .IsUnsigned = TRUE, .IsComplete = TRUE, .IntegerRank = INTEGER_RANK_INT};
VARIABLE_TYPE * VARIABLE_TYPE_ULONG  = &(VARIABLE_TYPE) {.Kind = TY_LONG, .Size = 8, .Align = 8, .IsUnsigned = TRUE, .IsComplete = TRUE, .IntegerRank = INTEGER_RANK_LONG};
VARIABLE_TYPE * VARIABLE_TYPE_ULLONG = &(VARIABLE_TYPE) {.Kind = TY_LLONG, .Size = 8, .Align = 8, .IsUnsigned = TRUE, .IsComplete = TRUE, .IntegerRank = INTEGER_RANK_LONG_LONG};

VARIABLE_TYPE * VARIABLE_TYPE_FLOAT   = &(VARIABLE_TYPE) {.Kind = TY_FLOAT, .Size = 4, .Align = 4, .IsComplete = TRUE};
VARIABLE_TYPE * VARIABLE_TYPE_DOUBLE  = &(VARIABLE_TYPE) {.Kind = TY_DOUBLE, .Size = 8, .Align = 8, .IsComplete = TRUE};
VARIABLE_TYPE * VARIABLE_TYPE_LDOUBLE = &(VARIABLE_TYPE) {.Kind = TY_LDOUBLE, .Size = 16, .Align = 16, .IsComplete = TRUE};

/**
 * @brief Return a variable type based on the token stack
 *
 * @param PtokenStack the token stack containing type tokens
 * @return VARIABLE_TYPE * pointer to the resolved variable type
 */
VARIABLE_TYPE *
HandleType(PSCRIPT_ENGINE_TOKEN_LIST PtokenStack)
{
    unsigned int          VoidCount = 0, BoolCount = 0, CharCount = 0;
    unsigned int          ShortCount = 0, IntCount = 0, LongCount = 0;
    unsigned int          FloatCount = 0, DoubleCount = 0;
    unsigned int          SignedCount = 0, UnsignedCount = 0;
    PSCRIPT_ENGINE_TOKEN TopToken = NULL;

    while (PtokenStack->Pointer > 0)
    {
        TopToken = Pop(PtokenStack);
        if (TopToken->Type != SCRIPT_VARIABLE_TYPE)
        {
            Push(PtokenStack, TopToken);
            break;
        }
        {
            PVARIABLE_TYPE TypedefType = FindTypedefType(TopToken->Value);
            if (TypedefType)
            {
                RemoveToken(&TopToken);
                if (VoidCount || BoolCount || CharCount || ShortCount || IntCount || LongCount ||
                    FloatCount || DoubleCount || SignedCount || UnsignedCount ||
                    (PtokenStack->Pointer && Top(PtokenStack)->Type == SCRIPT_VARIABLE_TYPE))
                    return VARIABLE_TYPE_UNKNOWN;
                return TypedefType;
            }
        }
        if (!strcmp(TopToken->Value, "void"))
        {
            VoidCount++;
        }
        else if (!strcmp(TopToken->Value, "bool"))
        {
            BoolCount++;
        }
        else if (!strcmp(TopToken->Value, "char"))
        {
            CharCount++;
        }
        else if (!strcmp(TopToken->Value, "short"))
        {
            ShortCount++;
        }
        else if (!strcmp(TopToken->Value, "int"))
        {
            IntCount++;
        }
        else if (!strcmp(TopToken->Value, "long"))
        {
            LongCount++;
        }
        else if (!strcmp(TopToken->Value, "float"))
        {
            FloatCount++;
        }
        else if (!strcmp(TopToken->Value, "double"))
        {
            DoubleCount++;
        }
        else if (!strcmp(TopToken->Value, "signed"))
        {
            SignedCount++;
        }
        else if (!strcmp(TopToken->Value, "unsigned"))
        {
            UnsignedCount++;
        }
        else
        {
            return VARIABLE_TYPE_UNKNOWN;
        }
        RemoveToken(&TopToken);
    }

    if (SignedCount > 1 || UnsignedCount > 1 || (SignedCount && UnsignedCount) ||
        VoidCount > 1 || BoolCount > 1 || CharCount > 1 || ShortCount > 1 ||
        IntCount > 1 || LongCount > 2 || FloatCount > 1 || DoubleCount > 1)
    {
        return VARIABLE_TYPE_UNKNOWN;
    }

    if (VoidCount || BoolCount || CharCount || ShortCount || FloatCount || DoubleCount)
    {
        if (VoidCount && !(BoolCount || CharCount || ShortCount || IntCount || LongCount || FloatCount || DoubleCount || SignedCount || UnsignedCount))
            return VARIABLE_TYPE_VOID;
        if (BoolCount && !(VoidCount || CharCount || ShortCount || IntCount || LongCount || FloatCount || DoubleCount || SignedCount || UnsignedCount))
            return VARIABLE_TYPE_BOOL;
        if (CharCount && !(VoidCount || BoolCount || ShortCount || IntCount || LongCount || FloatCount || DoubleCount))
            return UnsignedCount ? VARIABLE_TYPE_UCHAR : VARIABLE_TYPE_CHAR;
        if (ShortCount && !(VoidCount || BoolCount || CharCount || LongCount || FloatCount || DoubleCount))
            return UnsignedCount ? VARIABLE_TYPE_USHORT : VARIABLE_TYPE_SHORT;
        if (FloatCount && !(VoidCount || BoolCount || CharCount || ShortCount || IntCount || LongCount || DoubleCount || SignedCount || UnsignedCount))
            return VARIABLE_TYPE_FLOAT;
        if (DoubleCount && !VoidCount && !BoolCount && !CharCount && !ShortCount && !IntCount && !FloatCount && !SignedCount && !UnsignedCount)
            return LongCount == 0 ? VARIABLE_TYPE_DOUBLE : VARIABLE_TYPE_UNKNOWN;
        return VARIABLE_TYPE_UNKNOWN;
    }

    if (LongCount)
    {
        if (VoidCount || BoolCount || CharCount || ShortCount || FloatCount || DoubleCount)
            return VARIABLE_TYPE_UNKNOWN;
        if (LongCount == 2)
            return UnsignedCount ? VARIABLE_TYPE_ULLONG : VARIABLE_TYPE_LLONG;
        return UnsignedCount ? VARIABLE_TYPE_ULONG : VARIABLE_TYPE_LONG;
    }

    if (IntCount || SignedCount || UnsignedCount)
        return UnsignedCount ? VARIABLE_TYPE_UINT : VARIABLE_TYPE_INT;

    return VARIABLE_TYPE_UNKNOWN;
}

/**
 * @brief Returns the common variable type between two types
 *
 * @param Ty1 the first variable type
 * @param Ty2 the second variable type
 * @return VARIABLE_TYPE * pointer to the common variable type
 */
VARIABLE_TYPE *
GetCommonVariableType(VARIABLE_TYPE * Ty1, VARIABLE_TYPE * Ty2)
{
    PVARIABLE_TYPE LeftType  = PromoteIntegerVariableType(Ty1);
    PVARIABLE_TYPE RightType = PromoteIntegerVariableType(Ty2);

    if (!IsIntegerVariableType(LeftType) || !IsIntegerVariableType(RightType))
        return VARIABLE_TYPE_UNKNOWN;
    if (LeftType == RightType)
        return LeftType;
    if (LeftType->IsUnsigned == RightType->IsUnsigned)
        return LeftType->IntegerRank >= RightType->IntegerRank ? LeftType : RightType;

    PVARIABLE_TYPE UnsignedType = LeftType->IsUnsigned ? LeftType : RightType;
    PVARIABLE_TYPE SignedType   = LeftType->IsUnsigned ? RightType : LeftType;
    if (UnsignedType->IntegerRank >= SignedType->IntegerRank)
        return UnsignedType;
    if (SignedType->Size > UnsignedType->Size)
        return SignedType;
    if (SignedType == VARIABLE_TYPE_LONG) return VARIABLE_TYPE_ULONG;
    if (SignedType == VARIABLE_TYPE_LLONG) return VARIABLE_TYPE_ULLONG;
    return VARIABLE_TYPE_UINT;
}

PVARIABLE_TYPE
GetDefaultImplicitVariableType(VOID)
{
    return VARIABLE_TYPE_ULLONG;
}

BOOLEAN
IsIntegerVariableType(PVARIABLE_TYPE Type)
{
    return Type && (Type->Kind == TY_BOOL || Type->Kind == TY_CHAR ||
                    Type->Kind == TY_SHORT || Type->Kind == TY_INT ||
                    Type->Kind == TY_LONG || Type->Kind == TY_LLONG ||
                    Type->Kind == TY_ENUM);
}

PVARIABLE_TYPE
PromoteIntegerVariableType(PVARIABLE_TYPE Type)
{
    if (!IsIntegerVariableType(Type))
        return Type;
    if (Type->IntegerRank < INTEGER_RANK_INT)
        return VARIABLE_TYPE_INT;
    return Type;
}
