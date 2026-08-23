/**
 * @file PlatformStr.c
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief Implementation of cross platform APIs for kernel string formatting
 * @details
 * @version 0.19
 * @date 2026-08-14
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include <linux/string.h> // strnlen (vsnprintf comes from <linux/kernel.h>)

#    include "../header/PlatformStr.h"
#endif // defined(__linux__)

/**
 * @brief Platform independent wrapper for vsprintf_s / vsnprintf
 *
 * @param Buffer output buffer
 * @param BufferSize size of the output buffer
 * @param Format format string
 * @param ArgList variadic argument list
 * @return INT32 number of characters written, or -1 if it did not fit
 */
INT32
PlatformVsnprintf(CHAR * Buffer, SIZE_T BufferSize, const CHAR * Format, va_list ArgList)
{
#if defined(_WIN32) || defined(_WIN64)

    return vsprintf_s(Buffer, BufferSize, Format, ArgList);

#elif defined(__linux__)

    INT32 Result = vsnprintf(Buffer, BufferSize, Format, ArgList);

    //
    // vsnprintf returns the length the output WOULD have had; vsprintf_s
    // returns -1 when it does not fit, and callers test for that
    //
    if (Result >= 0 && (SIZE_T)Result >= BufferSize)
    {
        return -1;
    }

    return Result;

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Platform independent wrapper for sprintf_s / snprintf
 *
 * @param Buffer output buffer
 * @param BufferSize size of the output buffer
 * @param Format format string
 * @param ... Variable arguments
 * @return INT32 number of characters written, or -1 if it did not fit
 */
INT32
PlatformSprintf(CHAR * Buffer, SIZE_T BufferSize, const CHAR * Format, ...)
{
    INT32   Result;
    va_list ArgList;

    va_start(ArgList, Format);
    Result = PlatformVsnprintf(Buffer, BufferSize, Format, ArgList);
    va_end(ArgList);

    return Result;
}

/**
 * @brief Platform independent wrapper for strnlen_s / strnlen
 *
 * @param Str string to measure (must not be NULL)
 * @param MaxLength maximum number of characters to examine
 * @return SIZE_T length of the string, capped at MaxLength
 */
SIZE_T
PlatformStrnlen(const CHAR * Str, SIZE_T MaxLength)
{
#if defined(_WIN32) || defined(_WIN64)

    return strnlen_s(Str, MaxLength);

#elif defined(__linux__)

    return strnlen(Str, MaxLength);

#else

#    error "Unsupported platform"

#endif
}
