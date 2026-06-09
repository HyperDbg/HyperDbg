/**
 * @file Bitwise.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header files for bit-level operations
 * @details
 * @version 0.2
 * @date 2023-04-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

INT
TestBit(INT BitNumber, ULONG * Addr);

VOID
ClearBit(INT BitNumber, ULONG * Addr);

VOID
SetBit(INT BitNumber, ULONG * Addr);
