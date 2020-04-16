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
#include "Logging.h"

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

// Each core has one of the structure in g_GuestState
typedef struct _DEBUGGER_CORE_EVENTS
{
    LIST_ENTRY HiddenHookRwEventsHead;          // HIDDEN_HOOK_RW  [WARNING : MAKE SURE TO INITIALIZE LIST HEAD]
    LIST_ENTRY HiddenHooksExecDetourEventsHead; // HIDDEN_HOOK_EXEC_DETOUR [WARNING : MAKE SURE TO INITIALIZE LIST HEAD]
    LIST_ENTRY HiddenHookExecCcEventsHead;      // HIDDEN_HOOK_EXEC_CC [WARNING : MAKE SURE TO INITIALIZE LIST HEAD]
    LIST_ENTRY SyscallHooksEferEventsHead;      // SYSCALL_HOOK_EFER [WARNING : MAKE SURE TO INITIALIZE LIST HEAD]

} DEBUGGER_CORE_EVENTS, *PDEBUGGER_CORE_EVENTS;

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

VOID
DebuggerPerformActions(PDEBUGGER_EVENT Event, PVOID Context)
