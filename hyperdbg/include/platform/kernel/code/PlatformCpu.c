/**
 * @file PlatformCpu.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of cross platform APIs for CPU and processor queries
 * @details
 * @version 0.19
 * @date 2026-05-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/PlatformCpu.h"
#endif // defined(__linux__)

/**
 * @brief Get the count of active logical processors
 *
 * @return ULONG
 */
ULONG
PlatformCpuGetActiveProcessorCount(VOID)
{
#if defined(_WIN32) || defined(_WIN64)

    return KeQueryActiveProcessorCount(0);

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Get the current logical processor number
 *
 * @return ULONG
 */
ULONG
PlatformCpuGetCurrentProcessorNumber(VOID)
{
#if defined(_WIN32) || defined(_WIN64)

    return KeGetCurrentProcessorNumberEx(NULL);

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}
