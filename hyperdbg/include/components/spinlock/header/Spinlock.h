/**
 * @file Spinlock.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers of spinlock routines
 * @details
 * @version 0.1
 * @date 2020-04-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				 Spinlock Functions				//
//////////////////////////////////////////////////

BOOLEAN
SpinlockTryLock(volatile LONG * Lock);

BOOLEAN
SpinlockCheckLock(volatile LONG * Lock);

void
SpinlockLock(volatile LONG * Lock);

void
SpinlockLockWithCustomWait(volatile LONG * Lock, unsigned MaxWait);

void
SpinlockUnlock(volatile LONG * Lock);

void
SpinlockInterlockedCompareExchange(
    LONG volatile * Destination,
    LONG            Exchange,
    LONG            Comperand);

#define ScopedSpinlock(LockObject, CodeToRun)   \
    MetaScopedExpr(SpinlockLock(&LockObject),   \
                   SpinlockUnlock(&LockObject), \
                   CodeToRun)
