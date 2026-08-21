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
    va_list ArgList;
    va_start(ArgList, Format);
    vprintk(Format, ArgList);
    va_end(ArgList);
#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Break into the kernel debugger.
 * @details Windows fires int 3 into the attached kernel debugger. On Linux no
 *          debugger is normally attached, so a real breakpoint would panic the
 *          machine — and this sits in the LogError() path (fires only when
 *          DebugMode is set) — so the Linux arm is a no-op.
 *
 * TODO(Linux): route to a real breakpoint once a kgdb-style transport exists.
 */
VOID
PlatformDbgBreakPoint(VOID)
{
#if defined(_WIN32) || defined(_WIN64)

    DbgBreakPoint();

#elif defined(__linux__)

    // no-op

#else

#    error "Unsupported platform"

#endif
}
