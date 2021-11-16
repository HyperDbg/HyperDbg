/**
 * @file spinlock.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief The implementation of spinlocks for user-mode
 * @details
 * @version 0.1
 * @date 2021-11-17
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\hprdbgctrl\pch.h"

/**
 * @brief The maximum wait before PAUSE
 * 
 */
static unsigned MaxWait = 65536;

/**
 * @brief Tries to get the lock otherwise returns
 * 
 * @param LONG Lock variable
 * @return BOOLEAN If it was successfull on getting the lock
 */
BOOLEAN
SpinlockTryLock(volatile LONG * Lock)
{
    return (!(*Lock) && !_interlockedbittestandset(Lock, 0));
}

/**
 * @brief Tries to get the lock and won't return until successfully get the lock
 * 
 * @param LONG Lock variable
 */
void
SpinlockLock(volatile LONG * Lock)
{
    unsigned wait = 1;

    while (!SpinlockTryLock(Lock))
    {
        for (unsigned i = 0; i < wait; ++i)
        {
            _mm_pause();
        }

        //
        // Don't call "pause" too many times. If the wait becomes too big,
        // clamp it to the MaxWait.
        //

        if (wait * 2 > MaxWait)
        {
            wait = MaxWait;
        }
        else
        {
            wait = wait * 2;
        }
    }
}

/**
 * @brief Tries to get the lock and won't return until successfully get the lock
 * 
 * @param LONG Lock variable
 * @param LONG MaxWait Maximum wait (pause) count
 */
void
SpinlockLockWithCustomWait(volatile LONG * Lock, unsigned MaximumWait)
{
    unsigned wait = 1;

    while (!SpinlockTryLock(Lock))
    {
        for (unsigned i = 0; i < wait; ++i)
        {
            _mm_pause();
        }

        //
        // Don't call "pause" too many times. If the wait becomes too big,
        // clamp it to the MaxWait.
        //

        if (wait * 2 > MaximumWait)
        {
            wait = MaximumWait;
        }
        else
        {
            wait = wait * 2;
        }
    }
}

/**
 * @brief Release the lock
 * 
 * @param LONG Lock variable
 */

void
SpinlockUnlock(volatile LONG * Lock)
{
    *Lock = 0;
}
