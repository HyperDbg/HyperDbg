/**
 * @file spinlock.cpp
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
 * @date 2022-05-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief The maximum wait before PAUSE
 *
 */
static UINT32 MaxWait = 65536;

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
 */
VOID
SpinlockLock(volatile LONG * Lock)
{
    UINT32 Wait = 1;

    while (!SpinlockTryLock(Lock))
    {
        for (UINT32 i = 0; i < Wait; ++i)
        {
            CpuPause();
        }

        //
        // Don't call "pause" too many times. If the wait becomes too big,
        // clamp it to the MaxWait.
        //

        if (Wait * 2 > MaxWait)
        {
            Wait = MaxWait;
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
 */
VOID
SpinlockLockWithCustomWait(volatile LONG * Lock, UINT32 MaximumWait)
{
    UINT32 Wait = 1;

    while (!SpinlockTryLock(Lock))
    {
        for (UINT32 i = 0; i < Wait; ++i)
        {
            CpuPause();
        }

        //
        // Don't call "pause" too many times. If the wait becomes too big,
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
 */
VOID
SpinlockUnlock(volatile LONG * Lock)
{
    *Lock = 0;
}
