/**
 * @file Termination.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers of debugger functions for terminating events
 * @details
 *
 * @version 0.1
 * @date 2020-08-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

VOID
TerminateExternalInterruptEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminateHiddenHookReadAndWriteAndExecuteEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminateHiddenHookExecCcEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminateHiddenHookExecDetoursEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminateRdmsrExecutionEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminateWrmsrExecutionEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminateExceptionEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminateInInstructionExecutionEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminateOutInstructionExecutionEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminateSyscallHookEferEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminateSysretHookEferEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminateVmcallExecutionEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminateExecTrapModeChangedEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminateTscEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminatePmcEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminateDebugRegistersEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminateCpuidExecutionEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

VOID
TerminateControlRegistersEvent(PDEBUGGER_EVENT Event, BOOLEAN InputFromVmxRoot);

BOOLEAN
TerminateEptHookUnHookSingleAddressFromVmxRootAndApplyInvalidation(UINT64 VirtualAddress,
                                                                   UINT64 PhysAddress);

BOOLEAN
TerminateQueryDebuggerResource(UINT32                               CoreId,
                               PROTECTED_HV_RESOURCES_TYPE          ResourceType,
                               PVOID                                Context,
                               PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver);
