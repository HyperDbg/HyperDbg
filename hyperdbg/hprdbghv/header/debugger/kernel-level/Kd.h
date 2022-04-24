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
    UINT64 Process;

} DEBUGGEE_REQUEST_TO_CHANGE_PROCESS, *PDEBUGGEE_REQUEST_TO_CHANGE_PROCESS;

/**
 * @brief request to change the thread
 *
 */
typedef struct _DEBUGGEE_REQUEST_TO_CHANGE_THREAD
{
    UINT32 ThreadId;
    UINT64 Thread;

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

VOID
KdHaltSystem(PDEBUGGER_PAUSE_PACKET_RECEIVED PausePacket);

VOID
KdHandleDebugEventsWhenKernelDebuggerIsAttached(UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs);

VOID
KdManageSystemHaltOnVmxRoot(ULONG                             CurrentCore,
                            PGUEST_REGS                       GuestRegs,
                            PDEBUGGER_TRIGGERED_EVENT_DETAILS EventDetails);
VOID
KdHandleNmi(UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs);

VOID
KdInitializeKernelDebugger();

VOID
KdUninitializeKernelDebugger();

VOID
KdSendFormatsFunctionResult(UINT64 Value);

VOID
KdSendCommandFinishedSignal(UINT32      CurrentProcessorIndex,
                            PGUEST_REGS GuestRegs);

VOID
KdHandleBreakpointAndDebugBreakpoints(UINT32                            CurrentProcessorIndex,
                                      PGUEST_REGS                       GuestRegs,
                                      DEBUGGEE_PAUSING_REASON           Reason,
                                      PDEBUGGER_TRIGGERED_EVENT_DETAILS EventDetails);

VOID
KdHandleHaltsWhenNmiReceivedFromVmxRoot(UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs);

BOOLEAN
KdNmiCallback(PVOID Context, BOOLEAN Handled);

BOOLEAN
KdResponsePacketToDebugger(
    DEBUGGER_REMOTE_PACKET_TYPE             PacketType,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION Response,
    CHAR *                                  OptionalBuffer,
    UINT32                                  OptionalBufferLength);

BOOLEAN
KdLoggingResponsePacketToDebugger(CHAR * OptionalBuffer, UINT32 OptionalBufferLength, UINT32 OperationCode);

BOOLEAN
KdCheckGuestOperatingModeChanges(UINT16 PreviousCsSelector, UINT16 CurrentCsSelector);

BOOLEAN
KdIsGuestOnUsermode32Bit();
