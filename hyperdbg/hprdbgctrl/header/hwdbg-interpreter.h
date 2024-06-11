/**
 * @file hwdbg-interpreter.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for the interpreter of hwdbg packets and requests
 * @details
 * @version 1.0
 * @date 2024-06-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				   Definitions                  //
//////////////////////////////////////////////////

#define DEFAULT_INITIAL_DEBUGGEE_TO_DEBUGGER_OFFSET 0x200

//////////////////////////////////////////////////
//				    Functions                   //
//////////////////////////////////////////////////

BOOLEAN
HwdbgInterpretPacket(PVOID BufferReceived, UINT32 LengthReceived);
