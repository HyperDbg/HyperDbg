/**
 * @file Bitwise.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Functions for bit-level operations
 *
 * @version 0.2
 * @date 2023-04-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Check whether the bit is set or not
 *
 * @param nth
 * @param addr
 * @return int
 */
int
TestBit(int nth, unsigned long * addr)
{
    return (BITMAP_ENTRY(nth, addr) >> BITMAP_SHIFT(nth)) & 1;
}

/**
 * @brief unset the bit
 *
 * @param nth
 * @param addr
 */
void
ClearBit(int nth, unsigned long * addr)
{
    BITMAP_ENTRY(nth, addr) &= ~(1UL << BITMAP_SHIFT(nth));
}

/**
 * @brief set the bit
 *
 * @param nth
 * @param addr
 */
void
SetBit(int nth, unsigned long * addr)
{
    BITMAP_ENTRY(nth, addr) |= (1UL << BITMAP_SHIFT(nth));
}
