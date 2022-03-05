/**
 * @file Callstack.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Kernel headers for callstacks
 * 
 * @version 0.1
 * @date 2022-03-05
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				     Functions		      		//
//////////////////////////////////////////////////

BOOLEAN
CallstackWalkthroughStack(PDEBUGGER_SINGLE_CALLSTACK_FRAME AddressToSaveFrames,
                          UINT64                           StackBaseAddress,
                          UINT32                           Size,
                          BOOLEAN                          Is32Bit);
