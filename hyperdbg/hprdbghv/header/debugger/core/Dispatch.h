/**
 * @file Dispatch.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers of debugger functions for dispatching, triggering and
 * emulating events
 *
 * @version 0.1
 * @date 2022-09-21
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

VOID
DispatchEventEferSysret(UINT32 CoreIndex, PGUEST_REGS Regs, PVOID Context);

VOID
DispatchEventEferSyscall(UINT32 CoreIndex, PGUEST_REGS Regs, PVOID Context);

VOID
DispatchEventCpuid(PGUEST_REGS Regs);

VOID
DispatchEventTsc(PGUEST_REGS Regs, BOOLEAN IsRdtscp);
