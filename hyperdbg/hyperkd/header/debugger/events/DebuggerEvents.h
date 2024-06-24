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

BOOLEAN
DebuggerEventEnableMonitorReadWriteExec(EPT_HOOKS_ADDRESS_DETAILS_FOR_MEMORY_MONITOR * HookingDetails,
                                        UINT32                                         ProcessId,
                                        BOOLEAN                                        ApplyDirectlyFromVmxRoot);

BOOLEAN
DebuggerCheckProcessOrThreadChange(_In_ UINT32 CoreId);
