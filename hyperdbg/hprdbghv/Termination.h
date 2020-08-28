/**
 * @file Termination.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Headers of Debugger functions for terminating events
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
TerminateExternalInterruptEvent(PDEBUGGER_EVENT Event);

VOID
TerminateHiddenHookReadAndWriteEvent(PDEBUGGER_EVENT Event);

VOID
TerminateHiddenHookReadEvent(PDEBUGGER_EVENT Event);

VOID
TerminateHiddenHookWriteEvent(PDEBUGGER_EVENT Event);

VOID
TerminateHiddenHookExecCcEvent(PDEBUGGER_EVENT Event);

VOID
TerminateHiddenHookExecDetoursEvent(PDEBUGGER_EVENT Event);

VOID
TerminateRdmsrExecutionEvent(PDEBUGGER_EVENT Event);

VOID
TerminateWrmsrExecutionEvent(PDEBUGGER_EVENT Event);

VOID
TerminateExceptionEvent(PDEBUGGER_EVENT Event);

VOID
TerminateInInstructionExecutionEvent(PDEBUGGER_EVENT Event);

VOID
TerminateOutInstructionExecutionEvent(PDEBUGGER_EVENT Event);

VOID
TerminateSyscallHookEferEvent(PDEBUGGER_EVENT Event);

VOID
TerminateSysretHookEferEvent(PDEBUGGER_EVENT Event);

VOID
TerminateVmcallExecutionEvent(PDEBUGGER_EVENT Event);

VOID
TerminateTscEvent(PDEBUGGER_EVENT Event);

VOID
TerminatePmcEvent(PDEBUGGER_EVENT Event);

VOID
TerminateDebugRegistersEvent(PDEBUGGER_EVENT Event);

VOID
TerminateCpuidExecutionEvent(PDEBUGGER_EVENT Event);
