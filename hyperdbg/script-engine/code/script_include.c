/**
 * @file script_include.c
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
 *
 * @brief Include file path resolution and parsing routines
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"
#include "script_include.h"

/**
 * @brief Trims leading whitespace from a string in-place
 *
 * @param Str the string to left-trim
 * @return VOID
 */
static VOID
Ltrim(char * Str)
{
    char * P = Str;

    while (*P && isspace((unsigned char)*P))
        P++;

    if (P != Str)
        memmove(Str, P, strlen(P) + 1);
}

/**
 * @brief Trims trailing whitespace from a string in-place
 *
 * @param Str the string to right-trim
 * @return VOID
 */
static VOID
Rtrim(char * Str)
{
    char * End = Str + strlen(Str);

    while (End > Str && isspace((unsigned char)End[-1]))
        End--;

    *End = '\0';
}

/**
 * @brief Trims leading and trailing whitespace from a string in-place
 *
 * @param Str the string to trim
 * @return VOID
 */
static VOID
Trim(char * Str)
{
    Ltrim(Str);
    Rtrim(Str);
}

/**
 * @brief Inserts a string into another string at a given index
 *
 * @param Str the original string (will be reallocated)
 * @param InputIdx the index at which to insert
 * @param Buf the string to insert
 * @return char * the new string, or the original on allocation failure
 */
char *
InsertStrNew(char * Str, int InputIdx, const char * Buf)
{
    SIZE_T LenStr = strlen(Str);
    SIZE_T LenBuf = strlen(Buf);

    if (InputIdx < 0 || (SIZE_T)InputIdx > LenStr)
        return Str;

    char * NewStr = (char *)realloc(Str, LenStr + LenBuf + 1);
    if (!NewStr)
        return Str;

    memmove(NewStr + InputIdx + LenBuf,
            NewStr + InputIdx,
            LenStr - InputIdx + 1);

    memcpy(NewStr + InputIdx, Buf, LenBuf);

    return NewStr;
}

/**
 * @brief Checks whether a file path is absolute
 *
 * @param Path the file path to check
 * @return BOOLEAN TRUE if the path is absolute, FALSE otherwise
 */
BOOLEAN
IsAbsolutePath(const char * Path)
{
    if (!Path || !Path[0])
        return FALSE;

    if (strlen(Path) >= 3 &&
        ((Path[1] == ':' && (Path[2] == '\\' || Path[2] == '/'))))
        return TRUE;

    return FALSE;
}

/**
 * @brief Resolves an include file path to a full absolute path
 *
 * @param IncludeFilePath the include file path (relative or absolute)
 * @param OutPath the buffer to receive the resolved path
 * @return VOID
 */
VOID
ResolveIncludePath(const char * IncludeFilePath, char * OutPath)
{
    if (IsAbsolutePath(IncludeFilePath))
    {
        strncpy(OutPath, IncludeFilePath, MAX_PATH_LEN - 1);
        OutPath[MAX_PATH_LEN - 1] = '\0';
        return;
    }

    CHAR ExeDir[MAX_PATH_LEN];
    GetModuleFileNameA(NULL, ExeDir, MAX_PATH_LEN);

    char * P = strrchr(ExeDir, '\\');
    if (P)
        *P = '\0';

    snprintf(
        OutPath,
        MAX_PATH_LEN,
        "%s\\%s",
        ExeDir,
        IncludeFilePath);
}

/**
 * @brief Checks whether a file exists at the given path
 *
 * @param Path the file path to check
 * @return BOOLEAN TRUE if the file exists, FALSE otherwise
 */
BOOLEAN
FileExists(const char * Path)
{
    DWORD Attr = GetFileAttributesA(Path);

    if (Attr == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    if (Attr & FILE_ATTRIBUTE_DIRECTORY)
        return FALSE;

    return TRUE;
}

/**
 * @brief Reads and parses an include file into a buffer
 *
 * @param IncludeFile the path to the include file
 * @param Buffer pointer to receive the allocated file content buffer
 * @return BOOLEAN TRUE if the file was parsed successfully, FALSE otherwise
 */
BOOLEAN
ParseIncludeFile(char * IncludeFile, char ** Buffer)
{
    FILE * Fp = fopen(IncludeFile, "r");
    fseek(Fp, 0, SEEK_END);
    LONG Size = ftell(Fp);
    rewind(Fp);
    *Buffer = calloc(Size + 1, 1);
    fread(*Buffer, 1, Size, Fp);
    (*Buffer)[Size] = '\0';
    Trim(*Buffer);
    SIZE_T Len = strlen(*Buffer);
    if ((*Buffer)[0] == '?' &&
        (*Buffer)[1] == ' ' &&
        (*Buffer)[2] == '{' &&
        (*Buffer)[Len - 1] == '}')
    {
        memmove(
            *Buffer,
            *Buffer + 3,
            Len - 3 + 1);

        (*Buffer)[strlen(*Buffer) - 1] = '\0';

        return TRUE;
    }
    return FALSE;
}
