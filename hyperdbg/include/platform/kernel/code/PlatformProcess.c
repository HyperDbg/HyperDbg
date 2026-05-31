/**
 * @file PlatformProcess.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of cross platform APIs for process and thread queries
 * @details
 * @version 0.19
 * @date 2026-05-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/PlatformProcess.h"
#endif // defined(__linux__)

/**
 * @brief Get the current thread ID
 *
 * @return HANDLE
 */
HANDLE
PlatformProcessGetCurrentThreadId(VOID)
{
#if defined(_WIN32) || defined(_WIN64)

    return PsGetCurrentThreadId();

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Get the current process ID
 *
 * @return HANDLE
 */
HANDLE
PlatformProcessGetCurrentProcessId(VOID)
{
#if defined(_WIN32) || defined(_WIN64)

    return PsGetCurrentProcessId();

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Get the current process (PEPROCESS)
 *
 * @return PVOID Pointer to the EPROCESS structure for the current process
 */
PVOID
PlatformProcessGetCurrentProcess(VOID)
{
#if defined(_WIN32) || defined(_WIN64)

    return (PVOID)PsGetCurrentProcess();

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Get the current thread (PETHREAD)
 *
 * @return PVOID Pointer to the ETHREAD structure for the current thread
 */
PVOID
PlatformProcessGetCurrentThread(VOID)
{
#if defined(_WIN32) || defined(_WIN64)

    return (PVOID)PsGetCurrentThread();

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Get the TEB (Thread Environment Block) of the current thread
 *
 * @return PVOID Pointer to the TEB of the current thread
 */
PVOID
PlatformProcessGetCurrentThreadTeb(VOID)
{
#if defined(_WIN32) || defined(_WIN64)

    return PsGetCurrentThreadTeb();

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}
