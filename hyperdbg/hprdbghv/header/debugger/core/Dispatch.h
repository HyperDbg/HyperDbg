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

VOID
DispatchEventVmcall(UINT32 CoreIndex, PGUEST_REGS Regs);

VOID
DispatchEventIO(PGUEST_REGS Regs);

VOID
DispatchEventRdmsr(PGUEST_REGS Regs);

VOID
DispatchEventWrmsr(PGUEST_REGS Regs);

VOID
DispatchEventRdpmc(PGUEST_REGS Regs);

VOID
DispatchEventMovToFromControlRegisters(UINT32 CoreIndex, PGUEST_REGS Regs);

VOID
DispatchEventMov2DebugRegs(UINT32 CoreIndex, PGUEST_REGS Regs);

VOID
DispatchEventException(UINT32 CoreIndex, PGUEST_REGS Regs);

VOID
DispatchEventExternalInterrupts(UINT32 CoreIndex, PGUEST_REGS Regs);

VOID
DispatchEventHiddenHookExecCc(PGUEST_REGS Regs, PVOID Context);

VOID
DispatchEventHiddenHookExecDetours(PGUEST_REGS Regs, PVOID Context);

BOOLEAN
DispatchEventHiddenHookPageReadWriteWritePreEvent(PGUEST_REGS Regs, PVOID Context, BOOLEAN * IsTriggeringPostEventAllowed);

BOOLEAN
DispatchEventHiddenHookPageReadWriteReadPreEvent(PGUEST_REGS Regs, PVOID Context, BOOLEAN * IsTriggeringPostEventAllowed);

VOID
DispatchEventHiddenHookPageReadWriteWritePostEvent(PGUEST_REGS Regs, PVOID Context);

VOID
DispatchEventHiddenHookPageReadWriteReadPostEvent(PGUEST_REGS Regs, PVOID Context);
