/**
 * @file Steppings.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Headers of Debugger Steppings Mechanisms
 * @details Used in debugger
 * 
 * @version 0.1
 * @date 2020-08-30
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//					Structures					//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

VOID
SteppingsInitialize();

VOID
SteppingsUninitialize();

VOID
SteppingsHandleMovToCr3Exiting(PGUEST_REGS GuestRegs, UINT32 ProcessorIndex, UINT64 NewCr3);

VOID
SteppingsHandleClockInterruptOnTargetProcess(PGUEST_REGS GuestRegs, UINT32 ProcessorIndex);
