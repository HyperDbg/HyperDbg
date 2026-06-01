/**
 * @file platform-lib-calls.c
 * @author Max Raulea (max.raulea@gmail.com)
 * @brief User mode Cross platform APIs for platofrm dependend library calls
 * @details
 * @version 0.19
 * @date 2026-06-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "platform/user/header/platform-lib-calls.h"

/**
 * @brief Platform independent wrapper for vsprintf_s / vsnprintf
 *
 * @param Buffer output buffer
 * @param BufferSize size of the output buffer
 * @param Format format string
 * @param ArgList variadic argument list
 * @return INT number of characters written, or -1 on error
 */
INT
PlatformVsnprintf(char * Buffer, SIZE_T BufferSize, const char * Format, va_list ArgList)
{
#if defined(_WIN32)
    return vsprintf_s(Buffer, BufferSize, Format, ArgList);
#elif defined(__linux__)
    return vsnprintf(Buffer, BufferSize, Format, ArgList);
#else
#     error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for _strdup / strdup
 *
 * @param Str string to duplicate
 * @return char * pointer to the duplicated string, or NULL on failure
 */
char *
PlatformStrDup(const char * Str)
{
#if defined(_WIN32)
    return _strdup(Str);
#elif defined(__linux__)
    return strdup(Str);
#else
#     error "Unsupported platform"
#endif
}

/**
 * @brief Platform independent wrapper for RtlZeroMemory / memset
 *
 * @param Buffer pointer to the memory region to zero
 * @param Size number of bytes to zero
 */
VOID
PlatformZeroMemory(PVOID Buffer, SIZE_T Size)
{
#if defined(_WIN32)
    RtlZeroMemory(Buffer, Size);
#elif defined(__linux__)
    memset(Buffer, 0, Size);
#else
#     error "Unsupported platform"
#endif
}
