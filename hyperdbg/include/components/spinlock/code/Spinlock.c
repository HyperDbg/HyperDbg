/**
 * @file Spinlock.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief This is the implementation for custom spinlock.
 *
 * @details This implementation is derived from Hvpp by Petr Benes
 *      - https://github.com/wbenny/hvpp
 * Based on my benchmarks, this simple implementation beats other (often
 * more complex) spinlock implementations - such as queue spinlocks, ticket
 * spinlocks, MCS locks.  The only difference between this implementation
 * and completely naive spinlock is the "backoff".
 *
 * Also, benefit of this implementation is that we can use it with
 * STL lock guards, e.g.: std::lock_guard.
 *
 * Look here for more information:
 *      - https://locklessinc.com/articles/locks/
 *      - https://github.com/cyfdecyf/spinlock
 *
 * @version 0.1
 * @date 2020-04-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief The maximum Wait before PAUSE
 *
 */
static UINT32 g_MaxWait = 65536;

/**
 * @brief Tries to get the lock otherwise returns
 *
 * @param Lock Lock variable
 * @return BOOLEAN If it was successful on getting the lock
 */
BOOLEAN
SpinlockTryLock(volatile LONG * Lock)
{
    return (!(*Lock) && !_interlockedbittestandset(Lock, 0));
}

/**
 * @brief Tries to get the lock and won't return until successfully get the lock
 *
 * @param Lock Lock variable
 * @return VOID
 */
VOID
SpinlockLock(volatile LONG * Lock)
{
    UINT32 Wait = 1;

    while (!SpinlockTryLock(Lock))
    {
        for (UINT32 i = 0; i < Wait; ++i)
        {
            _mm_pause();
        }

        //
        // Don't call "pause" too many times. If the Wait becomes too big,
        // clamp it to the MaxWait.
        //

        if (Wait * 2 > g_MaxWait)
        {
            Wait = g_MaxWait;
        }
        else
        {
            Wait = Wait * 2;
        }
    }
}

/**
 * @brief Interlocked spinlock that tries to change the value
 * and makes sure that it changed the target value
 *
 * @param Destination A pointer to the destination value
 * @param Exchange The exchange value
 * @param Comperand The value to compare to Destination
 * @return VOID
 */
VOID
SpinlockInterlockedCompareExchange(
    LONG volatile * Destination,
    LONG            Exchange,
    LONG            Comperand)
{
    UINT32 Wait = 1;

    while (InterlockedCompareExchange(Destination, Exchange, Comperand) != Comperand)
    {
        for (UINT32 i = 0; i < Wait; ++i)
        {
            _mm_pause();
        }

        //
        // Don't call "pause" too many times. If the Wait becomes too big,
        // clamp it to the MaxWait.
        //

        if (Wait * 2 > g_MaxWait)
        {
            Wait = g_MaxWait;
        }
        else
        {
            Wait = Wait * 2;
        }
    }
}

/**
 * @brief Tries to get the lock and won't return until successfully get the lock
 *
 * @param Lock Lock variable
 * @param MaximumWait Maximum wait (pause) count
 * @return VOID
 */
VOID
SpinlockLockWithCustomWait(volatile LONG * Lock, UINT32 MaximumWait)
{
    UINT32 Wait = 1;

    while (!SpinlockTryLock(Lock))
    {
        for (UINT32 i = 0; i < Wait; ++i)
        {
            _mm_pause();
        }

        //
        // Don't call "pause" too many times. If the Wait becomes too big,
        // clamp it to the MaxWait.
        //

        if (Wait * 2 > MaximumWait)
        {
            Wait = MaximumWait;
        }
        else
        {
            Wait = Wait * 2;
        }
    }
}

/**
 * @brief Release the lock
 *
 * @param Lock Lock variable
 * @return VOID
 */
VOID
SpinlockUnlock(volatile LONG * Lock)
{
    *Lock = 0;
}

/**
 * @brief Check the lock without changing the state
 *
 * @param Lock Lock variable
 * @return BOOLEAN Whether the lock is acquired or not
 */
BOOLEAN
SpinlockCheckLock(volatile LONG * Lock)
{
    if (*Lock)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
