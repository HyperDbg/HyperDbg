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
 * @param Addr
 * @return INT
 */
INT
TestBit(INT BitNumber, ULONG * Addr)
{
    return (BITMAP_ENTRY(BitNumber, Addr) >> BITMAP_SHIFT(BitNumber)) & 1;
}

/**
 * @brief unset the bit
 *
 * @param BitNumber
 * @param Addr
 */
VOID
ClearBit(INT BitNumber, ULONG * Addr)
{
    BITMAP_ENTRY(BitNumber, Addr) &= ~(1UL << BITMAP_SHIFT(BitNumber));
}

/**
 * @brief set the bit
 *
 * @param BitNumber
 * @param Addr
 */
VOID
SetBit(INT BitNumber, ULONG * Addr)
{
    BITMAP_ENTRY(BitNumber, Addr) |= (1UL << BITMAP_SHIFT(BitNumber));
}
