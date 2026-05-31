/**
 * @file packets.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief headers for kernel packet functions
 * @details
 * @version 0.19
 * @date 2026-05-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				    Functions                   //
//////////////////////////////////////////////////

VOID
ReadIrpBasedBuffer();

DWORD WINAPI
IrpBasedBufferThread(void * data);
