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

    spin_lock_init(SpinLock);

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

    //
    // KeAcquireSpinLock raises to DISPATCH_LEVEL (preempt off, HW interrupts on)
    // then acquires — exactly spin_lock(). The OldIrql token is not needed on
    // Linux (preempt is a nesting counter, restored by spin_unlock); report
    // PASSIVE_LEVEL (0). (spin_lock_irqsave would additionally disable IRQs — a
    // higher level than Windows takes here, and its `flags` would not fit in the
    // 1-byte KIRQL — so plain spin_lock matches, consistent with PlatformIrql.)
    //
    spin_lock(SpinLock);

    *OldIrql = 0;

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

    //
    // Pairs with the spin_lock() above; the saved token is unused (see acquire).
    //
    UNREFERENCED_PARAMETER(OldIrql);

    spin_unlock(SpinLock);

#else

#    error "Unsupported platform"

#endif
}
