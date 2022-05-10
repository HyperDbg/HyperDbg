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
#include "pch.h"

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
 * @param IsStructNamePointerOrNot Shows whether the field specified in
 * FiledOfStructName is a pointer or not
 * @param IsFiledOfStructNamePointerOrNot Shows whether the field specified in
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
                                 _Out_ PBOOLEAN    IsStructNamePointerOrNot,
                                 _Out_ PBOOLEAN    IsFiledOfStructNamePointerOrNot,
                                 _Out_ char **     NewStructOrTypeName,
                                 _Out_ UINT32 * OffsetOfFieldFromTop,
                                 _Out_ UINT32 * SizeOfField)
{
    BOOLEAN IsPointer                 = FALSE;
    BOOLEAN IsTheStructItselfAPointer = FALSE;
    UINT32  TempOffsetOfFieldFromTop  = 0;
    UINT32  TempSizeOfField           = 0;

    if (strcmp(StructName, "STUPID_STRUCT1") == 0 ||
        strcmp(StructName, "PSTUPID_STRUCT1") == 0)
    {
        if (strcmp(StructName, "PSTUPID_STRUCT1") == 0)
        {
            IsTheStructItselfAPointer = TRUE;
        }

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
        if (strcmp(StructName, "PSTUPID_STRUCT2") == 0)
        {
            IsTheStructItselfAPointer = TRUE;
        }

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
        if (strcmp(StructName, "PUNICODE_STRING") == 0)
        {
            IsTheStructItselfAPointer = TRUE;
        }

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
    *OffsetOfFieldFromTop            = TempOffsetOfFieldFromTop;
    *SizeOfField                     = TempSizeOfField;
    *IsFiledOfStructNamePointerOrNot = IsPointer;
    *IsStructNamePointerOrNot        = IsTheStructItselfAPointer;

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
SymQuerySizeof(_In_ const char * StructNameOrTypeName, _Out_ UINT32 * SizeOfField)
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

int
main()
{
    UINT32 SizeofResult = 0;

    //
    // Test sizeof operator
    //

    if (SymQuerySizeof("PUNICODE_STRING", &SizeofResult))
    {
        printf("result of sizeof(PUNICODE_STRING) is : 0x%x\n", SizeofResult);
    }

    if (SymQuerySizeof("SOME_UNKNOWN_STRUCT", &SizeofResult))
    {
        printf("result of sizeof(SOME_UNKNOWN_STRUCT) is : 0x%x\n", SizeofResult);
    }
    else
    {
        printf("SOME_UNKNOWN_STRUCT is not found\n");
    }

    printf("\n\n\n");

    //
    // ********* Test casting *********
    //

    /*


        Local var @ 0x95fa18df58 Type AllocateStructForCasting::__l2::_STUPID_STRUCT2*
        0x00000260`d065ee30 

           +0x000 Sina32           : 0x32
           +0x008 Sina64           : 0x64
           +0x010 AghaaSina        : 0x00000000`00000055 Void
           +0x018 UnicodeStr       : 0x00000260`d065ec70 AllocateStructForCasting::__l2::_UNICODE_STRING
              +0x000 Length           : 0x40
              +0x002 MaximumLength    : 0x40
              +0x008 Buffer           : 0x00000260`d065ecf0  "Goodbye I'm at stupid struct 2!"
           +0x020 StupidStruct1    : 0x00000260`d065eda0 AllocateStructForCasting::__l2::_STUPID_STRUCT1
              +0x000 Flag32           : 0x3232
              +0x008 Flag64           : 0x6464
              +0x010 Context          : 0x00000000`00000085 Void
              +0x018 StringValue      : 0x00000260`d065eb50 AllocateStructForCasting::__l2::_UNICODE_STRING
                 +0x000 Length           : 0x3c
                 +0x002 MaximumLength    : 0x3c
                 +0x008 Buffer           : 0x00000260`d065ebd0  "Hi come from stupid struct 1!"


        my_var =  cast<PSTUPID_STRUCT2>(@rcx)->Sina32;
        // my_var = 0x32

        my_var =  cast<PSTUPID_STRUCT2>(@rcx)->Sina64;
        // my_var = 0x64

        my_var =  cast<PSTUPID_STRUCT2>(@rcx)->AghaaSina;
        // my_var = 0x55

        my_var =  cast<PSTUPID_STRUCT2>(@rcx).Unknownnnnnn;
        // Error because Unknownnnnnn not found

        my_var =  cast<PSTUPID_STRUCT2>(@rcx)->Unknownnnnnn;
        // Error because Unknownnnnnn not found

        my_var =  cast<PSTUPID_STRUCT2>(@rcx).Sina32;
        // Error because PSTUPID_STRUCT2 is pointer, should use '->'

        my_var =  cast<PSTUPID_STRUCT2>(*@rcx).Sina32;
        // Error because PSTUPID_STRUCT2 is pointer, should use '->'

        my_var =  cast<STUPID_STRUCT2>(*@rcx).Sina32;
        // my_var = 0x32

        my_var =  cast<STUPID_STRUCT2>(*@rcx).Sina64;
        // my_var = 0x64

        my_var =  cast<STUPID_STRUCT2>(*@rcx).AghaaSina;
        // my_var = 0x55

        my_var =  cast<STUPID_STRUCT2>(*@rcx).UnicodeStr->MaximumLength;
        // my_var = 0x40

        my_var =  cast<PSTUPID_STRUCT2>(@rcx)->UnicodeStr->MaximumLength;
        // my_var = 0x40

        my_var =  cast<PSTUPID_STRUCT2>(@rcx)->StupidStruct1->Flag32;
        // my_var = 0x3232

        my_var =  cast<PSTUPID_STRUCT2>(@rcx)->StupidStruct1->Flag64;
        // my_var = 0x6464

        my_var =
     cast<STUPID_STRUCT2>(*@rcx).StupidStruct1->StringValue->MaximumLength;
        // my_var = 0x3c

        printf("Result is : %ws\n", cast<PSTUPID_STRUCT2>(@rcx)->UnicodeStr->Buffer );
        // Result is : Goodbye I'm at stupid struct 2!"

        printf("Result is : %ws\n", cast<PSTUPID_STRUCT2>(@rcx)->StupidStruct1->StringValue->Buffer );
        // Result is : Goodbye I'm at stupid struct 2!"

  */

    BOOLEAN IsStructPointerOrNot = FALSE;
    BOOLEAN IsFieldPointerOrNot  = FALSE;
    UINT32  OffsetOfFieldFromTop = NULL;
    UINT32  SizeOfField          = NULL;
    CHAR *  NewStructOrTypeName  = (CHAR *)malloc(100);

    if (SymCastingQueryForFiledsAndTypes(
            "STUPID_STRUCT2",
            "UnicodeStr",
            &IsStructPointerOrNot,
            &IsFieldPointerOrNot,
            &NewStructOrTypeName,
            &OffsetOfFieldFromTop,
            &SizeOfField))
    {
        printf("is the structure itself pointer or not: %s\n",
               IsStructPointerOrNot ? "yes" : "no");
        printf("is the field of structure itself pointer or not: %s\n",
               IsFieldPointerOrNot ? "yes" : "no");
        printf("offset of field from top : %x\n", OffsetOfFieldFromTop);
        printf("size of field : %x\n", SizeOfField);
    }

    free(NewStructOrTypeName);
}
