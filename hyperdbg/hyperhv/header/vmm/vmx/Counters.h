/**
 * @file Counters.h
 * @author Sina Karvandi (sina@hyperdbg.org)
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
CounterEmulateRdtsc(VIRTUAL_MACHINE_STATE * VCpu);

VOID
CounterEmulateRdtscp(VIRTUAL_MACHINE_STATE * VCpu);

VOID
CounterEmulateRdpmc(VIRTUAL_MACHINE_STATE * VCpu);

VOID
CounterSetPreemptionTimer(UINT32 TimerValue);

VOID
CounterClearPreemptionTimer();
