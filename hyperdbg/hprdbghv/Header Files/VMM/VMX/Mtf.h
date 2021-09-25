/**
 * @file Mtf.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Monitor Trap Flag Headers 
 * @details
 * @version 0.1
 * @date 2021-01-27
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//			         Functions  				//
//////////////////////////////////////////////////
VOID
MtfHandleVmexit(ULONG CurrentProcessorIndex, PGUEST_REGS GuestRegs);
