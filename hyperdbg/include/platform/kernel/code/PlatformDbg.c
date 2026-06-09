/**
 * @file PlatformDbg.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of cross platform APIs for kernel debug output
 * @details
 * @version 0.19
 * @date 2026-05-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/PlatformDbg.h"
#endif // defined(__linux__)

/**
 * @brief Print a debug message to the kernel debugger
 *
 * @param Format printf-style format string
 * @param ... Variable arguments
 * @return VOID
 */
VOID
PlatformDbgPrint(const CHAR * Format, ...)
{
#if defined(_WIN32) || defined(_WIN64)

    va_list ArgList;
    va_start(ArgList, Format);
    vDbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, Format, ArgList);
    va_end(ArgList);

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}
