/**
 * @file debugger.h
 * @author Sina Karvandi (sina@hyperdbg.org)
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
//		Debugger Synchronization Objects        //
//////////////////////////////////////////////////

/**
 * @brief maximum number of event handles in kernel-debugger
 */
#define DEBUGGER_MAXIMUM_SYNCRONIZATION_KERNEL_DEBUGGER_OBJECTS 0x40

/**
 * @brief An event to show whether the debugger is running
 * or not in kernel-debugger
 *
 */

//
// Kernel-debugger
//
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_IS_DEBUGGER_RUNNING                 0x0
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_STARTED_PACKET_RECEIVED             0x1
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_PAUSED_DEBUGGEE_DETAILS             0x2
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_CORE_SWITCHING_RESULT               0x3
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_PROCESS_SWITCHING_RESULT            0x4
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_THREAD_SWITCHING_RESULT             0x5
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_SCRIPT_RUNNING_RESULT               0x6
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_SCRIPT_FORMATS_RESULT               0x7
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_DEBUGGEE_FINISHED_COMMAND_EXECUTION 0x8
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_FLUSH_RESULT                        0x9
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_REGISTER_EVENT                      0xa
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_ADD_ACTION_TO_EVENT                 0xb
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_MODIFY_AND_QUERY_EVENT              0xc
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_READ_REGISTERS                      0xd
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_BP                                  0xe
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_LIST_OR_MODIFY_BREAKPOINTS          0xf
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_READ_MEMORY                         0x10
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_EDIT_MEMORY                         0x11
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_SYMBOL_RELOAD                       0x12
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_TEST_QUERY                          0x13
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_CALLSTACK_RESULT                    0x14
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_SEARCH_QUERY_RESULT                 0x15
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_VA2PA_AND_PA2VA_RESULT              0x16
#define DEBUGGER_SYNCRONIZATION_OBJECT_KERNEL_DEBUGGER_PTE_RESULT                          0x17

/**
 * @brief Maximum number of event handles in user-debugger
 */
#define DEBUGGER_MAXIMUM_SYNCRONIZATION_USER_DEBUGGER_OBJECTS 0x40

/**
 * @brief An event to show whether the debugger is running
 * or not in user-debugger
 *
 */

//
// User-debugger
//
#define DEBUGGER_SYNCRONIZATION_OBJECT_USER_DEBUGGER_IS_DEBUGGER_RUNNING 0x30

//////////////////////////////////////////////////
//            	    Structures                  //
//////////////////////////////////////////////////

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

VOID
InterpreterRemoveComments(char * CommandText);

BOOLEAN
ShowErrorMessage(UINT32 Error);

BOOLEAN
IsConnectedToAnyInstanceOfDebuggerOrDebuggee();

BOOLEAN
IsTagExist(UINT64 Tag);

UINT64
DebuggerGetNtoskrnlBase();

BOOLEAN
DebuggerPauseDebuggee();

BOOLEAN
InterpretConditionsAndCodes(vector<string> * SplittedCommand,
                            vector<string> * SplittedCommandCaseSensitive,
                            BOOLEAN          IsConditionBuffer,
                            PUINT64          BufferAddress,
                            PUINT32          BufferLength);

VOID
FreeEventsAndActionsMemory(PDEBUGGER_GENERAL_EVENT_DETAIL Event,
                           PDEBUGGER_GENERAL_ACTION       ActionBreakToDebugger,
                           PDEBUGGER_GENERAL_ACTION       ActionCustomCode,
                           PDEBUGGER_GENERAL_ACTION       ActionScript);

BOOLEAN
SendEventToKernel(PDEBUGGER_GENERAL_EVENT_DETAIL Event,
                  UINT32                         EventBufferLength);

BOOLEAN
RegisterActionToEvent(PDEBUGGER_GENERAL_EVENT_DETAIL Event,
                      PDEBUGGER_GENERAL_ACTION       ActionBreakToDebugger,
                      UINT32                         ActionBreakToDebuggerLength,
                      PDEBUGGER_GENERAL_ACTION       ActionCustomCode,
                      UINT32                         ActionCustomCodeLength,
                      PDEBUGGER_GENERAL_ACTION       ActionScript,
                      UINT32                         ActionScriptLength);

BOOLEAN
InterpretGeneralEventAndActionsFields(
    vector<string> *                    SplittedCommand,
    vector<string> *                    SplittedCommandCaseSensitive,
    DEBUGGER_EVENT_TYPE_ENUM            EventType,
    PDEBUGGER_GENERAL_EVENT_DETAIL *    EventDetailsToFill,
    PUINT32                             EventBufferLength,
    PDEBUGGER_GENERAL_ACTION *          ActionDetailsToFillBreakToDebugger,
    PUINT32                             ActionBufferLengthBreakToDebugger,
    PDEBUGGER_GENERAL_ACTION *          ActionDetailsToFillCustomCode,
    PUINT32                             ActionBufferLengthCustomCode,
    PDEBUGGER_GENERAL_ACTION *          ActionDetailsToFillScript,
    PUINT32                             ActionBufferLengthScript,
    PDEBUGGER_EVENT_PARSING_ERROR_CAUSE ReasonForErrorInParsing);

BOOLEAN
CallstackReturnAddressToCallingAddress(UCHAR * ReturnAddress, PUINT32 IndexOfCallFromReturnAddress);

VOID
CallstackShowFrames(PDEBUGGER_SINGLE_CALLSTACK_FRAME  CallstackFrames,
                    UINT32                            FrameCount,
                    DEBUGGER_CALLSTACK_DISPLAY_METHOD DisplayMethod,
                    BOOLEAN                           Is32Bit);

UINT64
GetNewDebuggerEventTag();

DWORD WINAPI
ListeningSerialPauseDebuggeeThread(PVOID Param);

DWORD WINAPI
ListeningSerialPauseDebuggerThread(PVOID Param);

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
CommandEventsClearAllEventsAndResetTags();

VOID
CommandFlushRequestFlush();

UINT64
GetCommandAttributes(const string & FirstCommand);

VOID
DetachFromProcess();

BOOLEAN
CommandLoadVmmModule();

VOID
ShowAllRegisters();

VOID
CommandPauseRequest();

VOID
CommandGRequest();
