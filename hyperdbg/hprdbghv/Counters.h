/**
 * @file Counters.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief The headers for functions for emulating counters 
 * @details
 * @version 0.1
 * @date 2020-06-14
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//			         Functions  				//
//////////////////////////////////////////////////

VOID
CounterEmulateRdtsc(PGUEST_REGS GuestRegs);

VOID
CounterEmulateRdtscp(PGUEST_REGS GuestRegs);

VOID
CounterEmulateRdpmc(PGUEST_REGS GuestRegs);
