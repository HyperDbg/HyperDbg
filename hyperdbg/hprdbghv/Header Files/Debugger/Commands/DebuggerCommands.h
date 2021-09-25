/**
 * @file DebuggerCommands.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Commands for debugger
 * @details This file contains general commands that are implemented for debugger 
 * These command mostly start without "!" or "."
 * 
 * @version 0.1
 * @date 2020-04-10
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

NTSTATUS
DebuggerCommandReadMemory(PDEBUGGER_READ_MEMORY ReadMemRequest, PVOID UserBuffer, PSIZE_T ReturnSize);

BOOLEAN
DebuggerCommandReadMemoryVmxRoot(PDEBUGGER_READ_MEMORY ReadMemRequest, UCHAR * UserBuffer, PSIZE_T ReturnSize);

BOOLEAN
DebuggerCommandEditMemoryVmxRoot(PDEBUGGER_EDIT_MEMORY EditMemRequest);

NTSTATUS
DebuggerReadOrWriteMsr(PDEBUGGER_READ_AND_WRITE_ON_MSR ReadOrWriteMsrRequest, UINT64 * UserBuffer, PSIZE_T ReturnSize);

NTSTATUS
DebuggerCommandEditMemory(PDEBUGGER_EDIT_MEMORY EditMemRequest);

NTSTATUS
DebuggerCommandSearchMemory(PDEBUGGER_SEARCH_MEMORY SearchMemRequest);

NTSTATUS
DebuggerCommandFlush(PDEBUGGER_FLUSH_LOGGING_BUFFERS DebuggerFlushBuffersRequest);

NTSTATUS
DebuggerCommandSignalExecutionState(PDEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL DebuggerFinishedExecutionRequest);

NTSTATUS
DebuggerCommandSendMessage(PDEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER DebuggerSendUsermodeMessageRequest);

NTSTATUS
DebuggerCommandSendGeneralBufferToDebugger(PDEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER DebuggeeBufferRequest);
