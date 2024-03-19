/**
 * @file Kd.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for routines related to kernel mode debugging
 * @details
 * @version 0.1
 * @date 2020-12-20
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				      Locks 	    			//
//////////////////////////////////////////////////

/**
 * @brief Vmx-root lock for sending response of debugger
 *
 */
volatile LONG DebuggerResponseLock;

/**
 * @brief Vmx-root lock for handling breaks to debugger
 *
 */
volatile LONG DebuggerHandleBreakpointLock;

//////////////////////////////////////////////////
//				      Structures    			//
//////////////////////////////////////////////////

/**
 * @brief request to change the process
 *
 */
typedef struct _DEBUGGEE_REQUEST_TO_CHANGE_PROCESS
{
    UINT32 ProcessId;
    PVOID  Process;

} DEBUGGEE_REQUEST_TO_CHANGE_PROCESS, *PDEBUGGEE_REQUEST_TO_CHANGE_PROCESS;

/**
 * @brief request to change the thread
 *
 */
typedef struct _DEBUGGEE_REQUEST_TO_CHANGE_THREAD
{
    UINT32 ThreadId;
    PVOID  Thread;

} DEBUGGEE_REQUEST_TO_CHANGE_THREAD, *PDEBUGGEE_REQUEST_TO_CHANGE_THREAD;

/**
 * @brief request to pause and halt the system
 *
 */
typedef struct _DEBUGGEE_REQUEST_TO_IGNORE_BREAKS_UNTIL_AN_EVENT
{
    volatile BOOLEAN                        PauseBreaksUntilSpecialMessageSent;
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION SpeialEventResponse;

} DEBUGGEE_REQUEST_TO_IGNORE_BREAKS_UNTIL_AN_EVENT, *PDEBUGGEE_REQUEST_TO_IGNORE_BREAKS_UNTIL_AN_EVENT;

/**
 * @brief store the details of a hardware debug register to ignore any
 * trigger for other threads
 *
 */
typedef struct _HARDWARE_DEBUG_REGISTER_DETAILS
{
    UINT64 Address;
    UINT32 ProcessId;
    UINT32 ThreadId;

} HARDWARE_DEBUG_REGISTER_DETAILS, *PHARDWARE_DEBUG_REGISTER_DETAILS;

//////////////////////////////////////////////////
//				   Functions 	    			//
//////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// Private Interfaces
//

static VOID
KdCustomDebuggerBreakSpinlockLock(PROCESSOR_DEBUGGING_STATE * DbgState, volatile LONG * Lock);

static VOID
KdDummyDPC(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

static VOID
KdFireDpc(PVOID Routine, PVOID Parameter);

static BYTE
KdComputeDataChecksum(_In_reads_bytes_(Length) PVOID Buffer,
                      _In_ UINT32                    Length);

static VOID
KdApplyTasksPreHaltCore(PROCESSOR_DEBUGGING_STATE * DbgState);

static VOID
KdApplyTasksPostContinueCore(PROCESSOR_DEBUGGING_STATE * DbgState);

static VOID
KdContinueDebuggee(_Inout_ PROCESSOR_DEBUGGING_STATE *                              DbgState,
                   _In_ BOOLEAN                                                     PauseBreaksUntilSpecialMessageSent,
                   _In_ _Strict_type_match_ DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION SpeialEventResponse);

static VOID
KdContinueDebuggeeJustCurrentCore(PROCESSOR_DEBUGGING_STATE * DbgState);

static BOOLEAN
KdReadRegisters(_In_ PROCESSOR_DEBUGGING_STATE *            DbgState,
                _Inout_ PDEBUGGEE_REGISTER_READ_DESCRIPTION ReadRegisterRequest);
static BOOLEAN
KdReadMemory(_In_ PGUEST_REGS                            Regs,
             _Inout_ PDEBUGGEE_REGISTER_READ_DESCRIPTION ReadRegisterRequest);

static BOOLEAN
KdSwitchCore(PROCESSOR_DEBUGGING_STATE * DbgState, UINT32 NewCore);

static VOID
KdCloseConnectionAndUnloadDebuggee();

static VOID
KdReloadSymbolDetailsInDebuggee(_In_ PDEBUGGEE_SYMBOL_REQUEST_PACKET SymPacket);

static VOID
KdNotifyDebuggeeForUserInput(DEBUGGEE_USER_INPUT_PACKET * Descriptor, UINT32 Len);

static VOID
KdGuaranteedStepInstruction(PROCESSOR_DEBUGGING_STATE * DbgState);

static VOID
KdRegularStepInInstruction(PROCESSOR_DEBUGGING_STATE * DbgState);

static VOID
KdRegularStepOver(PROCESSOR_DEBUGGING_STATE * DbgState, BOOLEAN IsNextInstructionACall, UINT32 CallLength);

static BOOLEAN
KdPerformRegisterEvent(PDEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET EventDetailHeader,
                       DEBUGGER_EVENT_AND_ACTION_RESULT *                  DebuggerEventAndActionResult);

static BOOLEAN
KdPerformAddActionToEvent(PDEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET ActionDetailHeader,
                          DEBUGGER_EVENT_AND_ACTION_RESULT *                  DebuggerEventAndActionResult);

static VOID
KdQuerySystemState();

static BOOLEAN
KdPerformEventQueryAndModification(PDEBUGGER_MODIFY_EVENTS ModifyAndQueryEvent);

static VOID
KdDispatchAndPerformCommandsFromDebugger(PROCESSOR_DEBUGGING_STATE * DbgState);

static VOID
KdBroadcastHaltOnAllCores();

// ----------------------------------------------------------------------------
// Public Interfaces
//

VOID
KdHaltSystem(PDEBUGGER_PAUSE_PACKET_RECEIVED PausePacket);

VOID
KdHandleDebugEventsWhenKernelDebuggerIsAttached(PROCESSOR_DEBUGGING_STATE * DbgState, BOOLEAN TrapSetByDebugger);

VOID
KdManageSystemHaltOnVmxRoot(PROCESSOR_DEBUGGING_STATE *       DbgState,
                            PDEBUGGER_TRIGGERED_EVENT_DETAILS EventDetails);
BOOLEAN
KdCheckAndHandleNmiCallback(_In_ UINT32 CoreId);

VOID
KdHandleNmi(_Inout_ PROCESSOR_DEBUGGING_STATE * DbgState);

VOID
KdInitializeKernelDebugger();

VOID
KdUninitializeKernelDebugger();

VOID
KdInitializeInstantEventPools();

VOID
KdSendFormatsFunctionResult(UINT64 Value);

VOID
KdSendCommandFinishedSignal(UINT32 CoreId);

VOID
KdHandleBreakpointAndDebugBreakpointsCallback(_In_ UINT32                       CoreId,
                                              _In_ DEBUGGEE_PAUSING_REASON      Reason,
                                              PDEBUGGER_TRIGGERED_EVENT_DETAILS EventDetails);
VOID
KdHandleBreakpointAndDebugBreakpoints(_Inout_ PROCESSOR_DEBUGGING_STATE * DbgState,
                                      _In_ DEBUGGEE_PAUSING_REASON        Reason,
                                      PDEBUGGER_TRIGGERED_EVENT_DETAILS   EventDetails);

VOID
KdHandleRegisteredMtfCallback(_In_ UINT32 CoreId);

VOID
KdHandleHaltsWhenNmiReceivedFromVmxRoot(_Inout_ PROCESSOR_DEBUGGING_STATE * DbgState);

BOOLEAN
KdCheckImmediateMessagingMechanism(UINT32 OperationCode);

BOOLEAN
KdResponsePacketToDebugger(_In_ _Strict_type_match_ DEBUGGER_REMOTE_PACKET_TYPE             PacketType,
                           _In_ _Strict_type_match_ DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION Response,
                           _In_reads_bytes_opt_(OptionalBufferLength) CHAR *                OptionalBuffer,
                           _In_ UINT32                                                      OptionalBufferLength);

BOOLEAN
KdLoggingResponsePacketToDebugger(_In_reads_bytes_(OptionalBufferLength) CHAR * OptionalBuffer,
                                  _In_ UINT32                                   OptionalBufferLength,
                                  _In_ UINT32                                   OperationCode);

BOOLEAN
KdCheckGuestOperatingModeChanges(UINT16 PreviousCsSelector, UINT16 CurrentCsSelector);

BOOLEAN
KdIsGuestOnUsermode32Bit();

VOID
KdHandleNmiBroadcastDebugBreaks(UINT32 CoreId, BOOLEAN IsOnVmxNmiHandler);

VOID
KdUnlockTheHaltedCore(PROCESSOR_DEBUGGING_STATE * DbgState);

BOOLEAN
KdCheckTheHaltedCore(PROCESSOR_DEBUGGING_STATE * DbgState);

BOOLEAN
KdQueryDebuggerQueryThreadOrProcessTracingDetailsByCoreId(UINT32                          CoreId,
                                                          DEBUGGER_THREAD_PROCESS_TRACING TracingType);
