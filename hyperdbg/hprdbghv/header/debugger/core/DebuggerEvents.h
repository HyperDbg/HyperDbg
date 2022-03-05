/**
 * @file DebuggerEvents.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers of Debugger events (triggers and enable events)
 * 
 * @version 0.1
 * @date 2020-05-12
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				     Functions		      		//
//////////////////////////////////////////////////

VOID
DebuggerEventEnableEferOnAllProcessors();

VOID
DebuggerEventDisableEferOnAllProcessors();

VOID
DebuggerEventEnableMovToCr3ExitingOnAllProcessors();

VOID
DebuggerEventDisableMovToCr3ExitingOnAllProcessors();

PVOID
DebuggerEventEptHook2GeneralDetourEventHandler(PGUEST_REGS Regs, PVOID CalledFrom);

BOOLEAN
DebuggerEventEnableMonitorReadAndWriteForAddress(UINT64  Address,
                                                 UINT32  ProcessId,
                                                 BOOLEAN EnableForRead,
                                                 BOOLEAN EnableForWrite);
