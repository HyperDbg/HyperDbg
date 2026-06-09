/**
 * @file PlatformSpinlock.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of cross platform APIs for kernel spinlock operations
 * @details
 * @version 0.19
 * @date 2026-05-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/PlatformSpinlock.h"
#endif // defined(__linux__)

/**
 * @brief Initialize a kernel spinlock
 *
 * @param SpinLock Pointer to the KSPIN_LOCK to initialize
 * @return VOID
 */
VOID
PlatformSpinlockInitialize(PKSPIN_LOCK SpinLock)
{
#if defined(_WIN32) || defined(_WIN64)

    KeInitializeSpinLock(SpinLock);

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Acquire a kernel spinlock, raising IRQL to DISPATCH_LEVEL
 *
 * @param SpinLock Pointer to the KSPIN_LOCK to acquire
 * @param OldIrql Receives the previous IRQL value to be restored on release
 * @return VOID
 */
VOID
PlatformSpinlockAcquire(PKSPIN_LOCK SpinLock, PKIRQL OldIrql)
{
#if defined(_WIN32) || defined(_WIN64)

    KeAcquireSpinLock(SpinLock, OldIrql);

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}

/**
 * @brief Release a previously acquired kernel spinlock and restore IRQL
 *
 * @param SpinLock Pointer to the KSPIN_LOCK to release
 * @param OldIrql The previous IRQL value saved during acquire
 * @return VOID
 */
VOID
PlatformSpinlockRelease(PKSPIN_LOCK SpinLock, KIRQL OldIrql)
{
#if defined(_WIN32) || defined(_WIN64)

    KeReleaseSpinLock(SpinLock, OldIrql);

#elif defined(__linux__)

#    error "Not yet implemented"

#else

#    error "Unsupported platform"

#endif
}
