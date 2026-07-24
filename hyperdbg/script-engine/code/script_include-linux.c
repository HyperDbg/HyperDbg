/**
 * @file script_include-linux.c
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief Linux stub implementations of the script include-file resolver
 * @details The Windows implementation (script_include.c) uses Win32 APIs
 *          (GetModuleFileNameA / GetFileAttributesA) to resolve `#include`
 *          directives in scripts. Those calls have no drop-in Linux equivalent,
 *          so for now this file provides empty stubs that satisfy the link and
 *          keep every call site intact. Script includes are simply unsupported
 *          on Linux until a real resolver (readlink("/proc/self/exe") + stat) is
 *          implemented.
 *
 *          Signatures mirror script-engine/header/script_include.h exactly.
 *
 *          TODO: implement the real Linux path resolution and drop this file
 *                from the UNIX branch of script-engine/CMakeLists.txt.
 *
 * @version 0.1
 * @date 2026-07-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"
#include "script_include.h"

#ifdef __linux__

VOID
ResolveIncludePath(const char * IncludeFilePath, char * OutPath)
{
    (void)IncludeFilePath;

    if (OutPath != NULL)
    {
        OutPath[0] = '\0';
    }
}

BOOLEAN
FileExists(const char * Path)
{
    (void)Path;

    return FALSE;
}

BOOLEAN
ParseIncludeFile(char * IncludeFile, char ** Buffer)
{
    (void)IncludeFile;

    if (Buffer != NULL)
    {
        *Buffer = NULL;
    }

    return FALSE;
}

char *
InsertStrNew(char * Str, int InputIdx, const char * Buf)
{
    (void)Str;
    (void)InputIdx;
    (void)Buf;

    return NULL;
}

#endif // __linux__
