/**
 * @file casting.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief casting functions queries for script engine
 * @details
 * @version 0.1
 * @date 2022-02-04
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\symbol-parser\pch.h"

/*
 Grammar :

      EXPRSSION-> cast < STRING0 > ( EXPRESSION ) STRING_SEQUENCE
      STRING_SEQUENCE->.STRING[1 - N] STRING_SEQUENCE
      STRING_SEQUENCE->'->' STRING STRING_SEQUENCE
      STRING_SEQUENCE->eps

 */

typedef struct _UNICODE_STRING
{
    USHORT Length;        // +0x000
    USHORT MaximumLength; // +0x002
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

/**
 * @brief This function returns the needed details for making support
 * for the casting in the script engine
 *
 * @param StructName Top-level name of struct to perform the look up on this
 * struct
 * @param FiledOfStructName The field name (a field of struct)
 * @param IsItPointerOrNot Shows whether the field specified in
 * FiledOfStructName is a pointer or not
 * @param NewStructOrTypeName Returns the type (structure name) of the
 * FiledOfStructName for future (next '->' or '.' )
 * @param OffsetOfFieldFromTop The start position of this field from the top of
 * structure
 * @param SizeOfField The exact size of the target field
 *
 * @return BOOLEAN if the StructName or the FiledOfStructName not found or there
 * was any error it returns FALSE (in that case script engine should throw an
 * error to the user) otherwise return TRUE which means the script engine can
 * safely use the details
 */
BOOLEAN
SymCastingQueryForFiledsAndTypes(_In_ const char * StructName,
                                 _In_ const char * FiledOfStructName,
                                 _Out_ PBOOLEAN    IsItPointerOrNot,
                                 _Out_ char **     NewStructOrTypeName,
                                 _Out_ UINT32 * OffsetOfFieldFromTop,
                                 _Out_ UINT32 * SizeOfField)
{
    BOOLEAN IsPointer                = FALSE;
    UINT32  TempOffsetOfFieldFromTop = 0;
    UINT32  TempSizeOfField          = 0;

    if (strcmp(StructName, "STUPID_STRUCT1") == 0 ||
        strcmp(StructName, "PSTUPID_STRUCT1") == 0)
    {
        if (strcmp(FiledOfStructName, "Flag32") == 0)
        {
            IsPointer                = FALSE;
            TempOffsetOfFieldFromTop = 0x0;
            TempSizeOfField          = 0x4;
            strcpy(*NewStructOrTypeName, "UINT32");
        }
        else if (strcmp(FiledOfStructName, "Flag64") == 0)
        {
            IsPointer                = FALSE;
            TempOffsetOfFieldFromTop = 0x4;
            TempSizeOfField          = 0x8;
            strcpy(*NewStructOrTypeName, "UINT64");
        }
        else if (strcmp(FiledOfStructName, "Context") == 0)
        {
            IsPointer                = TRUE;
            TempOffsetOfFieldFromTop = 0xc;
            TempSizeOfField          = 0x8;
            strcpy(*NewStructOrTypeName, "PVOID");
        }
        else if (strcmp(FiledOfStructName, "StringValue") == 0)
        {
            IsPointer                = TRUE;
            TempOffsetOfFieldFromTop = 0x14;
            TempSizeOfField          = 0x8;
            strcpy(*NewStructOrTypeName, "PUNICODE_STRING");
        }
        else
        {
            //
            // Unknown Field
            //
            return FALSE;
        }
    }
    else if (strcmp(StructName, "STUPID_STRUCT2") == 0 ||
             strcmp(StructName, "PSTUPID_STRUCT2") == 0)
    {
        if (strcmp(FiledOfStructName, "Sina32") == 0)
        {
            IsPointer                = FALSE;
            TempOffsetOfFieldFromTop = 0x0;
            TempSizeOfField          = 0x4;
            strcpy(*NewStructOrTypeName, "UINT32");
        }
        else if (strcmp(FiledOfStructName, "Sina64") == 0)
        {
            IsPointer                = FALSE;
            TempOffsetOfFieldFromTop = 0x4;
            TempSizeOfField          = 0x8;
            strcpy(*NewStructOrTypeName, "UINT64");
        }
        else if (strcmp(FiledOfStructName, "AghaaSina") == 0)
        {
            IsPointer                = TRUE;
            TempOffsetOfFieldFromTop = 0xc;
            TempSizeOfField          = 0x8;
            strcpy(*NewStructOrTypeName, "PVOID");
        }
        else if (strcmp(FiledOfStructName, "UnicodeStr") == 0)
        {
            IsPointer                = TRUE;
            TempOffsetOfFieldFromTop = 0x14;
            TempSizeOfField          = 0x8;
            strcpy(*NewStructOrTypeName, "PUNICODE_STRING");
        }
        else if (strcmp(FiledOfStructName, "StupidStruct1") == 0)
        {
            IsPointer                = TRUE;
            TempOffsetOfFieldFromTop = 0x1c;
            TempSizeOfField          = 0x8;
            strcpy(*NewStructOrTypeName, "PSTUPID_STRUCT1");
        }
        else
        {
            //
            // Unknown Field
            //
            return FALSE;
        }
    }
    else if (strcmp(StructName, "UNICODE_STRING") == 0 ||
             strcmp(StructName, "PUNICODE_STRING") == 0)
    {
        if (strcmp(FiledOfStructName, "Length") == 0)
        {
            IsPointer                = FALSE;
            TempOffsetOfFieldFromTop = 0x0;
            TempSizeOfField          = 0x2;
            strcpy(*NewStructOrTypeName, "USHORT");
        }
        else if (strcmp(FiledOfStructName, "MaximumLength") == 0)
        {
            IsPointer                = FALSE;
            TempOffsetOfFieldFromTop = 0x2;
            TempSizeOfField          = 0x2;
            strcpy(*NewStructOrTypeName, "USHORT");
        }
        else if (strcmp(FiledOfStructName, "Buffer") == 0)
        {
            IsPointer                = TRUE;
            TempOffsetOfFieldFromTop = 0x4;
            TempSizeOfField          = 0x8;
            strcpy(*NewStructOrTypeName, "PWSTR");
        }
        else
        {
            //
            // Unknown Field
            //
            return FALSE;
        }
    }
    else
    {
        //
        // Unknown Structure
        //
        return FALSE;
    }

    //
    // Apply the needed information
    //
    *OffsetOfFieldFromTop = TempOffsetOfFieldFromTop;
    *SizeOfField          = TempSizeOfField;
    *IsItPointerOrNot     = IsPointer;

    return TRUE;
}

/**
 * @brief This function returns the sizeof of the structure
 *
 * @param StructNameOrTypeName Top-level name of struct
 * @param SizeOfField Result of the sizeof
 *
 * @return BOOLEAN if the StructNameOrTypeName not found or there was any error
 * it returns FALSE (in that case script engine should throw an error to the
 * user) otherwise return TRUE which means the script engine can safely use the
 * details
 */
BOOLEAN
SymQuerySizeof(_In_ const char * StructNameOrTypeName, UINT32 * SizeOfField)
{
    if (strcmp(StructNameOrTypeName, "STUPID_STRUCT1") == 0)
    {
        *SizeOfField = sizeof(STUPID_STRUCT1);
    }
    else if (strcmp(StructNameOrTypeName, "STUPID_STRUCT2") == 0)
    {
        *SizeOfField = sizeof(STUPID_STRUCT2);
    }
    else if (strcmp(StructNameOrTypeName, "UNICODE_STRING") == 0)
    {
        *SizeOfField = sizeof(UNICODE_STRING);
    }
    else if (strcmp(StructNameOrTypeName, "PSTUPID_STRUCT1") == 0)
    {
        *SizeOfField = sizeof(PSTUPID_STRUCT1);
    }
    else if (strcmp(StructNameOrTypeName, "PSTUPID_STRUCT2") == 0)
    {
        *SizeOfField = sizeof(PSTUPID_STRUCT2);
    }
    else if (strcmp(StructNameOrTypeName, "PUNICODE_STRING") == 0)
    {
        *SizeOfField = sizeof(PUNICODE_STRING);
    }
    else
    {
        //
        // Unknown Structure
        //
        return FALSE;
    }

    return TRUE;
}

VOID
TestCasting()
{
    //
    // Test
    //
    if (true)
    {
    }
}
