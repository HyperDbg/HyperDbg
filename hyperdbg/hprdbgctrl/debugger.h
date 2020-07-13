/**
 * @file debugger.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief General debugger functions
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				    Functions                   //
//////////////////////////////////////////////////

BOOLEAN
InterpretConditionsAndCodes(vector<string> *SplittedCommand,
                            BOOLEAN IsConditionBuffer, PUINT64 BufferAddrss,
                            PUINT32 BufferLength);

BOOLEAN
SendEventToKernel(PDEBUGGER_GENERAL_EVENT_DETAIL Event,
                  UINT32 EventBufferLength);

BOOLEAN
RegisterActionToEvent(PDEBUGGER_GENERAL_ACTION Action,
                      UINT32 ActionsBufferLength);

BOOLEAN InterpretGeneralEventAndActionsFields(
    vector<string> *SplittedCommand, DEBUGGER_EVENT_TYPE_ENUM EventType,
    PDEBUGGER_GENERAL_EVENT_DETAIL *EventDetailsToFill,
    PUINT32 EventBufferLength, PDEBUGGER_GENERAL_ACTION *ActionDetailsToFill,
    PUINT32 ActionBufferLength);

UINT64 GetNewDebuggerEventTag();
