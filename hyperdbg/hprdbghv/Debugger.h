/**
 * @file Debugger.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief General debugger headers
 * @details 
 * @version 0.1
 * @date 2020-04-14
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

#pragma once
#include <ntddk.h>
#include "Common.h"
#include "Logging.h"

//////////////////////////////////////////////////
//					Structures					//
//////////////////////////////////////////////////

/**
 * @brief Saves the debugger state
 * Each logical processor contains one of this structure which describes about the
 * state of debuggers, flags, etc.
 * 
 */
typedef struct _PROCESSOR_DEBUGGING_STATE
{
    UINT64 UndefinedInstructionAddress; // #UD Location of instruction (used by EFER Syscall)

} PROCESSOR_DEBUGGING_STATE, PPROCESSOR_DEBUGGING_STATE;

//////////////////////////////////////////////////
//					Log wit Tag					//
//////////////////////////////////////////////////

/* Send buffer to the usermode with a tag that shows what was the action */
#define LogWithTag(tag, IsImmediate, format, ...) \
    LogSendMessageToQueue(OPERATION_LOG_WITH_TAG, IsImmediate, FALSE, "%016x" format, tag, __VA_ARGS__);

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

BOOLEAN
DebuggerInitialize();

PDEBUGGER_EVENT
DebuggerCreateEvent(BOOLEAN Enabled, UINT32 CoreId, DEBUGGER_EVENT_TYPE_ENUM EventType, UINT64 Tag, UINT32 ConditionsBufferSize, PVOID ConditionBuffer);

BOOLEAN
DebuggerAddActionToEvent(PDEBUGGER_EVENT Event, DEBUGGER_EVENT_ACTION_TYPE_ENUM ActionType, BOOLEAN SendTheResultsImmediately, PDEBUGGER_EVENT_REQUEST_CUSTOM_CODE InTheCaseOfCustomCode, PDEBUGGER_EVENT_ACTION_LOG_CONFIGURATION InTheCaseOfLogTheStates);

BOOLEAN
DebuggerRegisterEvent(PDEBUGGER_EVENT Event);

BOOLEAN
DebuggerTriggerEvents(DEBUGGER_EVENT_TYPE_ENUM EventType, PGUEST_REGS Regs, PVOID Context);

VOID
DebuggerPerformActions(PDEBUGGER_EVENT Event, PGUEST_REGS Regs, PVOID Context);

VOID
DebuggerPerformBreakToDebugger(UINT64 Tag, PDEBUGGER_EVENT_ACTION Action, PGUEST_REGS Regs, PVOID Context);

VOID
DebuggerPerformLogTheStates(UINT64 Tag, PDEBUGGER_EVENT_ACTION Action,PGUEST_REGS Regs, PVOID Context);

VOID
DebuggerPerformRunTheCustomCode(UINT64 Tag, PDEBUGGER_EVENT_ACTION Action, PGUEST_REGS Regs, PVOID Context);
