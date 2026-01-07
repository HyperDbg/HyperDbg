#include "pch.h"
#include "script_include.h"

static void
Ltrim(char * s)
{
    char * p = s;

    while (*p && isspace((unsigned char)*p))
        p++;

    if (p != s)
        memmove(s, p, strlen(p) + 1);
}

static void
Rtrim(char * s)
{
    char * end = s + strlen(s);

    while (end > s && isspace((unsigned char)end[-1]))
        end--;

    *end = '\0';
}

static void
Trim(char * s)
{
    Ltrim(s);
    Rtrim(s);
}

char *
InsertStrNew(char * str, int InputIdx, const char * buf)
{
    size_t len_str = strlen(str);
    size_t len_buf = strlen(buf);

    if (InputIdx < 0 || InputIdx > len_str)
        return str;

    char * newstr = realloc(str, len_str + len_buf + 1);
    if (!newstr)
        return str;

    memmove(newstr + InputIdx + len_buf,
            newstr + InputIdx,
            len_str - InputIdx + 1);

    memcpy(newstr + InputIdx, buf, len_buf);

    return newstr;
}

BOOLEAN
IsAbsolutePath(const char * path)
{
    if (!path || !path[0])
        return FALSE;

    if (strlen(path) >= 3 &&
        ((path[1] == ':' && (path[2] == '\\' || path[2] == '/'))))
        return TRUE;

    return FALSE;
}

void
ResolveIncludePath(const char * IncludeFilePath, char * OutPath)
{
    if (IsAbsolutePath(IncludeFilePath))
    {
        strncpy(OutPath, IncludeFilePath, MAX_PATH_LEN - 1);
        OutPath[MAX_PATH_LEN - 1] = '\0';
        return;
    }

    char ExeDir[MAX_PATH_LEN];
    GetModuleFileNameA(NULL, ExeDir, MAX_PATH_LEN);

    char * p = strrchr(ExeDir, '\\');
    if (p)
        *p = '\0';

    snprintf(
        OutPath,
        MAX_PATH_LEN,
        "%s\\%s",
        ExeDir,
        IncludeFilePath);
}

BOOLEAN
FileExists(const char * path)
{
    DWORD attr = GetFileAttributesA(path);

    if (attr == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    if (attr & FILE_ATTRIBUTE_DIRECTORY)
        return FALSE;

    return TRUE;
}

BOOLEAN
ParseIncludeFile(char * IncludeFile, char ** Buffer)
{
    FILE * fp = fopen(IncludeFile, "r");
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    *Buffer = calloc(size + 1, 1);
    fread(*Buffer, 1, size, fp);
    (*Buffer)[size] = '\0';
    Trim(*Buffer);
    size_t len = strlen(*Buffer);
    if ((*Buffer)[0] == '?' &&
        (*Buffer)[1] == ' ' &&
        (*Buffer)[2] == '{' &&
        (*Buffer)[len - 1] == '}')
    {
        memmove(
            *Buffer,
            *Buffer + 3,
            len - 3 + 1);

        (*Buffer)[strlen(*Buffer) - 1] = '\0';

        return TRUE;
    }
    return FALSE;
}
