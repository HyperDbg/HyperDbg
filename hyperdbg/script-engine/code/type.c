#include "pch.h"

VARIABLE_TYPE * VARIABLE_TYPE_UNKNOWN = &(VARIABLE_TYPE) {TY_UNKNOWN};

VARIABLE_TYPE * VARIABLE_TYPE_VOID = &(VARIABLE_TYPE) {TY_VOID, 1, 1};
VARIABLE_TYPE * VARIABLE_TYPE_BOOL = &(VARIABLE_TYPE) {TY_BOOL, 1, 1};

VARIABLE_TYPE * VARIABLE_TYPE_CHAR  = &(VARIABLE_TYPE) {TY_CHAR, 1, 1};
VARIABLE_TYPE * VARIABLE_TYPE_SHORT = &(VARIABLE_TYPE) {TY_SHORT, 2, 2};
VARIABLE_TYPE * VARIABLE_TYPE_INT   = &(VARIABLE_TYPE) {TY_INT, 4, 4};
VARIABLE_TYPE * VARIABLE_TYPE_LONG  = &(VARIABLE_TYPE) {TY_LONG, 8, 8};

VARIABLE_TYPE * VARIABLE_TYPE_UCHAR  = &(VARIABLE_TYPE) {TY_CHAR, 1, 1, TRUE};
VARIABLE_TYPE * VARIABLE_TYPE_USHORT = &(VARIABLE_TYPE) {TY_SHORT, 2, 2, TRUE};
VARIABLE_TYPE * VARIABLE_TYPE_UINT   = &(VARIABLE_TYPE) {TY_INT, 4, 4, TRUE};
VARIABLE_TYPE * VARIABLE_TYPE_ULONG  = &(VARIABLE_TYPE) {TY_LONG, 8, 8, TRUE};

VARIABLE_TYPE * VARIABLE_TYPE_FLOAT   = &(VARIABLE_TYPE) {TY_FLOAT, 4, 4};
VARIABLE_TYPE * VARIABLE_TYPE_DOUBLE  = &(VARIABLE_TYPE) {TY_DOUBLE, 8, 8};
VARIABLE_TYPE * VARIABLE_TYPE_LDOUBLE = &(VARIABLE_TYPE) {TY_LDOUBLE, 16, 16};

/**
 * @brief Return a variable type
 *
 * @param str
 */
VARIABLE_TYPE *
HandleType(PTOKEN_LIST PtokenStack)
{
    enum
    {
        ENUM_VOID     = 1 << 0,
        ENUM_BOOL     = 1 << 2,
        ENUM_CHAR     = 1 << 4,
        ENUM_SHORT    = 1 << 6,
        ENUM_INT      = 1 << 8,
        ENUM_LONG     = 1 << 10,
        ENUM_FLOAT    = 1 << 12,
        ENUM_DOUBLE   = 1 << 14,
        ENUM_OTHER    = 1 << 16,
        ENUM_SIGNED   = 1 << 17,
        ENUM_UNSIGNED = 1 << 18,
    };

    VARIABLE_TYPE * Result   = VARIABLE_TYPE_UNKNOWN;
    int             Counter  = 0;
    PTOKEN          TopToken = NULL;

    while (PtokenStack->Pointer > 0)
    {
        TopToken = Pop(PtokenStack);
        if (TopToken->Type != INPUT_VARIABLE_TYPE)
        {
            Push(PtokenStack, TopToken);
            break;
        }
        if (!strcmp(TopToken->Value, "@VOID"))
        {
            Counter += ENUM_VOID;
        }
        else if (!strcmp(TopToken->Value, "@BOOL"))
        {
            Counter += ENUM_BOOL;
        }
        else if (!strcmp(TopToken->Value, "@CHAR"))
        {
            Counter += ENUM_CHAR;
        }
        else if (!strcmp(TopToken->Value, "@SHORT"))
        {
            Counter += ENUM_SHORT;
        }
        else if (!strcmp(TopToken->Value, "@INT"))
        {
            Counter += ENUM_INT;
        }
        else if (!strcmp(TopToken->Value, "@LONG"))
        {
            Counter += ENUM_LONG;
        }
        else if (!strcmp(TopToken->Value, "@FLOAT"))
        {
            Counter += ENUM_FLOAT;
        }
        else if (!strcmp(TopToken->Value, "@DOUBLE"))
        {
            Counter += ENUM_DOUBLE;
        }
        else if (!strcmp(TopToken->Value, "@SIGNED"))
        {
            Counter |= ENUM_SIGNED;
        }
        else if (!strcmp(TopToken->Value, "@UNSIGNED"))
        {
            Counter |= ENUM_UNSIGNED;
        }
        else
        {
            return VARIABLE_TYPE_UNKNOWN;
        }
        RemoveToken(&TopToken);

        switch (Counter)
        {
        case ENUM_VOID:
            Result = VARIABLE_TYPE_VOID;
            break;
        case ENUM_BOOL:
            Result = VARIABLE_TYPE_BOOL;
            break;
        case ENUM_CHAR:
        case ENUM_SIGNED + ENUM_CHAR:
            Result = VARIABLE_TYPE_CHAR;
            break;
        case ENUM_UNSIGNED + ENUM_CHAR:
            Result = VARIABLE_TYPE_UCHAR;
            break;
        case ENUM_SHORT:
        case ENUM_SHORT + ENUM_INT:
        case ENUM_SIGNED + ENUM_SHORT:
        case ENUM_SIGNED + ENUM_SHORT + ENUM_INT:
            Result = VARIABLE_TYPE_INT;
            break;
        case ENUM_UNSIGNED + ENUM_SHORT:
        case ENUM_UNSIGNED + ENUM_SHORT + ENUM_INT:
            Result = VARIABLE_TYPE_USHORT;
            break;
        case ENUM_INT:
        case ENUM_SIGNED:
        case ENUM_SIGNED + ENUM_INT:
            Result = VARIABLE_TYPE_INT;
            break;
        case ENUM_UNSIGNED:
        case ENUM_UNSIGNED + ENUM_INT:
            Result = VARIABLE_TYPE_UINT;
            break;
        case ENUM_LONG:
        case ENUM_LONG + ENUM_INT:
        case ENUM_LONG + ENUM_LONG:
        case ENUM_LONG + ENUM_LONG + ENUM_INT:
        case ENUM_SIGNED + ENUM_LONG:
        case ENUM_SIGNED + ENUM_LONG + ENUM_INT:
        case ENUM_SIGNED + ENUM_LONG + ENUM_LONG:
        case ENUM_SIGNED + ENUM_LONG + ENUM_LONG + ENUM_INT:
            Result = VARIABLE_TYPE_LONG;
            break;
        case ENUM_UNSIGNED + ENUM_LONG:
        case ENUM_UNSIGNED + ENUM_LONG + ENUM_INT:
        case ENUM_UNSIGNED + ENUM_LONG + ENUM_LONG:
        case ENUM_UNSIGNED + ENUM_LONG + ENUM_LONG + ENUM_INT:
            Result = VARIABLE_TYPE_ULONG;
            break;
        case ENUM_FLOAT:
            Result = VARIABLE_TYPE_FLOAT;
            break;
        case ENUM_DOUBLE:
            Result = VARIABLE_TYPE_DOUBLE;
            break;
        case ENUM_LONG + ENUM_DOUBLE:
            Result = VARIABLE_TYPE_LDOUBLE;
            break;
        }
    }
    return Result;
}