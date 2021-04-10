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
//          Script Engine Wrapper               //
//////////////////////////////////////////////////

VOID
ScriptEngineWrapperTestParser(string Expr);

PVOID
ScriptEngineParseWrapper(char * str);

void
PrintSymbolBufferWrapper(PVOID SymbolBuffer);

UINT64
ScriptEngineWrapperGetHead(PVOID SymbolBuffer);

UINT32
ScriptEngineWrapperGetSize(PVOID SymbolBuffer);

UINT32
ScriptEngineWrapperGetPointer(PVOID SymbolBuffer);

VOID
ScriptEngineWrapperRemoveSymbolBuffer(PVOID SymbolBuffer);

DWORD WINAPI
ListeningSerialPauseDebuggeeThread(PVOID Param);

DWORD WINAPI
ListeningSerialPauseDebuggerThread(PVOID Param);

//////////////////////////////////////////////////
//            	    Structures                  //
//////////////////////////////////////////////////

/**
 * @brief structures related to debugging
 *
 */
typedef struct _DEBUGGING_STATE
{
    BOOLEAN IsAttachedToUsermodeProcess;
    UINT64  ConnectedProcessId;
    UINT64  ConnectedThreadId;
} DEBUGGING_STATE, *PDEBUGGING_STATE;

/**
 * @brief In debugger holds the state of events
 *
 */
typedef struct _DEBUGGER_SYNCRONIZATION_EVENTS_STATE
{
    HANDLE  EventHandle;
    BOOLEAN IsOnWaitingState;
} DEBUGGER_SYNCRONIZATION_EVENTS_STATE, *PDEBUGGER_SYNCRONIZATION_EVENTS_STATE;

//////////////////////////////////////////////////
//				    Functions                   //
//////////////////////////////////////////////////

BOOLEAN
ShowErrorMessage(UINT32 Error);

BOOLEAN
IsConnectedToAnyInstanceOfDebuggerOrDebuggee();

BOOLEAN
IsTagExist(UINT64 Tag);

BOOLEAN
DebuggerPauseDebuggee();

BOOLEAN
InterpretConditionsAndCodes(vector<string> * SplittedCommand,
                            BOOLEAN          IsConditionBuffer,
                            PUINT64          BufferAddress,
                            PUINT32          BufferLength);

BOOLEAN
SendEventToKernel(PDEBUGGER_GENERAL_EVENT_DETAIL Event,
                  UINT32                         EventBufferLength);

BOOLEAN
RegisterActionToEvent(PDEBUGGER_GENERAL_ACTION ActionBreakToDebugger,
                      UINT32                   ActionBreakToDebuggerLength,
                      PDEBUGGER_GENERAL_ACTION ActionCustomCode,
                      UINT32                   ActionCustomCodeLength,
                      PDEBUGGER_GENERAL_ACTION ActionScript,
                      UINT32                   ActionScriptLength);

BOOLEAN
InterpretGeneralEventAndActionsFields(
    vector<string> *                 SplittedCommand,
    DEBUGGER_EVENT_TYPE_ENUM         EventType,
    PDEBUGGER_GENERAL_EVENT_DETAIL * EventDetailsToFill,
    PUINT32                          EventBufferLength,
    PDEBUGGER_GENERAL_ACTION *       ActionDetailsToFillBreakToDebugger,
    PUINT32                          ActionBufferLengthBreakToDebugger,
    PDEBUGGER_GENERAL_ACTION *       ActionDetailsToFillCustomCode,
    PUINT32                          ActionBufferLengthCustomCode,
    PDEBUGGER_GENERAL_ACTION *       ActionDetailsToFillScript,
    PUINT32                          ActionBufferLengthScript);

UINT64
GetNewDebuggerEventTag();

VOID
LogopenSaveToFile(const char * Text);

BOOL
BreakController(DWORD CtrlType);

VOID
CommandEventsShowEvents();

BOOLEAN
CommandEventsModifyAndQueryEvents(UINT64                      Tag,
                                  DEBUGGER_MODIFY_EVENTS_TYPE TypeOfAction);

VOID
CommandEventsHandleModifiedEvent(
    UINT64                  Tag,
    PDEBUGGER_MODIFY_EVENTS ModifyEventRequest);

VOID
CommandFlushRequestFlush();

UINT64
GetCommandAttributes(string FirstCommand);

VOID
AttachToProcess(UINT32 TargetPid, UINT32 TargetTid);

VOID
DetachFromProcess();

BOOLEAN
CommandLoadVmmModule();

VOID
CommandFormatsShowResults(UINT64 U64Value);

VOID
ShowAllRegisters();
