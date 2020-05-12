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
//				Memory Manager		    		//
//////////////////////////////////////////////////

NTSTATUS
MemoryManagerReadProcessMemoryNormal(HANDLE PID, PVOID Address, DEBUGGER_READ_MEMORY_TYPE MemType, PVOID UserBuffer, SIZE_T Size, PSIZE_T ReturnSize);

//////////////////////////////////////////////////
//					Structures					//
//////////////////////////////////////////////////

/**
 * @brief Use to modify Msrs or read MSR values
 * 
 */
typedef struct _PROCESSOR_DEBUGGING_MSR_READ_OR_WRITE
{
    UINT64 Msr;   // Msr (ecx)
    UINT64 Value; // the value to write on msr

} PROCESSOR_DEBUGGING_MSR_READ_OR_WRITE, PPROCESSOR_DEBUGGING_MSR_READ_OR_WRITE;

/**
 * @brief Saves the debugger state
 * Each logical processor contains one of this structure which describes about the
 * state of debuggers, flags, etc.
 * 
 */
typedef struct _PROCESSOR_DEBUGGING_STATE
{
    PROCESSOR_DEBUGGING_MSR_READ_OR_WRITE MsrState;

} PROCESSOR_DEBUGGING_STATE, PPROCESSOR_DEBUGGING_STATE;

//////////////////////////////////////////////////
//					Data Type					//
//////////////////////////////////////////////////

typedef UINT64 DebuggerCheckForCondition(VOID);

typedef PVOID DebuggerRunCustomCodeFunc(VOID);

typedef PVOID
DebuggerRunCustomCodeWithPreAllocatedBufferFunc(PVOID PreAllocatedBufferAddress);

//////////////////////////////////////////////////
//					Log wit Tag					//
//////////////////////////////////////////////////


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
DebuggerPerformLogTheStates(UINT64 Tag, PDEBUGGER_EVENT_ACTION Action, PGUEST_REGS Regs, PVOID Context);

VOID
DebuggerPerformRunTheCustomCode(UINT64 Tag, PDEBUGGER_EVENT_ACTION Action, PGUEST_REGS Regs, PVOID Context);
