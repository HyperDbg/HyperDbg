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
 * @param BitNumber
 * @param addr
 * @return int
 */
int
TestBit(int BitNumber, unsigned long * addr)
{
    return (BITMAP_ENTRY(BitNumber, addr) >> BITMAP_SHIFT(BitNumber)) & 1;
}

/**
 * @brief unset the bit
 *
 * @param BitNumber
 * @param addr
 */
void
ClearBit(int BitNumber, unsigned long * addr)
{
    BITMAP_ENTRY(BitNumber, addr) &= ~(1UL << BITMAP_SHIFT(BitNumber));
}

/**
 * @brief set the bit
 *
 * @param BitNumber
 * @param addr
 */
void
SetBit(int BitNumber, unsigned long * addr)
{
    BITMAP_ENTRY(BitNumber, addr) |= (1UL << BITMAP_SHIFT(BitNumber));
}
