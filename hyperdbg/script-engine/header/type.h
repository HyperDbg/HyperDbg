/**
 * @file type.h
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 *
 * @brief Variable type definitions for the script engine
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#ifndef TYPE_H
#    define TYPE_H

typedef enum _VARIABLE_TYPE_KIND
{
    TY_UNKNOWN,
    TY_VOID,
    TY_BOOL,
    TY_CHAR,
    TY_SHORT,
    TY_INT,
    TY_LONG,
    TY_FLOAT,
    TY_DOUBLE,
    TY_LDOUBLE,
    TY_ENUM,
    TY_PTR,
    TY_FUNC,
    TY_ARRAY,
    TY_VLA, // variable-length array
    TY_STRUCT,
    TY_UNION,
} VARIABLE_TYPE_KIND;

typedef struct _VARIABLE_TYPE
{
    VARIABLE_TYPE_KIND      Kind;
    int                     Size;  // sizeof() value
    int                     Align; // alignment
    BOOLEAN                 IsUnsigned;
    struct _VARIABLE_TYPE * Base;
    int                     ArrayLen;
} VARIABLE_TYPE, *PVARIABLE_TYPE;

extern VARIABLE_TYPE * VARIABLE_TYPE_UNKNOWN;

extern VARIABLE_TYPE * VARIABLE_TYPE_VOID;
extern VARIABLE_TYPE * VARIABLE_TYPE_BOOL;

extern VARIABLE_TYPE * VARIABLE_TYPE_CHAR;
extern VARIABLE_TYPE * VARIABLE_TYPE_SHORT;
extern VARIABLE_TYPE * VARIABLE_TYPE_INT;
extern VARIABLE_TYPE * VARIABLE_TYPE_LONG;

extern VARIABLE_TYPE * VARIABLE_TYPE_UCHAR;
extern VARIABLE_TYPE * VARIABLE_TYPE_USHORT;
extern VARIABLE_TYPE * VARIABLE_TYPE_UINT;
extern VARIABLE_TYPE * VARIABLE_TYPE_ULONG;

extern VARIABLE_TYPE * VARIABLE_TYPE_FLOAT;
extern VARIABLE_TYPE * VARIABLE_TYPE_DOUBLE;
extern VARIABLE_TYPE * VARIABLE_TYPE_LDOUBLE;

#endif
