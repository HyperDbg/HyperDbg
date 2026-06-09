/**
 * @file PlatformSpinlock.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Cross platform APIs for kernel spinlock operations
 * @details
 * @version 0.19
 * @date 2026-05-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#if defined(__linux__)
#    include "../../../../include/SDK/HyperDbgSdk.h"
#endif // defined(__linux__)

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

#if defined(_WIN32) || defined(_WIN64)

VOID
PlatformSpinlockInitialize(PKSPIN_LOCK SpinLock);

VOID
PlatformSpinlockAcquire(PKSPIN_LOCK SpinLock, PKIRQL OldIrql);

VOID
PlatformSpinlockRelease(PKSPIN_LOCK SpinLock, KIRQL OldIrql);

#endif // defined(_WIN32) || defined(_WIN64)
