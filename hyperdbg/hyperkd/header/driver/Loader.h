/**
 * @file Loader.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Routines for perform initial VMM and debugger loads
 * @details
 *
 * @version 0.2
 * @date 2023-01-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//		    	 Loader Functions				//
//////////////////////////////////////////////////

BOOLEAN
LoaderInitHyperLog();

BOOLEAN
LoaderInitHyperTrace(PDEBUGGER_INIT_HYPERTRACE_PACKET InitHyperTracePacket, BOOLEAN RunningOnHypervisorEnvironment);

BOOLEAN
LoaderInitDebuggerAndVmm(PDEBUGGER_INIT_VMM_PACKET InitVmmPacket);

VOID
LoaderUninitVmmAndDebugger();

VOID
LoaderUninitLogTracer();
