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

int
TestBit(int BitNumber, unsigned long * Addr);

void
ClearBit(int BitNumber, unsigned long * Addr);

void
SetBit(int BitNumber, unsigned long * Addr);
